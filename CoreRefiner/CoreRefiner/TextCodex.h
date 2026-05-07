#pragma once
#include "WRL.h"
#include <dwrite.h>

#include <string>
#include <unordered_map>
#include <vector>

class Canvas;
class Color;

class TextCodex
{
public:
    static TextCodex& Get() noexcept;

    void Init();

    /** 系统字体：返回可复用的 TextFormat（按 key 缓存） */
    Microsoft::WRL::ComPtr<IDWriteTextFormat> GetSystemFormat(
        const std::wstring& fontFamily,
        float fontSize,
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL);

    /** 外部字体：从 .ttf/.otf 路径加载并缓存 FontFace（按路径缓存） */
    Microsoft::WRL::ComPtr<IDWriteFontFace> GetFontFaceFromFile(
        const std::wstring& fontFilePath);

    /**
     * 阶段4-MVP：用系统字体绘制一行到 Canvas（灰度AA，颜色由 textColor 控制）
     * baselineY：基线位置（而不是左上角）
     */
    void DrawLine_SystemFont(
        Canvas& canvas,
        const std::string& text,
        const std::string& fontFamily,
        float fontSize,
        DWRITE_FONT_WEIGHT weight,
        int originX,
        int baselineY,
        Color textColor);

    /**
     * 阶段4-MVP：用外部字体文件绘制一行到 Canvas
     * baselineY：基线位置
     */
    void DrawLine_FontFile(
        Canvas& canvas,
        const std::string& text,
        const std::string& fontFilePath,
        float fontEmSize,
        int originX,
        int baselineY,
        Color textColor);


    IDWriteFactory* GetFactory() const noexcept { return factory_.Get(); }
    /** 阶段5：直接把 TextLayout 给出的 glyphRun 写入 Canvas */
    void DrawGlyphRunToCanvas(
        Canvas& canvas,
        float baselineOriginX,
        float baselineOriginY,
        const DWRITE_GLYPH_RUN& glyphRun,
        Color color);

private:
    TextCodex() = default;


    Microsoft::WRL::ComPtr<IDWriteFontFace> GetSystemFontFace(const std::string& fontFamily, DWRITE_FONT_WEIGHT weight);



    struct SystemFormatKey
    {
        std::wstring family;
        float size = 0.0f;
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;
        DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;

        bool operator==(const SystemFormatKey& o) const noexcept;
    };

    struct SystemFormatKeyHash
    {
        size_t operator()(const SystemFormatKey& k) const noexcept;
    };

private:
    bool initialized_ = false;
    Microsoft::WRL::ComPtr<IDWriteFactory> factory_;

    std::unordered_map<SystemFormatKey, Microsoft::WRL::ComPtr<IDWriteTextFormat>, SystemFormatKeyHash> systemFormats_;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDWriteFontFace>> fontFacesByPath_;

    // 用于创建 FontFile/stream 时暂存（保证 CreateFontFace 完成前数据有效）
    std::vector<uint8_t> fileScratch_;
};