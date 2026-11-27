#pragma once

#include "TextTypes.h"

#include <cstddef>

class TextCodex;
class Canvas;

namespace Text { class TextRenderer; }


//Single rendering session: Rents a TextRenderer from the TextCodex pool and holds the RenderRequest for the current frame.
//Returns the renderer to the pool and clears its internal cache during destruction to avoid state changes across sessions.

namespace Text
{
    class TextDrawContext
    {
        friend class TextCodex;

        TextDrawContext(TextCodex* owner, std::size_t slot, Text::TextRenderer* renderer) noexcept;

    public:
        TextDrawContext(TextDrawContext&& o) noexcept;
        TextDrawContext& operator=(TextDrawContext&& o) noexcept;

        TextDrawContext(const TextDrawContext&) = delete;
        TextDrawContext& operator=(const TextDrawContext&) = delete;

        ~TextDrawContext();

        // Current frame drawing parameters (UTF-8 text, font, spans, etc.)
        Text::RenderRequest& Request() noexcept { return request_; }
        const Text::RenderRequest& Request() const noexcept { return request_; }

        // Measure according to the current Request (does not change canvas pixels).
        Text::MeasureResult Measure();

        // Render to the canvas according to the current Request.
        void Render(Canvas& canvas);

    private:
        void Release_() noexcept;

        TextCodex* owner_ = nullptr;
        std::size_t slot_ = 0;
        Text::TextRenderer* renderer_ = nullptr;
        Text::RenderRequest request_;
    };
}