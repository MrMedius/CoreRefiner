#pragma once
#include <DirectXMath.h>

using namespace DirectX;

struct DynamicTextureTag
{
    using value_type = int;
    static constexpr value_type Default() noexcept { return 0; }
};

struct DynamicCubeTextureTag
{
    using value_type = int;
    static constexpr value_type Default() noexcept { return 0; }
};

struct SpriteUVTag
{
    struct value_type
    {
        XMFLOAT2 offset{ 0.0f, 0.0f };
        XMFLOAT2 scale{ 1.0f, 1.0f };
    };
    static constexpr value_type Default() noexcept { return {}; }
};

struct RingParamsTag
{
    struct value_type
    {
        float startAngleDeg{ 0.0f };
        float endAngleDeg{ 359.0f };
        float ratio{ 1.0f };
        float padding;
    };
    static constexpr value_type Default() noexcept { return {}; }
};



template<class Tag>
struct IProvides
{
    using value_type = typename Tag::value_type;
    virtual ~IProvides() = default;

    virtual value_type Provide(Tag) const noexcept = 0;
};

template<class Tag, class BaseT>
inline typename Tag::value_type TryProvide(const BaseT* pBase) noexcept
{
    if (auto* p = dynamic_cast<const IProvides<Tag>*>(pBase))
    {
        return p->Provide(Tag{});
    }

    return Tag::Default();
}