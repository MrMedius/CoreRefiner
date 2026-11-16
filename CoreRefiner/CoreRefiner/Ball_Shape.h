#pragma once
#include "Drawable.h"
#include "Bindable.h"
#include "IndexBuffer.h"
#include "Transformation.h"
#include <cstdint>
#include <memory>

namespace Bind
{
	class AttackShadeCbuf;
}

class Ball_Shape : public Drawable
{
public:
	Ball_Shape(Graphics& gfx, DirectX::XMFLOAT3 size);
	void Update(float dt);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	// 记下这一发的颜色编号。colorId 为 0 是灰，1 是白。
	void SetColorId(std::uint32_t colorId) noexcept;
private:
	// 这一发正在使用的颜色缓冲。默认编号 0，也就是灰。
	std::shared_ptr<Bind::AttackShadeCbuf> shade_;
};