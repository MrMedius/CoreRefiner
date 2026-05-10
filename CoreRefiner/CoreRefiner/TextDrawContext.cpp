#include "TextDrawContext.h"

#include "TextCodex.h"
#include "TextRenderer.h"
#include "Canvas.h"

namespace Text
{
    TextDrawContext::TextDrawContext(TextCodex* owner, std::size_t slot, Text::TextRenderer* renderer) noexcept
        : owner_(owner)
        , slot_(slot)
        , renderer_(renderer)
    {}

    TextDrawContext::TextDrawContext(TextDrawContext&& o) noexcept
        : owner_(o.owner_)
        , slot_(o.slot_)
        , renderer_(o.renderer_)
        , request_(std::move(o.request_))
    {
        o.owner_ = nullptr;
        o.renderer_ = nullptr;
    }

    TextDrawContext& TextDrawContext::operator=(TextDrawContext&& o) noexcept
    {
        if (this != &o)
        {
            Release_();
            owner_ = o.owner_;
            slot_ = o.slot_;
            renderer_ = o.renderer_;
            request_ = std::move(o.request_);
            o.owner_ = nullptr;
            o.renderer_ = nullptr;
        }
        return *this;
    }

    TextDrawContext::~TextDrawContext()
    {
        Release_();
    }

    void TextDrawContext::Release_() noexcept
    {
        if (!owner_)
            return;
        owner_->ReleaseTextRendererSlot_(slot_);
        owner_ = nullptr;
        renderer_ = nullptr;
    }

    Text::MeasureResult TextDrawContext::Measure()
    {
        if (!owner_ || !renderer_)
            throw std::runtime_error("TextDrawContext: invalid (moved-from?)");
        return renderer_->Measure(request_);
    }

    void TextDrawContext::Render(Canvas& canvas)
    {
        if (!owner_ || !renderer_)
            throw std::runtime_error("TextDrawContext: invalid (moved-from?)");
        renderer_->Render(request_, canvas);
    }
}