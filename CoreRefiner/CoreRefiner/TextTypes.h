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

        float lineSpacing = 0.0f; //0 = Default; >0 = Uniform line spacing
    };

    struct Span
    {
        // UTF-16 code unit range（Identical to DirectWrite）
        UINT32 start = 0;
        UINT32 length = 0;

        std::optional<DWRITE_FONT_WEIGHT> weight;
        std::optional<DWRITE_FONT_STYLE> style;
        std::optional<DWRITE_FONT_STRETCH> stretch;
        std::optional<std::wstring> fontFamily;
        std::optional<bool> underline;
        std::optional<bool> strikethrough;
        std::optional<Color> color;
    };

    struct MeasureResult
    {
        UINT widthPx = 1;
        UINT heightPx = 1;
    };

    struct RenderRequest
    {
        // Input text: UTF-8 (standardized across machines and is not affected by system region)
        std::string utf8Text;

        CanvasMode canvasMode = CanvasMode::Fixed;
        ClearMode clearMode = ClearMode::Clear;

        // Main font source (system family name or file font)
        FontSource primaryFont = FontSource::System(L"Segoe UI");

        // Fallback fonts: it's recommended to use system family names (can also be extended to files)
        std::vector<FontSource> fallbackFonts;

        // Text style and spans
        Style style{};
        std::vector<Span> spans;

        // Canvas strategy
        float maxWidthPx = 320.0f; // Auto: fixed width; Fixed: usually overridden by canvas width
        int paddingPx = 6;

		// Color settings
        Color defaultColor;
        Color backgroundColor;
    };
}