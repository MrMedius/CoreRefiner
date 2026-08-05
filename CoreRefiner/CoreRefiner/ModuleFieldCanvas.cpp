#include "ModuleFieldCanvas.h"
#include "Plane.h"
#include "BindableCommon.h"
#include "CanvasTexture.h"
#include "Channels.h"

#include <algorithm>
#include <cassert>

// ------------------------------------------------------------------
// ParamsCbuf
// ------------------------------------------------------------------
class ModuleFieldCanvas::ParamsCbuf : public Bind::CloningBindable
{
public:
	explicit ParamsCbuf(Graphics& gfx, UINT slot = 0u)
	{
		if (!pPcbuf_)
		{
			pPcbuf_ = std::make_unique<Bind::PixelConstantBuffer<ModuleFieldCanvas::Params>>(
				gfx, ModuleFieldCanvas::Params{}, slot);
		}
	}

	void Bind(Graphics& gfx) noxnd override
	{
		assert(pOwner_ != nullptr);
		pPcbuf_->Update(gfx, pOwner_->params_);
		pPcbuf_->Bind(gfx);
	}

	void InitializeParentReference(const Drawable& parent) noexcept override
	{
		pOwner_ = static_cast<const ModuleFieldCanvas*>(&parent);
	}

	std::unique_ptr<CloningBindable> Clone() const noexcept override
	{
		return std::make_unique<ParamsCbuf>(*this);
	}

private:
	static std::unique_ptr<Bind::PixelConstantBuffer<ModuleFieldCanvas::Params>> pPcbuf_;
	const ModuleFieldCanvas* pOwner_ = nullptr;
};

std::unique_ptr<Bind::PixelConstantBuffer<ModuleFieldCanvas::Params>>
	ModuleFieldCanvas::ParamsCbuf::pPcbuf_;

// ------------------------------------------------------------------
// ModuleFieldCanvas
// ------------------------------------------------------------------
ModuleFieldCanvas::ModuleFieldCanvas(Graphics& gfx, unsigned width, unsigned height, Color bgColor)
	:
	Canvas(width, height),
	width_(width),
	height_(height)
{
	using namespace Bind;

	params_.aspect = (height_ == 0u)
		? 1.0f
		: static_cast<float>(width_) / static_cast<float>(height_);
	params_.ringCount = 0;

	Clear(bgColor);
	NotifyPixelsChanged();

	auto model = Plane::Make2D(1, 1);
	const auto geometryTag = std::string("$module_field.")
		+ std::to_string(width) + "x" + std::to_string(height);
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

		auto pvs = VertexShader::Resolve(gfx, "ScanWaveField_VS.cso");
		draw.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
		draw.AddBindable(std::move(pvs));

		draw.AddBindable(PixelShader::Resolve(gfx, "ScanWaveField_PS.cso"));

		draw.AddBindable(tcb);
		draw.AddBindable(paramCb);

		ui.AddStep(std::move(draw));
	}
	AddTechnique(std::move(ui));
}

void ModuleFieldCanvas::ClearWaves() noexcept
{
	params_.ringCount = 0;
	for (unsigned i = 0u; i < kMaxRings; ++i)
	{
		params_.rings[i] = DirectX::XMFLOAT4{ 0.0f, 0.0f, 0.0f, 0.0f };
	}
}

void ModuleFieldCanvas::SetWavesLocal(
	const DirectX::XMFLOAT2* centers,
	const float* radii,
	unsigned count,
	float fieldSide) noexcept
{
	ClearWaves();
	if (centers == nullptr || radii == nullptr || fieldSide <= 0.0f)
	{
		return;
	}

	const unsigned n = std::min(count, kMaxRings);
	const float invSide = 1.0f / fieldSide;
	for (unsigned i = 0u; i < n; ++i)
	{
		params_.rings[i] = DirectX::XMFLOAT4{
			centers[i].x * invSide,
			centers[i].y * invSide,
			radii[i] * invSide,
			0.0f
		};
	}
	params_.ringCount = static_cast<int>(n);
}
