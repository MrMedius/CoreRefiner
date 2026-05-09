#pragma once
#include "WRL.h"
#include <dwrite.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "Canvas.h"

/**
 * TextCodex：DirectWrite 资源层单例。
 * - 管理 IDWriteFactory、自定义 FontCollectionLoader 注册。
 * - 缓存：系统 TextFormat、按路径/路径组的自定义 Collection、文件主族名、文件 TextFormat、FontFace。
 */
class TextCodex
{
public:
    static TextCodex& Get() noexcept;

    void Init();

    /**
     * 系统字体集合上的 TextFormat（按 family + 字号 + 字重/样式/拉伸 缓存）。
     */
    Microsoft::WRL::ComPtr<IDWriteTextFormat> GetSystemFormat(
        const std::wstring& fontFamily,
        float fontSize,
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL);

    /**
     * 从磁盘 .ttf/.otf/.ttc 创建 FontFace（按路径缓存，供底层字形/度量等用法）。
     */
    Microsoft::WRL::ComPtr<IDWriteFontFace> GetFontFaceFromFile(const std::wstring& fontFilePath);

    IDWriteFactory* GetFactory() const noexcept { return factory_.Get(); }

    /**
     * 将 TextLayout 回调中的 glyphRun 光栅化到 Canvas（DWriteLayoutRenderer 使用）。
     */
    void DrawGlyphRunToCanvas(
        Canvas& canvas,
        float baselineOriginX,
        float baselineOriginY,
        const DWRITE_GLYPH_RUN& glyphRun,
        Color color);

    /**
     * 由单字体文件构建自定义 IDWriteFontCollection（路径级缓存；与 TextLayout/CreateTextFormat 配套）。
     */
    Microsoft::WRL::ComPtr<IDWriteFontCollection> GetCustomFontCollectionFromFile(const std::wstring& fontFilePath);

    /**
     * 由多个字体文件构建同一 Collection（顺序影响族枚举顺序；整组路径字符串作缓存键）。
     */
    Microsoft::WRL::ComPtr<IDWriteFontCollection> GetCustomFontCollectionFromFiles(const std::vector<std::wstring>& fontFilePaths);

    /**
     * 取集合中第一个字族名称（用于 CreateTextFormat 的 family 参数）。
     */
    std::wstring GetFirstFamilyName(IDWriteFontCollection* collection);

    /**
     * 基于自定义 Collection 的 TextFormat（按路径 + 样式参数缓存；内部复用 Collection 与族名缓存）。
     */
    Microsoft::WRL::ComPtr<IDWriteTextFormat> GetFileTextFormat(
        const std::wstring& fontFilePath,
        float fontSize,
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL);

private:
    TextCodex() = default;

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

    static std::wstring MakeCollectionCacheKey_(const std::vector<std::wstring>& paths);

    Microsoft::WRL::ComPtr<IDWriteFontCollection> GetCustomFontCollectionFromPaths_(const std::vector<std::wstring>& paths);

    std::wstring ResolvePrimaryFamilyName_(const std::wstring& singleFilePath, IDWriteFontCollection* collection);

    bool initialized_ = false;
    Microsoft::WRL::ComPtr<IDWriteFactory> factory_;

    Microsoft::WRL::ComPtr<IDWriteFontCollectionLoader> customCollectionLoader_;

    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDWriteFontCollection>> customCollectionsByKey_;
    std::unordered_map<std::wstring, std::wstring> primaryFamilyNameByKey_;

    std::unordered_map<SystemFormatKey, Microsoft::WRL::ComPtr<IDWriteTextFormat>, SystemFormatKeyHash> systemFormats_;
    std::unordered_map<FileFormatKey, Microsoft::WRL::ComPtr<IDWriteTextFormat>, FileFormatKeyHash> fileFormats_;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDWriteFontFace>> fontFacesByPath_;
};