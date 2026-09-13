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

        bool wordWrapEnabled = true;

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
        std::string text;

		// Canvas mode: Auto (usually for UI, auto-sizing based on text content; Fixed (usually for world, fixed size and text wraps/clips within it)
        CanvasMode canvasMode = CanvasMode::Fixed;

		// Clear mode: whether to clear the canvas before drawing (usually clear for UI, no clear for world to allow overlaying multiple texts)
        ClearMode clearMode = ClearMode::Clear;

        // Main font source (system family name or file font)
        FontSource primaryFont = FontSource::System(L"Segoe UI");

        // Fallback fonts: it's recommended to use system family names (can also be extended to files)
        std::vector<FontSource> fallbackFonts;

		// Text Style: common properties that affect the whole text (can be overridden by spans)
        Style style{};

		// Text spans: each span can override specific style properties for a range of text (similar to HTML/CSS spans); the renderer will apply these spans on top of the base style when creating the TextLayout.
        std::vector<Span> spans;

        // Canvas strategy
        float maxWidthPx = 320.0f; // Auto: fixed width; Fixed: usually overridden by canvas width
        int paddingPx = 6;

		// Optional draw offset (relative to the top-left corner of the layout)
        float drawOffsetXPx = 0.0f;
        float drawOffsetYPx = 0.0f;

		// Fixed: destWPx/destHPx > 0 时 layout 盒子用这块区域，原点是 dest 左上。否则用整张画布。
		float destXPx = 0.0f;
		float destYPx = 0.0f;
		float destWPx = 0.0f;
		float destHPx = 0.0f;

		[[nodiscard]] bool HasDestRect() const noexcept
		{
			return destWPx > 0.0f && destHPx > 0.0f;
		}

		void SetDestRect(float x, float y, float w, float h) noexcept
		{
			destXPx = x;
			destYPx = y;
			destWPx = w;
			destHPx = h;
		}

		// Color settings
        Color defaultColor = Colors::White;
        Color backgroundColor = Colors::None;
    };
}