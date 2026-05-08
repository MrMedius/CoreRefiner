#include "TextCodex.h"

#include "Canvas.h"
#include "Surface.h"
#include "Colors.h"
#include "Util.h"

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <fstream>

#include "DWriteCustomFontCollection.h"

#pragma comment(lib, "dwrite.lib")

static inline uint8_t Mul255(uint8_t v, uint8_t a)
{
    return static_cast<uint8_t>((static_cast<uint32_t>(v) * static_cast<uint32_t>(a) + 127u) / 255u);
}

/** dst = srcOver(dst)，srcColor 的 alpha 由 coverage(0..255) 提供 */
static inline Color AlphaBlendCoverage(Color dst, Color srcColor, uint8_t coverage)
{
    if (coverage == 0) return dst;
    if (coverage == 255) return srcColor;

    const uint32_t inv = 255u - coverage;

    const uint8_t outR = static_cast<uint8_t>((srcColor.GetR() * coverage + dst.GetR() * inv + 127u) / 255u);
    const uint8_t outG = static_cast<uint8_t>((srcColor.GetG() * coverage + dst.GetG() * inv + 127u) / 255u);
    const uint8_t outB = static_cast<uint8_t>((srcColor.GetB() * coverage + dst.GetB() * inv + 127u) / 255u);
    const uint8_t outA = static_cast<uint8_t>(std::min<uint32_t>(255u, (srcColor.GetA() * coverage + dst.GetA() * inv + 127u) / 255u));

    return Color(outR, outG, outB, outA);
}

TextCodex& TextCodex::Get() noexcept
{
    static TextCodex s;
    return s;
}

bool TextCodex::SystemFormatKey::operator==(const SystemFormatKey& o) const noexcept
{
    return family == o.family
        && size == o.size
        && weight == o.weight
        && style == o.style
        && stretch == o.stretch;
}

size_t TextCodex::SystemFormatKeyHash::operator()(const SystemFormatKey& k) const noexcept
{
    // 简单 hash：够用即可
    size_t h = std::hash<std::wstring>{}(k.family);
    h ^= std::hash<int>{}(static_cast<int>(k.weight) * 1315423911) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(static_cast<int>(k.style) * 2654435761) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(static_cast<int>(k.stretch) * 97531) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(static_cast<int>(k.size * 100.0f)) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}

void TextCodex::Init()
{
    if (initialized_) return;

    const HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(factory_.GetAddressOf()));
    if (FAILED(hr) || !factory_)
        throw std::runtime_error("TextCodex: DWriteCreateFactory failed");

    customCollectionLoader_.Attach(static_cast<IDWriteFontCollectionLoader*>(new Text::FontCollectionLoader()));
    HRESULT hrL = factory_->RegisterFontCollectionLoader(customCollectionLoader_.Get());
    if (FAILED(hrL))
        throw std::runtime_error("TextCodex: RegisterFontCollectionLoader failed");

    initialized_ = true;
}

bool TextCodex::FileFormatKey::operator==(const FileFormatKey& o) const noexcept
{
    return path == o.path
        && size == o.size
        && weight == o.weight
        && style == o.style
        && stretch == o.stretch;
}

size_t TextCodex::FileFormatKeyHash::operator()(const FileFormatKey& k) const noexcept
{
    size_t h = std::hash<std::wstring>{}(k.path);
    h ^= std::hash<int>{}(static_cast<int>(k.weight)) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(static_cast<int>(k.style)) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(static_cast<int>(k.stretch)) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(static_cast<int>(k.size * 100.0f)) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}

Microsoft::WRL::ComPtr<IDWriteTextFormat> TextCodex::GetSystemFormat(
    const std::wstring& fontFamily,
    float fontSize,
    DWRITE_FONT_WEIGHT weight,
    DWRITE_FONT_STYLE style,
    DWRITE_FONT_STRETCH stretch)
{
    if (!initialized_) Init();

    SystemFormatKey key{};
    key.family = fontFamily;
    key.size = fontSize;
    key.weight = weight;
    key.style = style;
    key.stretch = stretch;

    if (auto it = systemFormats_.find(key); it != systemFormats_.end())
        return it->second;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmt;
    const HRESULT hr = factory_->CreateTextFormat(
        fontFamily.c_str(),
        nullptr,
        weight,
        style,
        stretch,
        fontSize,
        L"",
        &fmt);
    if (FAILED(hr) || !fmt)
        throw std::runtime_error("TextCodex: CreateTextFormat failed");

    systemFormats_.emplace(std::move(key), fmt);
    return fmt;
}

static std::vector<uint8_t> ReadFileAllBytes(const std::wstring& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("ReadFileAllBytes failed");
    f.seekg(0, std::ios::end);
    const size_t sz = static_cast<size_t>(f.tellg());
    f.seekg(0, std::ios::beg);

    std::vector<uint8_t> buf(sz);
    if (sz > 0) f.read(reinterpret_cast<char*>(buf.data()), sz);
    return buf;
}

Microsoft::WRL::ComPtr<IDWriteFontFace> TextCodex::GetFontFaceFromFile(const std::wstring& fontFilePath)
{
    if (!initialized_) Init();

    if (auto it = fontFacesByPath_.find(fontFilePath); it != fontFacesByPath_.end())
        return it->second;

    // 这里用 CreateFontFileReference 走“文件引用”方式：字体文件必须真实存在于磁盘（符合你打包字体资源的方式）。
    Microsoft::WRL::ComPtr<IDWriteFontFile> fontFile;
    HRESULT hr = factory_->CreateFontFileReference(fontFilePath.c_str(), nullptr, &fontFile);
    if (FAILED(hr) || !fontFile)
        throw std::runtime_error("TextCodex: CreateFontFileReference failed");

    BOOL isSupported = FALSE;
    DWRITE_FONT_FILE_TYPE fileType{};
    DWRITE_FONT_FACE_TYPE faceType{};
    UINT32 numberOfFaces = 0;
    hr = fontFile->Analyze(&isSupported, &fileType, &faceType, &numberOfFaces);
    if (FAILED(hr) || !isSupported || numberOfFaces == 0)
        throw std::runtime_error("TextCodex: font file Analyze failed");

    IDWriteFontFile* files[] = { fontFile.Get() };

    Microsoft::WRL::ComPtr<IDWriteFontFace> face;
    hr = factory_->CreateFontFace(faceType, 1, files, 0, DWRITE_FONT_SIMULATIONS_NONE, &face);
    if (FAILED(hr) || !face)
        throw std::runtime_error("TextCodex: CreateFontFace failed");

    fontFacesByPath_.emplace(fontFilePath, face);
    return face;
}

Microsoft::WRL::ComPtr<IDWriteFontFace> TextCodex::GetSystemFontFace(const std::string& fontFamily, DWRITE_FONT_WEIGHT weight)
{
    Microsoft::WRL::ComPtr<IDWriteFontCollection> sysFonts;
    HRESULT hr = factory_->GetSystemFontCollection(&sysFonts);
    if (FAILED(hr) || !sysFonts) throw std::runtime_error("GetSystemFontCollection failed");

    UINT32 idx = 0;
    BOOL exists = FALSE;
    hr = sysFonts->FindFamilyName(ToWideUtf8(fontFamily).c_str(), &idx, &exists);
    if (FAILED(hr) || !exists) return nullptr;

    Microsoft::WRL::ComPtr<IDWriteFontFamily> family;
    sysFonts->GetFontFamily(idx, &family);

    Microsoft::WRL::ComPtr<IDWriteFont> font;
    family->GetFirstMatchingFont(weight, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font);

    Microsoft::WRL::ComPtr<IDWriteFontFace> face;
    font->CreateFontFace(&face);
    return face;
}


void TextCodex::DrawGlyphRunToCanvas(
    Canvas& canvas,
    float baselineOriginX,
    float baselineOriginY,
    const DWRITE_GLYPH_RUN& glyphRun,
    Color color)
{
    Surface& surface = canvas.GetSurface();
    Microsoft::WRL::ComPtr<IDWriteGlyphRunAnalysis> analysis;
    const DWRITE_MATRIX transform = { 1,0,0,1,0,0 };
    const HRESULT hrA = factory_->CreateGlyphRunAnalysis(
        &glyphRun,
        1.0f,
        &transform,
        DWRITE_RENDERING_MODE_ALIASED,
        DWRITE_MEASURING_MODE_NATURAL,
        0.0f,
        0.0f,
        &analysis
    );
    if (FAILED(hrA) || !analysis)
        return;
    RECT bounds{};
    analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &bounds);
    const int texW = bounds.right - bounds.left;
    const int texH = bounds.bottom - bounds.top;
    if (texW <= 0 || texH <= 0)
        return;
    std::vector<uint8_t> alpha(static_cast<size_t>(texW) * static_cast<size_t>(texH));
    const HRESULT hrT = analysis->CreateAlphaTexture(
        DWRITE_TEXTURE_ALIASED_1x1,
        &bounds,
        alpha.data(),
        static_cast<UINT32>(alpha.size())
    );
    if (FAILED(hrT))
        return;
    // TextLayout::Draw 传入的是 float 基线；这里取 floor 贴近像素网格
    const int baseX = static_cast<int>(std::floor(baselineOriginX));
    const int baseY = static_cast<int>(std::floor(baselineOriginY));
    for (int y = 0; y < texH; ++y)
    {
        const int dstY = baseY + bounds.top + y;
        if (dstY < 0 || dstY >= static_cast<int>(surface.GetHeight()))
            continue;
        for (int x = 0; x < texW; ++x)
        {
            const int dstX = baseX + bounds.left + x;
            if (dstX < 0 || dstX >= static_cast<int>(surface.GetWidth()))
                continue;
            const uint8_t cov = alpha[static_cast<size_t>(y) * texW + x];
            if (!cov) continue;
            const Color dst = surface.GetPixel(dstX, dstY);
            surface.PutPixel(dstX, dstY, AlphaBlendCoverage(dst, color, cov));
        }
    }
}


Microsoft::WRL::ComPtr<IDWriteFontCollection> TextCodex::GetCustomFontCollectionFromFile(const std::wstring& fontFilePath)
{
    if (!initialized_) Init();

    if (auto it = customCollectionsByPath_.find(fontFilePath); it != customCollectionsByPath_.end())
        return it->second;

    Text::FontCollectionKey key{};
    key.filePaths = { fontFilePath };
    std::vector<uint8_t> keyBytes = Text::BuildFontCollectionKeyBytes(key);

    Microsoft::WRL::ComPtr<IDWriteFontCollection> collection;
    HRESULT hr = factory_->CreateCustomFontCollection(
        customCollectionLoader_.Get(),
        keyBytes.data(),
        static_cast<UINT32>(keyBytes.size()),
        &collection
    );
    if (FAILED(hr) || !collection)
        throw std::runtime_error("TextCodex: CreateCustomFontCollection failed");

    customCollectionsByPath_.emplace(fontFilePath, collection);
    return collection;
}

std::wstring TextCodex::GetFirstFamilyName(IDWriteFontCollection* collection)
{
    if (!collection) return L"";

    const UINT32 familyCount = collection->GetFontFamilyCount();
    if (familyCount == 0) return L"";

    Microsoft::WRL::ComPtr<IDWriteFontFamily> family;
    collection->GetFontFamily(0, &family);

    Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> names;
    family->GetFamilyNames(&names);

    UINT32 index = 0;
    BOOL exists = FALSE;
    // 先取 en-us，不存在再取第 0 个
    names->FindLocaleName(L"en-us", &index, &exists);
    if (!exists) index = 0;

    UINT32 len = 0;
    names->GetStringLength(index, &len);

    std::wstring out(len + 1, L'\0');
    names->GetString(index, out.data(), len + 1);
    out.resize(len);
    return out;
}

Microsoft::WRL::ComPtr<IDWriteTextFormat> TextCodex::GetFileTextFormat(
    const std::wstring& fontFilePath,
    float fontSize,
    DWRITE_FONT_WEIGHT weight,
    DWRITE_FONT_STYLE style,
    DWRITE_FONT_STRETCH stretch)
{
    if (!initialized_) Init();

    FileFormatKey key{};
    key.path = fontFilePath;
    key.size = fontSize;
    key.weight = weight;
    key.style = style;
    key.stretch = stretch;

    if (auto it = fileFormats_.find(key); it != fileFormats_.end())
        return it->second;

    auto collection = GetCustomFontCollectionFromFile(fontFilePath);
    const std::wstring familyName = GetFirstFamilyName(collection.Get());
    if (familyName.empty())
        throw std::runtime_error("TextCodex: family name empty for font file");

    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmt;
    HRESULT hr = factory_->CreateTextFormat(
        familyName.c_str(),
        collection.Get(), // 关键：这里不再是 nullptr
        weight,
        style,
        stretch,
        fontSize,
        L"",
        &fmt
    );
    if (FAILED(hr) || !fmt)
        throw std::runtime_error("TextCodex: CreateTextFormat(file) failed");

    fileFormats_.emplace(std::move(key), fmt);
    return fmt;
}


//// 日文（先试 UI 字体，再试传统字体）
//fallbacks.push_back(GetSystemFontFace("Yu Gothic UI", weight));
//fallbacks.push_back(GetSystemFontFace("Meiryo", weight));
//// 中文
//fallbacks.push_back(GetSystemFontFace("Microsoft YaHei UI", weight));
//fallbacks.push_back(GetSystemFontFace("SimSun", weight));
//// 你打包的字体（如果有）
//fallbacks.push_back(GetFontFaceFromFile(ToWideUtf8("asset\\Fonts\\ZiKuXingQiuFeiYangTi-2.ttf")));