#include "TextCodex.h"

#include "Canvas.h"
#include "Surface.h"
#include "Colors.h"
#include "Util.h"

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <fstream>

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

    initialized_ = true;
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

//static void DrawGlyphRunToSurface(
//    IDWriteFontFace* fontFace,
//    float fontEmSize,
//    const std::wstring& text,
//    Surface& surface,
//    int originX,
//    int baselineY,
//    Color textColor)
//{
//    std::vector<UINT32> codepoints;
//    codepoints.reserve(text.size());
//    for (wchar_t ch : text)
//    {
//        codepoints.push_back(static_cast<UINT32>(static_cast<uint16_t>(ch)));
//    }
//    std::vector<UINT16> glyphIndices(codepoints.size());
//    fontFace->GetGlyphIndices(codepoints.data(), (UINT32)codepoints.size(), glyphIndices.data());
//
//    // advances 简化：用 nominal advance（更严谨需 GetDesignGlyphMetrics + scale + kerning）
//    std::vector<FLOAT> advances(text.size(), fontEmSize * 0.6f);
//    std::vector<DWRITE_GLYPH_OFFSET> offsets(text.size(), DWRITE_GLYPH_OFFSET{ 0,0 });
//
//    DWRITE_GLYPH_RUN run{};
//    run.fontFace = fontFace;
//    run.fontEmSize = fontEmSize;
//    run.glyphCount = static_cast<UINT32>(glyphIndices.size());
//    run.glyphIndices = glyphIndices.data();
//    run.glyphAdvances = advances.data();
//    run.glyphOffsets = offsets.data();
//    run.isSideways = FALSE;
//    run.bidiLevel = 0;
//
//    Microsoft::WRL::ComPtr<IDWriteFactory> tmpFactory;
//    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(tmpFactory.GetAddressOf()));
//
//    Microsoft::WRL::ComPtr<IDWriteGlyphRunAnalysis> analysis;
//    const DWRITE_MATRIX transform = { 1,0,0,1,0,0 };
//
//    // 灰度 AA：DWRITE_RENDERING_MODE_NATURAL / MEASURING_MODE_NATURAL1 都可；这里先用 NATURAL
//    HRESULT hr = tmpFactory->CreateGlyphRunAnalysis(
//        &run,
//        1.0f,
//        &transform,
//        DWRITE_RENDERING_MODE_ALIASED,
//        DWRITE_MEASURING_MODE_NATURAL,
//        0.0f,
//        0.0f,
//        &analysis);
//    if (FAILED(hr) || !analysis)
//        throw std::runtime_error("CreateGlyphRunAnalysis failed");
//
//    RECT bounds{};
//    analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &bounds);
//
//    const int texW = bounds.right - bounds.left;
//    const int texH = bounds.bottom - bounds.top;
//    if (texW <= 0 || texH <= 0) return;
//
//    std::vector<uint8_t> alpha(static_cast<size_t>(texW) * static_cast<size_t>(texH));
//    hr = analysis->CreateAlphaTexture(
//        DWRITE_TEXTURE_ALIASED_1x1,
//        &bounds,
//        alpha.data(),
//        static_cast<UINT32>(alpha.size()));
//    if (FAILED(hr))
//        throw std::runtime_error("CreateAlphaTexture failed");
//
//    // bounds 相对 glyph run 原点（baseline），所以写入时要加 originX/baselineY
//    for (int y = 0; y < texH; ++y)
//    {
//        const int dstY = baselineY + bounds.top + y;
//        if (dstY < 0 || dstY >= static_cast<int>(surface.GetHeight())) continue;
//
//        for (int x = 0; x < texW; ++x)
//        {
//            const int dstX = originX + bounds.left + x;
//            if (dstX < 0 || dstX >= static_cast<int>(surface.GetWidth())) continue;
//
//            const uint8_t cov = alpha[static_cast<size_t>(y) * texW + x];
//            if (cov == 0) continue;
//
//            const Color dst = surface.GetPixel(dstX, dstY);
//            surface.PutPixel(dstX, dstY, AlphaBlendCoverage(dst, textColor, cov));
//        }
//    }
//}







// ---------- 1) UTF-16(wstring) -> Unicode codepoints ----------
static std::vector<UINT32> Utf16ToCodepoints(const std::wstring& text)
{
    std::vector<UINT32> cps;
    cps.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i)
    {
        const uint16_t w1 = static_cast<uint16_t>(text[i]);

        // surrogate pair
        if (w1 >= 0xD800 && w1 <= 0xDBFF && (i + 1) < text.size())
        {
            const uint16_t w2 = static_cast<uint16_t>(text[i + 1]);
            if (w2 >= 0xDC00 && w2 <= 0xDFFF)
            {
                const UINT32 hi = static_cast<UINT32>(w1 - 0xD800);
                const UINT32 lo = static_cast<UINT32>(w2 - 0xDC00);
                const UINT32 cp = (hi << 10) + lo + 0x10000;
                cps.push_back(cp);
                ++i;
                continue;
            }
        }

        cps.push_back(static_cast<UINT32>(w1));
    }

    return cps;
}

// ---------- 2) compute advances from design metrics ----------
static std::vector<FLOAT> ComputeAdvances(
    IDWriteFontFace* face,
    float fontEmSize,
    const std::vector<UINT16>& glyphIndices)
{
    const UINT32 glyphCount = static_cast<UINT32>(glyphIndices.size());
    std::vector<DWRITE_GLYPH_METRICS> metrics(glyphCount);

    HRESULT hr = face->GetDesignGlyphMetrics(glyphIndices.data(), glyphCount, metrics.data(), FALSE);
    if (FAILED(hr))
        throw std::runtime_error("GetDesignGlyphMetrics failed");

    DWRITE_FONT_METRICS fm{};
    face->GetMetrics(&fm);
    const float scale = (fm.designUnitsPerEm > 0) ? (fontEmSize / static_cast<float>(fm.designUnitsPerEm)) : 1.0f;

    std::vector<FLOAT> advances(glyphCount);
    for (UINT32 i = 0; i < glyphCount; ++i)
    {
        // advanceWidth is in design units
        advances[i] = metrics[i].advanceWidth * scale;
    }
    return advances;
}

// ---------- 3) alpha blend helper (use your existing AlphaBlendCoverage) ----------
// 这里假设你原本的 AlphaBlendCoverage(dst, color, coverage) 保持不变

// ---------- 4) draw a single glyph run ----------
static void DrawOneGlyphRun(
    IDWriteFactory* factory,
    IDWriteFontFace* face,
    float fontEmSize,
    const std::vector<UINT16>& glyphIndices,
    const std::vector<FLOAT>& advances,
    Surface& surface,
    int originX,
    int baselineY,
    Color textColor)
{
    if (!face) return;
    if (glyphIndices.empty()) return;

    std::vector<DWRITE_GLYPH_OFFSET> offsets(glyphIndices.size(), DWRITE_GLYPH_OFFSET{ 0,0 });

    DWRITE_GLYPH_RUN run{};
    run.fontFace = face;
    run.fontEmSize = fontEmSize;
    run.glyphCount = static_cast<UINT32>(glyphIndices.size());
    run.glyphIndices = glyphIndices.data();
    run.glyphAdvances = advances.data();
    run.glyphOffsets = offsets.data();
    run.isSideways = FALSE;
    run.bidiLevel = 0;

    Microsoft::WRL::ComPtr<IDWriteGlyphRunAnalysis> analysis;
    const DWRITE_MATRIX transform = { 1,0,0,1,0,0 };

    HRESULT hr = factory->CreateGlyphRunAnalysis(
        &run,
        1.0f,
        &transform,
        DWRITE_RENDERING_MODE_ALIASED,
        DWRITE_MEASURING_MODE_NATURAL,
        0.0f, 0.0f,
        &analysis);
    if (FAILED(hr) || !analysis)
        throw std::runtime_error("CreateGlyphRunAnalysis failed");

    RECT bounds{};
    analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &bounds);

    const int texW = bounds.right - bounds.left;
    const int texH = bounds.bottom - bounds.top;
    if (texW <= 0 || texH <= 0) return;

    std::vector<uint8_t> alpha(static_cast<size_t>(texW) * static_cast<size_t>(texH));
    hr = analysis->CreateAlphaTexture(
        DWRITE_TEXTURE_ALIASED_1x1,
        &bounds,
        alpha.data(),
        static_cast<UINT32>(alpha.size()));
    if (FAILED(hr))
        throw std::runtime_error("CreateAlphaTexture failed");

    for (int y = 0; y < texH; ++y)
    {
        const int dstY = baselineY + bounds.top + y;
        if (dstY < 0 || dstY >= static_cast<int>(surface.GetHeight())) continue;

        for (int x = 0; x < texW; ++x)
        {
            const int dstX = originX + bounds.left + x;
            if (dstX < 0 || dstX >= static_cast<int>(surface.GetWidth())) continue;

            const uint8_t cov = alpha[static_cast<size_t>(y) * texW + x];
            if (cov == 0) continue;

            const Color dst = surface.GetPixel(dstX, dstY);
            surface.PutPixel(dstX, dstY, AlphaBlendCoverage(dst, textColor, cov));
        }
    }
}

// ---------- 5) fallback: choose a face per codepoint and split into runs ----------
static UINT16 GetGlyphIndexOrZero(IDWriteFontFace* face, UINT32 cp)
{
    UINT16 gi = 0;
    face->GetGlyphIndices(&cp, 1, &gi);
    return gi;
}

/**
 * faces: 按优先级排列，例如 [PrimaryLatin, Japanese, Chinese, ...]
 * 规则：对每个 codepoint，找到第一个 glyphIndex != 0 的 face；若都为 0，就用 faces[0]（会显示 notdef）。
 */
static void DrawWithFallbackFaces(
    IDWriteFactory* factory,
    const std::vector<Microsoft::WRL::ComPtr<IDWriteFontFace>>& faces,
    float fontEmSize,
    const std::wstring& text,
    Surface& surface,
    int originX,
    int baselineY,
    Color textColor)
{
    if (!factory) throw std::runtime_error("factory is null");
    if (faces.empty() || !faces[0]) throw std::runtime_error("no font faces");

    const std::vector<UINT32> cps = Utf16ToCodepoints(text);

    // 当前 run
    Microsoft::WRL::ComPtr<IDWriteFontFace> curFace;
    std::vector<UINT32> runCps;

    auto flushRun = [&](int& penX)
        {
            if (!curFace || runCps.empty()) return;

            // cps -> glyphIndices
            std::vector<UINT16> glyphIndices(runCps.size());
            curFace->GetGlyphIndices(runCps.data(), (UINT32)runCps.size(), glyphIndices.data());

            // advances from metrics (fix overlap)
            std::vector<FLOAT> advances = ComputeAdvances(curFace.Get(), fontEmSize, glyphIndices);

            // draw
            DrawOneGlyphRun(factory, curFace.Get(), fontEmSize, glyphIndices, advances, surface, penX, baselineY, textColor);

            // advance penX
            float advSum = 0.0f;
            for (float a : advances) advSum += a;
            penX += static_cast<int>(advSum + 0.5f);

            runCps.clear();
        };

    int penX = originX;

    for (UINT32 cp : cps)
    {
        // choose face
        Microsoft::WRL::ComPtr<IDWriteFontFace> chosen = faces[0];
        for (const auto& f : faces)
        {
            if (!f) continue;
            const UINT16 gi = GetGlyphIndexOrZero(f.Get(), cp);
            if (gi != 0)
            {
                chosen = f;
                break;
            }
        }

        if (!curFace)
        {
            curFace = chosen;
            runCps.push_back(cp);
            continue;
        }

        // split when face changes
        if (chosen.Get() != curFace.Get())
        {
            flushRun(penX);
            curFace = chosen;
        }

        runCps.push_back(cp);
    }

    flushRun(penX);
}

// ---------- 6) Replace your old DrawGlyphRunToSurface with this wrapper ----------
static void DrawGlyphRunToSurface(
    IDWriteFactory* factory,
    IDWriteFontFace* primaryFace,
    const std::vector<Microsoft::WRL::ComPtr<IDWriteFontFace>>& fallbackFaces,
    float fontEmSize,
    const std::wstring& text,
    Surface& surface,
    int originX,
    int baselineY,
    Color textColor)
{
    std::vector<Microsoft::WRL::ComPtr<IDWriteFontFace>> faces;
    if (primaryFace)
    {
        Microsoft::WRL::ComPtr<IDWriteFontFace> pf;
        pf = primaryFace;
        faces.push_back(pf);
    }
    for (auto& f : fallbackFaces)
        if (f) faces.push_back(f);

    DrawWithFallbackFaces(factory, faces, fontEmSize, text, surface, originX, baselineY, textColor);
}






void TextCodex::DrawLine_FontFile(
    Canvas& canvas,
    const std::string& text,
    const std::string& fontFilePath,
    float fontEmSize,
    int originX,
    int baselineY,
    Color textColor)
{
    if (!initialized_) Init();

    auto face = GetFontFaceFromFile(ToWideUtf8(fontFilePath));



    std::vector<Microsoft::WRL::ComPtr<IDWriteFontFace>> fallbacks;
    // 日文（先试 UI 字体，再试传统字体）
    fallbacks.push_back(GetSystemFontFace("Yu Gothic UI", DWRITE_FONT_WEIGHT_NORMAL));
    fallbacks.push_back(GetSystemFontFace("Meiryo", DWRITE_FONT_WEIGHT_NORMAL));
    // 中文
    fallbacks.push_back(GetSystemFontFace("Microsoft YaHei UI", DWRITE_FONT_WEIGHT_NORMAL));
    fallbacks.push_back(GetSystemFontFace("SimSun", DWRITE_FONT_WEIGHT_NORMAL));
    // 你打包的字体（如果有）
    fallbacks.push_back(GetFontFaceFromFile(ToWideUtf8("asset\\Fonts\\ZiKuXingQiuFeiYangTi-2.ttf")));

    Surface& s = canvas.GetSurface();
    DrawGlyphRunToSurface(factory_.Get(), face.Get(), fallbacks, fontEmSize, ToWideUtf8(text), s, originX, baselineY, textColor);
    canvas.NotifyPixelsChanged();
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



void TextCodex::DrawLine_SystemFont(
    Canvas& canvas,
    const std::string& text,
    const std::string& fontFamily,
    float fontSize,
    DWRITE_FONT_WEIGHT weight,
    int originX,
    int baselineY,
    Color textColor)
{
    if (!initialized_) Init();

    // 系统字体：这里为了阶段4的“可见闭环”，我们直接走 IDWriteTextLayout + DrawGlyphRun 的路径会更长；
    // 先用系统字体族名创建 format，然后从系统字体集合拿一个 face 来走同样的 GlyphRun 路径。
    Microsoft::WRL::ComPtr<IDWriteFontCollection> sysFonts;
    HRESULT hr = factory_->GetSystemFontCollection(&sysFonts);
    if (FAILED(hr) || !sysFonts) throw std::runtime_error("GetSystemFontCollection failed");

    UINT32 idx = 0;
    BOOL exists = FALSE;
    hr = sysFonts->FindFamilyName(ToWideUtf8(fontFamily).c_str(), &idx, &exists);
    if (FAILED(hr) || !exists) throw std::runtime_error("System font family not found");

    Microsoft::WRL::ComPtr<IDWriteFontFamily> family;
    sysFonts->GetFontFamily(idx, &family);

    Microsoft::WRL::ComPtr<IDWriteFont> font;
    family->GetFirstMatchingFont(weight, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font);

    Microsoft::WRL::ComPtr<IDWriteFontFace> face;
    font->CreateFontFace(&face);


    std::vector<Microsoft::WRL::ComPtr<IDWriteFontFace>> fallbacks;
    // 日文（先试 UI 字体，再试传统字体）
    fallbacks.push_back(GetSystemFontFace("Yu Gothic UI", weight));
    fallbacks.push_back(GetSystemFontFace("Meiryo", weight));
    // 中文
    fallbacks.push_back(GetSystemFontFace("Microsoft YaHei UI", weight));
    fallbacks.push_back(GetSystemFontFace("SimSun", weight));
    // 你打包的字体（如果有）
    fallbacks.push_back(GetFontFaceFromFile(ToWideUtf8("asset\\Fonts\\ZiKuXingQiuFeiYangTi-2.ttf")));

    Surface& s = canvas.GetSurface();
    DrawGlyphRunToSurface(factory_.Get(), face.Get(), fallbacks, fontSize, ToWideUtf8(text), s, originX, baselineY, textColor);
    canvas.NotifyPixelsChanged();
}