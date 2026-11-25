#pragma once
#include "EffectBase.h"
#include "Drawable.h"
#include "Sphere.h"
#include "BindableCommon.h"
#include "Channels.h"
#include "RenderGraph.h"
#include "ColliderComponentBase.h"

#include <cmath>
#include <memory>

// 单位球。半径是 1，世界半径由特效的缩放决定。正对镜头的地方透，轮廓是深色。
class ExecuteSphere_Shape : public Drawable
{
public:
	explicit ExecuteSphere_Shape(Graphics& gfx)
	{
		using namespace Bind;

		auto model = Sphere::Make();
		pVertices = VertexBuffer::Resolve(gfx, "$executeSphere", model.vertices);
		pIndices = IndexBuffer::Resolve(gfx, "$executeSphere", model.indices);
		pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		{
			Technique shade("Shade", Chan::main);
			Step only("lambertianTrans");

			auto pvs = VertexShader::Resolve(gfx, "ExecuteSphere_VS.cso");
			only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			only.AddBindable(std::move(pvs));
			only.AddBindable(PixelShader::Resolve(gfx, "ExecuteSphere_PS.cso"));
			only.AddBindable(std::make_shared<TransformCbuf>(gfx));
			only.AddBindable(Rasterizer::Resolve(gfx, false));
			only.AddBindable(Blender::Resolve(gfx, true));

			shade.AddStep(std::move(only));
			AddTechnique(std::move(shade));
		}
	}

	DirectX::XMMATRIX GetTransformXM() const noexcept override
	{
		return trans.GetTransformXM();
	}
};

// 秒杀时留下的外形副本。网格、颜色和朝向在播放时从另一只外形记下。球外的像素不画。
class ExecuteVictim_Shape : public Drawable
{
public:
	// 和 ModelClip_PS 的 b1 一致，共 32 字节。
	// centerWorld 是裁切球心的世界坐标。radius 是当前世界半径。
	// color 是记下的外形颜色。
	struct ClipData
	{
		DirectX::XMFLOAT3 centerWorld{ 0.0f, 0.0f, 0.0f };
		float radius{ 0.0f };
		DirectX::XMFLOAT4 color{ 0.0f, 0.0f, 0.0f, 1.0f };
	};

	explicit ExecuteVictim_Shape(Graphics& gfx)
	{
		using namespace Bind;

		gfx_ = &gfx;
		vs_ = VertexShader::Resolve(gfx, "ModelClip_VS.cso");
		layoutSlot_ = std::make_shared<LayoutSlot>();

		Technique shade("Shade", Chan::main);
		Step only("lambertian");
		only.AddBindable(layoutSlot_);
		only.AddBindable(vs_);
		only.AddBindable(PixelShader::Resolve(gfx, "ModelClip_PS.cso"));
		only.AddBindable(std::make_shared<ClipCbuf>(gfx, this));
		only.AddBindable(std::make_shared<TransformCbuf>(gfx));
		only.AddBindable(Rasterizer::Resolve(gfx, false));
		shade.AddStep(std::move(only));
		AddTechnique(std::move(shade));
	}

	DirectX::XMMATRIX GetTransformXM() const noexcept override
	{
		return trans.GetTransformXM();
	}

	// source 是被处决的外形。记下它在 Codex 里的网格、当前位置、旋转、缩放和颜色。
	// 空指针、没有网格或没有颜色时清掉副本。
	void Adopt(const Drawable* source)
	{
		if (source == nullptr || gfx_ == nullptr || vs_ == nullptr)
		{
			ClearMesh_();
			return;
		}

		auto vertices = source->ShareVertices();
		auto indices = source->ShareIndices();
		auto topology = source->ShareTopology();
		DirectX::XMFLOAT4 color{};
		if (vertices == nullptr || indices == nullptr || topology == nullptr || !source->GetMaterialColor(color))
		{
			ClearMesh_();
			return;
		}

		pVertices = std::move(vertices);
		pIndices = std::move(indices);
		pTopology = std::move(topology);
		SetPosition(source->GetPosition());
		const DirectX::XMFLOAT3 rotation = source->GetRotationRaw();
		SetRotationRad(rotation.x, rotation.y, rotation.z);
		SetScale(source->GetScale());
		clip_.color = color;
		if (layoutSlot_ != nullptr)
		{
			layoutSlot_->Set(Bind::InputLayout::Resolve(*gfx_, pVertices->GetLayout(), *vs_));
		}
	}

	// centerWorld 是这一帧的球心。radius 是这一帧的世界半径。颜色保持记下的值。
	void SetClip(DirectX::XMFLOAT3 centerWorld, float radius) noexcept
	{
		clip_.centerWorld = centerWorld;
		clip_.radius = radius;
	}

	// 绘制前读取。半径不是正数时调用方不应提交。
	[[nodiscard]] const ClipData& GetClip() const noexcept
	{
		return clip_;
	}

	// 已经记下网格时才提交。
	[[nodiscard]] bool HasMesh() const noexcept
	{
		return pVertices != nullptr && pIndices != nullptr && pTopology != nullptr;
	}

private:
	// 输入布局跟着记下的网格换。步骤里只放这一个槽，重复播放不会越加越多。
	class LayoutSlot : public Bind::Bindable
	{
	public:
		void Set(std::shared_ptr<Bind::InputLayout> layout) noexcept
		{
			layout_ = std::move(layout);
		}

		void Bind(Graphics& gfx) noxnd override
		{
			if (layout_ != nullptr)
			{
				layout_->Bind(gfx);
			}
		}

	private:
		// 当前网格对应的输入布局。没有网格时是空的。
		std::shared_ptr<Bind::InputLayout> layout_;
	};

	// 每帧把这一只副本的球心、半径和记下的颜色写进 b1。
	class ClipCbuf : public Bind::Bindable
	{
	public:
		ClipCbuf(Graphics& gfx, const ExecuteVictim_Shape* owner)
			:
			owner_(owner),
			buf_(gfx, ClipData{}, 1u)
		{}

		void Bind(Graphics& gfx) noxnd override
		{
			buf_.Update(gfx, owner_->GetClip());
			buf_.Bind(gfx);
		}

	private:
		// 正在画的那只副本。
		const ExecuteVictim_Shape* owner_;
		// 这一只副本自己的像素常量。不和别的物体共用。
		Bind::PixelConstantBuffer<ClipData> buf_;
	};

	// 清掉记下的网格和输入布局。
	void ClearMesh_() noexcept
	{
		pVertices.reset();
		pIndices.reset();
		pTopology.reset();
		if (layoutSlot_ != nullptr)
		{
			layoutSlot_->Set(nullptr);
		}
	}

	// 记下网格时用来生成输入布局。指向窗口的图形设备。
	Graphics* gfx_{ nullptr };
	// ModelClip 的顶点着色器。输入布局要和它配对。
	std::shared_ptr<Bind::VertexShader> vs_;
	// 步骤里唯一的输入布局槽。
	std::shared_ptr<LayoutSlot> layoutSlot_;
	// 这一帧的裁切球，以及记下的颜色。
	ClipData clip_{};
};

static_assert(sizeof(ExecuteVictim_Shape::ClipData) == 32u);

// fullEdges 是三条整边。返回最长的一条。
inline float ExecuteSphereRadius(XMFLOAT3 fullEdges) noexcept
{
	const float longest = fullEdges.x > fullEdges.y ? fullEdges.x : fullEdges.y;
	return longest > fullEdges.z ? longest : fullEdges.z;
}

// 碰撞体最长的整条尺寸。盒子是三条整边的最大值，球是直径，胶囊是总高度和直径里较长的。
// 盒子的 GetCollisionSize 是半边长，这里先乘 2。包住的是碰撞体；模型要在碰撞体里面。
inline float ExecuteSphereRadius(const ColliderComponentBase& collider) noexcept
{
	switch (collider.GetCollideType())
	{
	case Collider3D::CollideType::Box:
	{
		const auto& box = static_cast<const Collider3D::BoxCollider&>(collider.GetVolume());
		return ExecuteSphereRadius(XMFLOAT3{ box.half.x * 2.0f, box.half.y * 2.0f, box.half.z * 2.0f });
	}
	case Collider3D::CollideType::Sphere:
	{
		const auto& sphere = static_cast<const Collider3D::SphereCollider&>(collider.GetVolume());
		return sphere.radius * 2.0f;
	}
	case Collider3D::CollideType::Capsule:
	{
		const auto& capsule = static_cast<const Collider3D::CapsuleCollider&>(collider.GetVolume());
		const float diameter = capsule.radius * 2.0f;
		const DirectX::XMVECTOR span = DirectX::XMLoadFloat3(&capsule.pointB) - DirectX::XMLoadFloat3(&capsule.pointA);
		const float totalHeight = DirectX::XMVectorGetX(DirectX::XMVector3Length(span)) + diameter;
		return totalHeight > diameter ? totalHeight : diameter;
	}
	default:
		return 0.0f;
	}
}

// 秒杀用的透明黑球。黑球负责扩大、抖动和缩小。
// 传入外形时，另放一只不跟着抖、也不跟着变大的副本，按当前球裁切。
class ExecuteSphere : public EffectBase
{
public:
	ExecuteSphere(Graphics& gfx, Rgph::RenderGraph& rg)
		:
		EffectBase(effect_ExecuteSphere)
	{
		sphere_ = std::make_unique<ExecuteSphere_Shape>(gfx);
		sphere_->LinkTechniques(rg);
		victim_ = std::make_unique<ExecuteVictim_Shape>(gfx);
		victim_->LinkTechniques(rg);
	}

	// 不带半径的播放。flip 不用。半径用 kDefaultRadius_，方便单独看。
	void SpawnAt(XMFLOAT3 pos, bool flip) override
	{
		(void)flip;
		Play(pos, kDefaultRadius_);
	}

	// pos 是停下时的球心。maxRadius 是外接球半径。不是正数时不播放。
	// 不传来源外形。黑球和步骤 11 一样，只画透明黑球。
	void Play(XMFLOAT3 pos, float maxRadius)
	{
		Play(pos, maxRadius, nullptr);
	}

	// source 是被处决的外形。副本记下它的网格、颜色、位置、旋转和缩放，之后不再跟着它变。
	void Play(XMFLOAT3 pos, float maxRadius, const Drawable* source)
	{
		origin_ = pos;
		age_ = 0.0f;
		maxRadius_ = maxRadius;
		if (victim_ != nullptr)
		{
			victim_->Adopt(source);
		}
		if (maxRadius_ <= 0.0f)
		{
			RequestDisable();
		}
	}

	void OnEnable(void) override
	{
		age_ = 0.0f;
		if (victim_ != nullptr)
		{
			victim_->Adopt(nullptr);
		}
	}

	void OnCollide(CharacterBase* other) override
	{
		// 黑球不挡碰撞。
		(void)other;
	}

	void Update(float dt) override
	{
		age_ += dt;
		float radius = 0.0f;
		DirectX::XMFLOAT3 pos{};
		if (!Pose_(radius, pos))
		{
			RequestDisable();
		}
		ObjectBase::Update(dt);
	}

	void Submit(void) override
	{
		float radius = 0.0f;
		DirectX::XMFLOAT3 pos{};
		if (Pose_(radius, pos) && radius > 0.0f)
		{
			if (sphere_ != nullptr)
			{
				sphere_->SetPosition(pos);
				sphere_->SetScale({ radius, radius, radius });
				sphere_->Submit(Chan::main);
			}
			// 副本停在记下的位置和朝向。裁切球心和半径跟当前黑球走。
			if (victim_ != nullptr && victim_->HasMesh())
			{
				victim_->SetClip(pos, radius);
				victim_->Submit(Chan::main);
			}
		}
		ObjectBase::Submit();
	}

private:
	// u 是这一段抖动从 0 到 1 的进度。球心绕原点在水平面上转两圈，半径是 kShakeOffset_。
	DirectX::XMFLOAT3 Shaken_(float u) const
	{
		const float angle = u * 6.2831853f * kShakeTurns_;
		DirectX::XMFLOAT3 pos = origin_;
		pos.x += kShakeOffset_ * std::sin(angle);
		pos.z += kShakeOffset_ * std::cos(angle);
		return pos;
	}

	// 写出这一帧的世界半径和球心。播完返回 false。
	// 第一次就停在外接球半径上。第二次到 1.25 倍，第三次到 1.5 倍。
	// 每次用 kShakeSeconds_ 抖动。前两次之后各停 kInterval_。然后用 kShrinkSeconds_ 从 1.5 倍缩到 0。
	bool Pose_(float& outRadius, DirectX::XMFLOAT3& outPos) const
	{
		if (maxRadius_ <= 0.0f)
		{
			return false;
		}

		float cursor = age_;
		for (int step = 0; step < 3; ++step)
		{
			const float to = maxRadius_ * kStageScale_[step];
			const float from = step == 0 ? to : maxRadius_ * kStageScale_[step - 1];
			if (cursor < kShakeSeconds_)
			{
				const float u = cursor / kShakeSeconds_;
				outRadius = from + (to - from) * u;
				outPos = Shaken_(u);
				return true;
			}
			cursor -= kShakeSeconds_;
			if (step < 2)
			{
				if (cursor < kInterval_)
				{
					outRadius = to;
					outPos = origin_;
					return true;
				}
				cursor -= kInterval_;
				continue;
			}
			if (cursor >= kShrinkSeconds_)
			{
				return false;
			}
			const float u = cursor / kShrinkSeconds_;
			outRadius = to * (1.0f - u);
			outPos = origin_;
			return true;
		}
		return false;
	}

	// 这一只黑球的网格。
	std::unique_ptr<ExecuteSphere_Shape> sphere_;
	// 停在记下那一帧的外形。没有网格时不提交。
	std::unique_ptr<ExecuteVictim_Shape> victim_;
	// 这次播放停下时的球心。抖动从这里偏移。
	DirectX::XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	// 传入的外接球半径。第一次就用它，后面按 kStageScale_ 放大。
	float maxRadius_{ 0.0f };
	// 从 Play 或 SpawnAt 起算的秒数。
	float age_{ 0.0f };

	// 没传入半径时用的外接球半径。
	static constexpr float kDefaultRadius_{ 2.0f };
	// 三次停住时相对外接球半径的倍数。第一次就是外接球，然后 1.25、2.0。
	static constexpr float kStageScale_[3]{ 1.0f, 1.25f, 2.0f };
	// 一次扩大和伴随抖动的秒数。
	static constexpr float kShakeSeconds_{ 0.05f };
	// 前两次扩大之后停住的秒数。
	static constexpr float kInterval_{ 0.05f };
	// 从最大半径匀速缩到 0 的秒数。
	static constexpr float kShrinkSeconds_{ 0.4f };
	// 抖动时球心离开原点的水平距离。
	static constexpr float kShakeOffset_{ 0.12f };
	// 0.05 秒里绕圈的次数。
	static constexpr float kShakeTurns_{ 2.0f };
};
