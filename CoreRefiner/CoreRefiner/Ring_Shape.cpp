#include "Ring_Shape.h"

#include "Ring.h"
#include "BindableCommon.h"
#include "Channels.h"

#include <cassert>

static_assert(sizeof(Ring_Shape::RingData) == 48u);

// 所有环共用一份显卡缓冲。轮到哪只环，Bind 时写下它自己的数据。
class Ring_Shape::RingCbuf : public Bind::CloningBindable
{
public:
	explicit RingCbuf(Graphics& gfx)
	{
		if (!pPcbuf_)
		{
			pPcbuf_ = std::make_unique<Bind::PixelConstantBuffer<RingData>>(gfx, RingData{}, 2u);
		}
	}

	void Bind(Graphics& gfx) noxnd override
	{
		assert(pOwner_ != nullptr);
		pPcbuf_->Update(gfx, pOwner_->data_);
		pPcbuf_->Bind(gfx);
	}

	void InitializeParentReference(const Drawable& parent) noexcept override
	{
		pOwner_ = static_cast<const Ring_Shape*>(&parent);
	}

	std::unique_ptr<CloningBindable> Clone() const noexcept override
	{
		return std::make_unique<RingCbuf>(*this);
	}

private:
	// 像素着色器 b2。时间占用 b1。
	static std::unique_ptr<Bind::PixelConstantBuffer<RingData>> pPcbuf_;
	// 当前正在画的那只环。
	const Ring_Shape* pOwner_{ nullptr };
};

std::unique_ptr<Bind::PixelConstantBuffer<Ring_Shape::RingData>> Ring_Shape::RingCbuf::pPcbuf_;

Ring_Shape::Ring_Shape(Graphics& gfx)
{
	using namespace Bind;

	// 内径比例用 Ring::Make 的默认 0.75，和着色器常量一致。
	auto model = Ring::Make();
	const auto geometryTag = std::string("$ring");

	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	{
		Technique shade("Shade", Chan::main);
		Step only("lambertianTrans");

		auto pvs = VertexShader::Resolve(gfx, "Effect_Ring_VS.cso");
		only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
		only.AddBindable(std::move(pvs));

		only.AddBindable(PixelShader::Resolve(gfx, "Effect_Ring_PS.cso"));
		only.AddBindable(std::make_shared<TimeCbuf_PS>(gfx));
		only.AddBindable(std::make_shared<RingCbuf>(gfx));
		only.AddBindable(std::make_shared<TransformCbuf>(gfx));
		// 环是平的，两面都画。
		only.AddBindable(Rasterizer::Resolve(gfx, true));
		// 这只环用透明度混合。别的特效可以换成别的混合方式。
		only.AddBindable(Blender::Resolve(gfx, true));

		shade.AddStep(std::move(only));
		AddTechnique(std::move(shade));
	}
}

DirectX::XMMATRIX Ring_Shape::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}

void Ring_Shape::SetInnerColor(DirectX::XMFLOAT4 rgba) noexcept
{
	data_.innerColor = rgba;
}

void Ring_Shape::SetOuterColor(DirectX::XMFLOAT4 rgba) noexcept
{
	data_.outerColor = rgba;
}

void Ring_Shape::SetOpacity(float opacity) noexcept
{
	data_.opacity = opacity;
}

void Ring_Shape::SetInnerWave(bool on) noexcept
{
	data_.innerWave = on ? 1.0f : 0.0f;
}

void Ring_Shape::SetOuterWave(bool on) noexcept
{
	data_.outerWave = on ? 1.0f : 0.0f;
}

void Ring_Shape::SetAmplitude(float amplitude) noexcept
{
	data_.amplitude = amplitude;
}
