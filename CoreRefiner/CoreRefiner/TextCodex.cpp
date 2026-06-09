#include "TextCodex.h"
#include "DWriteCustomFontCollection.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "dwrite.lib")

// =============================================================================
// Pixel blending (glyph alpha -> Surface)
// =============================================================================

// dst = srcOver(dst), srcColor's alpha is provided by coverage (0..255)
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

// =============================================================================
// Singleton
// =============================================================================

TextCodex& TextCodex::Get() noexcept
{
    static TextCodex inst;
    return inst;
}

// =============================================================================
// Init / BeginDraw
// =============================================================================

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
    const HRESULT hrL = factory_->RegisterFontCollectionLoader(customCollectionLoader_.Get());
    if (FAILED(hrL))
        throw std::runtime_error("TextCodex: RegisterFontCollectionLoader failed");

    initialized_ = true;
}

Text::TextDrawContext TextCodex::BeginDraw()
{
    const std::size_t slot = AcquireTextRendererSlot_();
    return Text::TextDrawContext(this, slot, textRendererPool_[slot].get());
}

// =============================================================================
// TextRenderer pool (TextDrawContext)
// =============================================================================

std::size_t TextCodex::AcquireTextRendererSlot_()
{
    if (!initialized_) Init();

    for (std::size_t i = 0; i < kTextRendererPoolCap; ++i)
    {
        if (!textRendererSlotUsed_[i])
        {
            textRendererSlotUsed_[i] = true;
            if (!textRendererPool_[i])
                textRendererPool_[i] = std::make_unique<Text::TextRenderer>(*this);
            return i;
        }
    }
    throw std::runtime_error("TextCodex: TextRenderer pool exhausted (increase kTextRendererPoolCap or avoid nested BeginDraw)");
}

void TextCodex::ReleaseTextRendererSlot_(std::size_t slot) noexcept
{
    if (slot >= kTextRendererPoolCap)
        return;
    if (!textRendererSlotUsed_[slot])
        return;

    textRendererSlotUsed_[slot] = false;
    if (textRendererPool_[slot])
        textRendererPool_[slot]->ResetForPool_();
}

// =============================================================================
// Cache keys: SystemFormatKey / FileFormatKey
// =============================================================================

bool TextCodex::SystemFormatKey::operator==(const SystemFormatKey& o) const noexcept
{
    return family   == o.family
        && size     == o.size
        && weight   == o.weight
        && style    == o.style
        && stretch  == o.stretch;
}

size_t TextCodex::SystemFormatKeyHash::operator()(const SystemFormatKey& k) const noexcept
{
    size_t h = std::hash<std::wstring>{}(k.family);
    h ^= std::hash<int>{}(static_cast<int>(k.weight) * 1315423911) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(static_cast<int>(k.style) * 2654435761) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(static_cast<int>(k.stretch) * 97531) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(static_cast<int>(k.size * 100.0f)) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}

bool TextCodex::FileFormatKey::operator==(const FileFormatKey& o) const noexcept
{
    return path    == o.path
        && size    == o.size
        && weight  == o.weight
        && style   == o.style
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

// =============================================================================
// TextRenderer: Custom FontCollection and primary family name
// =============================================================================

std::wstring TextCodex::MakeCollectionCacheKey_(const std::vector<std::wstring>& paths)
{
    if (paths.empty())
        return L"";

    if (paths.size() == 1)
        return paths.front();

    std::wstring key;
    key.push_back(L'\x1E');
    for (size_t i = 0; i < paths.size(); ++i)
    {
        if (i > 0) key.push_back(L'\x1F');
        key += paths[i];
    }
    return key;
}

Microsoft::WRL::ComPtr<IDWriteFontCollection> TextCodex::GetCustomFontCollectionFromPaths_(const std::vector<std::wstring>& paths)
{
    if (!initialized_) Init();
    if (paths.empty())
        throw std::runtime_error("TextCodex: font file path list empty");

    const std::wstring cacheKey = MakeCollectionCacheKey_(paths);
    if (auto it = customCollectionsByKey_.find(cacheKey); it != customCollectionsByKey_.end())
        return it->second;

    Text::FontCollectionKey key{};
    key.filePaths = paths;
    std::vector<uint8_t> keyBytes = Text::BuildFontCollectionKeyBytes(key);

    Microsoft::WRL::ComPtr<IDWriteFontCollection> collection;
    const HRESULT hr = factory_->CreateCustomFontCollection(
        customCollectionLoader_.Get(),
        keyBytes.data(),
        static_cast<UINT32>(keyBytes.size()),
        &collection
    );
    if (FAILED(hr) || !collection)
        throw std::runtime_error("TextCodex: CreateCustomFontCollection failed");

    customCollectionsByKey_.emplace(cacheKey, collection);
    return collection;
}

Microsoft::WRL::ComPtr<IDWriteFontCollection> TextCodex::GetCustomFontCollectionFromFile(const std::wstring& fontFilePath)
{
    return GetCustomFontCollectionFromPaths_({ fontFilePath });
}

Microsoft::WRL::ComPtr<IDWriteFontCollection> TextCodex::GetCustomFontCollectionFromFiles(const std::vector<std::wstring>& fontFilePaths)
{
    return GetCustomFontCollectionFromPaths_(fontFilePaths);
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
    names->FindLocaleName(L"en-us", &index, &exists);
    if (!exists) index = 0;

    UINT32 len = 0;
    names->GetStringLength(index, &len);

    std::wstring out(len + 1, L'\0');
    names->GetString(index, out.data(), len + 1);
    out.resize(len);
    return out;
}

std::wstring TextCodex::ResolvePrimaryFamilyName_(const std::wstring& singleFilePath, IDWriteFontCollection* collection)
{
    const std::wstring key = MakeCollectionCacheKey_({ singleFilePath });
    if (auto it = primaryFamilyNameByKey_.find(key); it != primaryFamilyNameByKey_.end())
        return it->second;

    std::wstring name = GetFirstFamilyName(collection);
    if (name.empty())
        throw std::runtime_error("TextCodex: family name empty for font file");

    primaryFamilyNameByKey_.emplace(key, name);
    return name;
}

// =============================================================================
// TextRenderer：TextFormat / FontFace
// =============================================================================

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
    const std::wstring familyName = ResolvePrimaryFamilyName_(fontFilePath, collection.Get());

    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmt;
    const HRESULT hr = factory_->CreateTextFormat(
        familyName.c_str(),
        collection.Get(),
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

Microsoft::WRL::ComPtr<IDWriteFontFace> TextCodex::GetFontFaceFromFile(const std::wstring& fontFilePath)
{
    if (!initialized_) Init();

    if (auto it = fontFacesByPath_.find(fontFilePath); it != fontFacesByPath_.end())
        return it->second;

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

// =============================================================================
// DWriteLayoutRenderer: glyph rasterization
// =============================================================================

void TextCodex::DrawGlyphRunToCanvas(
    Canvas& canvas,
    float baselineOriginX,
    float baselineOriginY,
    const DWRITE_GLYPH_RUN& glyphRun,
    Color color)
{
    if (!initialized_) Init();

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
