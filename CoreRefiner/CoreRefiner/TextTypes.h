#pragma once
#pragma once
#include <dwrite.h>
#include <string>
#include <vector>
#include <optional>
#include "Colors.h"

namespace Text
{
    enum class CanvasMode { Fixed, Auto };
    enum class ClearMode  { Clear, NoClear };
    enum class FontSourceKind { SystemFamily, FontFile };

    struct FontSource
    {
        FontSourceKind kind = FontSourceKind::SystemFamily;
        std::wstring systemFamily;  // kind==SystemFamily
        std::wstring fontFilePath;  // kind==FontFile

        static FontSource System(std::wstring family)
        {
            FontSource s;
            s.kind = FontSourceKind::SystemFamily;
            s.systemFamily = std::move(family);
            return s;
        }

        static FontSource File(std::wstring path)
        {
            FontSource s;
            s.kind = FontSourceKind::FontFile;
            s.fontFilePath = std::move(path);
            return s;
        }
    };

    struct Style
    {
        float fontSize = 24.0f;
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
        DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;

        DWRITE_TEXT_ALIGNMENT textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
        DWRITE_PARAGRAPH_ALIGNMENT paragraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
        DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_WRAP;

        float lineSpacing = 0.0f; // 0=默认；>0=统一行距
    };

    struct Span
    {
        // UTF-16 code unit range（与 DirectWrite 完全一致）
        UINT32 start = 0;
        UINT32 length = 0;

        std::optional<Color> color;
        std::optional<DWRITE_FONT_WEIGHT> weight;
        std::optional<DWRITE_FONT_STYLE> style;
        std::optional<DWRITE_FONT_STRETCH> stretch;
        std::optional<std::wstring> fontFamily;
    };

    struct MeasureResult
    {
        UINT widthPx = 1;
        UINT heightPx = 1;
    };

    struct RenderRequest
    {
        // 输入文本：统一约定 UTF-8（跨机器不受系统区域影响）
        std::string utf8Text;

        CanvasMode canvasMode = CanvasMode::Fixed;
        ClearMode clearMode = ClearMode::Clear;

        // 主字体来源（系统族名或文件字体）
        FontSource primaryFont = FontSource::System(L"Segoe UI");

        // fallback：建议放系统族名（也可再扩展为文件）
        std::vector<FontSource> fallbackFonts;

        Style style{};
        std::vector<Span> spans;

        // 画布策略
        float maxWidthPx = 320.0f; // Auto：固定宽度；Fixed：通常会被 canvas 宽覆盖
        int paddingPx = 6;

        // 颜色
        Color defaultColor;
        Color backgroundColor;
    };
}