#include "SliderCanvasFill.h"

#include "Plane.h"
#include "BindableCommon.h"
#include "CanvasTexture.h"
#include "Channels.h"

#include <cassert>
#include <string>

class SliderCanvasFill::ParamsCbuf : public Bind::CloningBindable
{
public:
	explicit ParamsCbuf(Graphics& gfx, const UINT slot = 0u)
	{
		if (!pPcbuf_)
			pPcbuf_ = std::make_unique<Bind::PixelConstantBuffer<SliderCanvasFill::Params>>(gfx, SliderCanvasFill::Params{}, slot);
	}

	void Bind(Graphics& gfx) noxnd override
	{
		assert(pOwner_ != nullptr);
		pPcbuf_->Update(gfx, pOwner_->params_);
		pPcbuf_->Bind(gfx);
	}

	void InitializeParentReference(const Drawable& parent) noexcept override
	{
		pOwner_ = static_cast<const SliderCanvasFill*>(&parent);
	}

	std::unique_ptr<CloningBindable> Clone() const noexcept override
	{
		return std::make_unique<ParamsCbuf>(*this);
	}

private:
	static std::unique_ptr<Bind::PixelConstantBuffer<SliderCanvasFill::Params>> pPcbuf_;
	const SliderCanvasFill* pOwner_ = nullptr;
};

std::unique_ptr<Bind::PixelConstantBuffer<SliderCanvasFill::Params>> SliderCanvasFill::ParamsCbuf::pPcbuf_;

SliderCanvasFill::SliderCanvasFill(Graphics& gfx, const unsigned width, const unsigned height)
	:
	Canvas(width, height)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model = Plane::Make2D(1, 1);
	const auto geometryTag = std::string("$slider_fill.") + std::to_string(width) + "x" + std::to_string(height);
	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tcb = std::make_shared<Transform2DCbuf>(gfx);
	auto canvasTex = std::make_shared<CanvasTexture>(gfx);
	auto paramCb = std::make_shared<ParamsCbuf>(gfx);

	Technique ui("UI", Chan::ui);
	{
		Step draw("ui");

		draw.AddBindable(canvasTex);
		draw.AddBindable(Sampler::Resolve(gfx, Sampler::Type::Point, Sampler::Address::Clamp));

		auto pvs = VertexShader::Resolve(gfx, "Canvas2D_VS.cso");
		draw.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
		draw.AddBindable(std::move(pvs));

		draw.AddBindable(PixelShader::Resolve(gfx, "SliderCanvasFill_PS.cso"));

		draw.AddBindable(tcb);
		draw.AddBindable(paramCb);

		ui.AddStep(std::move(draw));
	}
	AddTechnique(std::move(ui));
}
