#pragma once
#include "EffectBase.h"
#include "Canvas3D.h"
#include "Plane.h"
#include "BindableCommon.h"
#include "CanvasTexture.h"
#include "Channels.h"
#include "RenderGraph.h"
#include "IconAtlas.h"
#include "IModuleNode.h"
#include "Framework/Helpers/Mathematics/XMath.h"
#include "Graphics.h"

#include <memory>

// 一块朝向镜头的画布。图案按节点标签画上去，背景留空。
class MarkerCanvas : public Canvas3D
{
public:
	explicit MarkerCanvas(Graphics& gfx)
		:
		Canvas3D(gfx, kTexSize_, kTexSize_)
	{
		using namespace Bind;
		// 丢掉基类的光照通道。标志只采样画布，并按像素透明度混合。
		techniques.clear();
		Clear(Colors::None);

		auto model = Plane::Make();
		auto vs = VertexShader::Resolve(gfx, "Marker_VS.cso");

		Technique shade("Shade", Chan::main);
		{
			Step only("lambertianTrans");
			only.AddBindable(std::make_shared<CanvasTexture>(gfx));
			only.AddBindable(Sampler::Resolve(gfx, Sampler::Type::Point, Sampler::Address::Clamp));
			only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *vs));
			only.AddBindable(std::move(vs));
			only.AddBindable(PixelShader::Resolve(gfx, "Marker_PS.cso"));
			only.AddBindable(Rasterizer::Resolve(gfx, true));
			only.AddBindable(Blender::Resolve(gfx, true));
			only.AddBindable(std::make_shared<TransformCbuf>(gfx));
			shade.AddStep(std::move(only));
		}
		AddTechnique(std::move(shade));
	}

	// label 决定画哪张节点图，颜色用这个节点所属种类的填色。opacity 是整张图的透明度，0 全透，1 不透明。
	void Show(ModuleNodeLabel label, float opacity)
	{
		Clear(Colors::None);
		const float clamped = opacity < 0.0f ? 0.0f : (opacity > 1.0f ? 1.0f : opacity);
		const auto alpha = static_cast<unsigned char>(clamped * 255.0f);
		const ModuleNodeKind kind = KindOf(label);
		const std::size_t index = ToIndex(kind);
		// fill 是种类色。对不上时用 Other 的淡红。透明度另写，种类色本身的不透明不用。
		const Color fill = index < ModuleNodeKindCount()
			? ModuleNodeKindFill::kFill[index]
			: ModuleNodeKindFill::kFill[ToIndex(ModuleNodeKind::Other)];
		IconAtlas::BlitIcon(*this, NodeIconAtlas::Get(label), Color(fill, alpha));
	}

private:
	// 画布边长。节点图是 16×16，一像素对一位。
	static constexpr unsigned kTexSize_{ 16u };
};

// 原地弹出的节点标志。自己留着一块画布，按标签画 Logo。不碰撞，不跟着攻击走。
class Marker : public EffectBase
{
public:
	Marker(Graphics& gfx, Rgph::RenderGraph& rg)
		:
		EffectBase(effect_Marker),
		gfx_(&gfx)
	{
		canvas_ = std::make_unique<MarkerCanvas>(gfx);
		canvas_->LinkTechniques(rg);
	}

	// label 是要画的节点。须在 SpawnAt 之前调用，池里的下一发可以换成别的节点。
	void SetLogo(ModuleNodeLabel label) noexcept
	{
		label_ = label;
	}

	// pos 是弹出的世界坐标。flip 这只标志不用。
	void SpawnAt(XMFLOAT3 pos, bool flip) override
	{
		(void)flip;
		origin_ = pos;
		age_ = 0.0f;
		ApplyFrame_();
	}

	void OnEnable(void) override
	{
		age_ = 0.0f;
	}

	void Update(float dt) override
	{
		age_ += dt;
		if (age_ >= kLife_)
		{
			RequestDisable();
			return;
		}
		ApplyFrame_();
	}

	void Submit(void) override
	{
		if (age_ >= kLife_ || canvas_ == nullptr)
		{
			return;
		}
		canvas_->Submit(Chan::main);
	}

	void OnCollide(CharacterBase* other) override
	{
		(void)other;
	}

private:
	// 按已经播放的时间写下大小、透明度和朝向。
	void ApplyFrame_()
	{
		if (canvas_ == nullptr)
		{
			return;
		}
		// progress 是 0 到 1。同时放大并变淡。
		const float progress = age_ / kLife_;
		const float scale = kScale0_ + (kScale1_ - kScale0_) * progress;
		const float opacity = 1.0f - progress;
		XMFLOAT3 pos = origin_;
		pos.y += kLift_;
		canvas_->SetPosition(pos);
		canvas_->SetScale({ scale, scale, scale });
		FaceCamera_();
		canvas_->Show(label_, opacity);
	}

	// 让画布平面对着当前镜头。平面在模型 XY 上，法线朝模型 -Z。
	void FaceCamera_()
	{
		if (canvas_ == nullptr || gfx_ == nullptr)
		{
			return;
		}
		const DirectX::XMMATRIX view = gfx_->GetCamera();
		DirectX::XMFLOAT4X4 stored{};
		DirectX::XMStoreFloat4x4(&stored, view);
		const DirectX::XMVECTOR right = DirectX::XMVector3Normalize(
			DirectX::XMVectorSet(stored._11, stored._21, stored._31, 0.0f));
		const DirectX::XMVECTOR up = DirectX::XMVector3Normalize(
			DirectX::XMVectorSet(stored._12, stored._22, stored._32, 0.0f));
		const DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(right, up));
		DirectX::XMMATRIX rot = DirectX::XMMatrixIdentity();
		rot.r[0] = right;
		rot.r[1] = up;
		rot.r[2] = forward;
		DirectX::XMFLOAT4X4 rot4{};
		DirectX::XMStoreFloat4x4(&rot4, rot);
		const DirectX::XMFLOAT3 euler = ExtractEulerAngles(rot4);
		canvas_->SetRotationRad(euler.x, euler.y, euler.z);
	}

	Graphics* gfx_{ nullptr };
	// 这一发要画的节点图。
	ModuleNodeLabel label_{ ModuleNodeLabel::Rule_Return };
	// 弹出点。高度另加 kLift_。
	XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	// 从 SpawnAt 起算的秒数。
	float age_{ 0.0f };
	// 用来画 Logo 的画布。
	std::unique_ptr<MarkerCanvas> canvas_;

	// 一整段弹出的秒数。放大和变淡同时走完。
	static constexpr float kLife_{ 0.45f };
	// 开始时的边长。
	static constexpr float kScale0_{ 2.0f };
	// 结束时的边长。
	static constexpr float kScale1_{ 6.0f };
	// 在弹出点上方抬高，避免贴进地面。
	static constexpr float kLift_{ 1.2f };
};
