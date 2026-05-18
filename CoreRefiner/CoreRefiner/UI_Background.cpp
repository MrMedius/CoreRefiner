#include "UI_Background.h"
#include "Plane.h"
#include "BindableCommon.h"
#include "CanvasTexture.h"
#include "Channels.h"

#include "TextCodex.h"


// ------------------------------------------------------------------ 
// ParamsCbuf
// ------------------------------------------------------------------ 
class UI_Background::ParamsCbuf : public Bind::CloningBindable
{
public:
	explicit ParamsCbuf(Graphics& gfx, UINT slot = 0u)
	{
		if (!pPcbuf_)
			pPcbuf_ = std::make_unique<Bind::PixelConstantBuffer<UI_Background::Params>>(gfx, UI_Background::Params{}, slot);
	}
	void Bind(Graphics& gfx) noxnd override
	{
		assert(pOwner_ != nullptr);
		pPcbuf_->Update(gfx, pOwner_->params_);
		pPcbuf_->Bind(gfx);
	}
	void InitializeParentReference(const Drawable& parent) noexcept override
	{
		pOwner_ = static_cast<const UI_Background*>(&parent);
	}
	std::unique_ptr<CloningBindable> Clone() const noexcept override
	{
		return std::make_unique<ParamsCbuf>(*this);
	}
private:
	static std::unique_ptr<Bind::PixelConstantBuffer<UI_Background::Params>> pPcbuf_;
	const UI_Background* pOwner_ = nullptr;
};

std::unique_ptr<Bind::PixelConstantBuffer<UI_Background::Params>> UI_Background::ParamsCbuf::pPcbuf_;



// ------------------------------------------------------------------ 
// UI_Background
// ------------------------------------------------------------------
UI_Background::UI_Background(Graphics& gfx, unsigned width, unsigned height)
	:
	Canvas(width, height)
{
	using namespace Bind;
	namespace dx = DirectX;

	params_ = MakeDefaultParams(width, height);

	auto model = Plane::Make2D(1, 1);
	const auto geometryTag = std::string("$ui_bg.") + std::to_string(width) + std::to_string(height);
	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tcb = std::make_shared<Transform2DCbuf>(gfx);
	auto canvasTex = std::make_shared<CanvasTexture>(gfx);
	auto timeCb = std::make_shared<TimeCbuf_PS>(gfx);
	auto paramCb = std::make_shared<ParamsCbuf>(gfx);

	Technique ui("UI", Chan::ui);
	{
		Step draw("ui");

		draw.AddBindable(canvasTex);
		draw.AddBindable(Sampler::Resolve(gfx, Sampler::Type::Point, Sampler::Address::Clamp));

		auto pvs = VertexShader::Resolve(gfx, "UI_Background_VS.cso");
		draw.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
		draw.AddBindable(std::move(pvs));

		draw.AddBindable(PixelShader::Resolve(gfx, "UI_Background_PS.cso"));

		draw.AddBindable(tcb);
		draw.AddBindable(timeCb);
		draw.AddBindable(paramCb);

		ui.AddStep(std::move(draw));
	}
	AddTechnique(std::move(ui));
}

UI_Background::Params UI_Background::MakeDefaultParams(const unsigned width, const unsigned height) noexcept
{
	Params p{};
	if (width == 0u || height == 0u)
		return p;
	const float w = static_cast<float>(width);
	const float h = static_cast<float>(height);
	const float halfMin = 0.5f * std::min(w, h);
	p.aspect = w / h;
	return p;
}