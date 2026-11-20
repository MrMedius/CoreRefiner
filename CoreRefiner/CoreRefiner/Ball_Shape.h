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
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	// 清掉这一发外形上已经记下的颜色编号。
	void ClearColorIds() noexcept;
	// 按写入顺序再追加一条。colorId 为 0 是灰，1 是白，2 是黑。
	void AppendColorId(std::uint32_t colorId);
private:
	// 这一发的颜色列表。没有编号时球是灰色。
	std::shared_ptr<Bind::AttackShadeCbuf> shade_;
};