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


    IDWriteFactory* GetFactory() const noexcept { return factory_.Get(); }
    /** 阶段5：直接把 TextLayout 给出的 glyphRun 写入 Canvas */
    void DrawGlyphRunToCanvas(
        Canvas& canvas,
        float baselineOriginX,
        float baselineOriginY,
        const DWRITE_GLYPH_RUN& glyphRun,
        Color color);


    // 自定义字体集合：单文件也用 collection 表达（后续可扩展多文件）
    Microsoft::WRL::ComPtr<IDWriteFontCollection> GetCustomFontCollectionFromFile(const std::wstring& fontFilePath);
    // 从 collection 取第一个 family 名称（用于 CreateTextFormat）
    std::wstring GetFirstFamilyName(IDWriteFontCollection* collection);
    // 用“文件字体”创建可缓存的 TextFormat
    Microsoft::WRL::ComPtr<IDWriteTextFormat> GetFileTextFormat(
        const std::wstring& fontFilePath,
        float fontSize,
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL);

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


    // custom collection loader（需保持活到进程退出）
    Microsoft::WRL::ComPtr<IDWriteFontCollectionLoader> customCollectionLoader_;

    // cache: fontFilePath -> custom collection
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDWriteFontCollection>> customCollectionsByPath_;

    // cache: (path + size + weight + style + stretch) -> format
    struct FileFormatKey
    {
        std::wstring path;
        float size = 0.0f;
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;
        DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;
        bool operator==(const FileFormatKey& o) const noexcept;
    };

    struct FileFormatKeyHash
    {
        size_t operator()(const FileFormatKey& k) const noexcept;
    };

    std::unordered_map<FileFormatKey, Microsoft::WRL::ComPtr<IDWriteTextFormat>, FileFormatKeyHash> fileFormats_;

private:
    bool initialized_ = false;
    Microsoft::WRL::ComPtr<IDWriteFactory> factory_;

    std::unordered_map<SystemFormatKey, Microsoft::WRL::ComPtr<IDWriteTextFormat>, SystemFormatKeyHash> systemFormats_;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDWriteFontFace>> fontFacesByPath_;

    // 用于创建 FontFile/stream 时暂存（保证 CreateFontFace 完成前数据有效）
    std::vector<uint8_t> fileScratch_;
};