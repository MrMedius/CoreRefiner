#pragma once
#include "EffectBase.h"
#include "Canvas3D.h"
#include "Plane.h"
#include "BindableCommon.h"
#include "CanvasTexture.h"
#include "Channels.h"
#include "RenderGraph.h"
#include "GraphicsThrowMacros.h"

#include <cmath>
#include <cstring>
#include <memory>

namespace Bind
{
	// 顶点着色器每帧要的宿主位置、飞行方向、颗数，以及朝向镜头用的摄像机轴向。
	// 时间不在这里，顶点着色器读 Time_VS.hlsli。
	class TrailStateCbuf : public Bindable
	{
	public:
		explicit TrailStateCbuf(Graphics& gfx)
		{
			INFOMAN(gfx);
			D3D11_BUFFER_DESC bd = {};
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bd.ByteWidth = sizeof(State);
			GFX_THROW_INFO(GetDevice(gfx)->CreateBuffer(&bd, nullptr, &buffer_));
		}

		// hostPos 是宿主当前位置。direction 是单位飞行方向。count 是这一帧要画的颗数。
		// travel 是这一发已经飞过的距离。着色器用它决定拖尾拉到多长。
		void SetState(DirectX::XMFLOAT3 hostPos, DirectX::XMFLOAT3 direction, float count, float travel) noexcept
		{
			hostPos_ = hostPos;
			direction_ = direction;
			count_ = count;
			travel_ = travel;
		}

		void Bind(Graphics& gfx) noxnd override
		{
			INFOMAN_NOHR(gfx);
			const DirectX::XMMATRIX view = gfx.GetCamera();
			DirectX::XMFLOAT4X4 stored{};
			DirectX::XMStoreFloat4x4(&stored, view);
			State state{};
			state.count = count_;
			state.travel = travel_;
			state.hostPos = hostPos_;
			state.direction = direction_;
			DirectX::XMStoreFloat3(
				&state.camRight,
				DirectX::XMVector3Normalize(DirectX::XMVectorSet(stored._11, stored._21, stored._31, 0.0f)));
			DirectX::XMStoreFloat3(
				&state.camUp,
				DirectX::XMVector3Normalize(DirectX::XMVectorSet(stored._12, stored._22, stored._32, 0.0f)));

			D3D11_MAPPED_SUBRESOURCE mapped{};
			GFX_THROW_INFO_ONLY(GetContext(gfx)->Map(buffer_.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mapped));
			std::memcpy(mapped.pData, &state, sizeof(State));
			GetContext(gfx)->Unmap(buffer_.Get(), 0u);
			GFX_THROW_INFO_ONLY(GetContext(gfx)->VSSetConstantBuffers(kSlot_, 1u, buffer_.GetAddressOf()));
		}

	private:
		// 和着色器的 TrailState 逐项对齐。每个 float3 后面补一个 float。
		struct State
		{
			DirectX::XMFLOAT3 camRight{};
			float count{ 0.0f };
			DirectX::XMFLOAT3 camUp{};
			float travel{ 0.0f };
			DirectX::XMFLOAT3 hostPos{};
			float pad1{ 0.0f };
			DirectX::XMFLOAT3 direction{};
			float pad2{ 0.0f };
		};

		// 顶点着色器的 b1。b0 留给变换矩阵，b3 留给时间。
		static constexpr UINT kSlot_{ 1u };
		DirectX::XMFLOAT3 hostPos_{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 direction_{ 0.0f, 0.0f, 1.0f };
		float count_{ 0.0f };
		// 这一发已经飞过的距离。
		float travel_{ 0.0f };
		Microsoft::WRL::ComPtr<ID3D11Buffer> buffer_;
	};
}

// 太阴拖尾的外形。自己就是一张 Canvas，多边形由基类的表面画出来，再涂成黑。
// 每一颗的位置、边长和透明度在顶点着色器里按编号和年龄计算。
class AttackTrailShape : public Canvas3D
{
public:
	explicit AttackTrailShape(Graphics& gfx)
		:
		Canvas3D(gfx, kImageSize_, kImageSize_)
	{
		using namespace Bind;
		// 四边多边形，尖角朝上。实心先由 Canvas 填白，再改成不透明的黑。
		ApplyForm(Canvas::Polygon, 4.0f);
		PaintBlack_();
		// 丢掉基类刚装上的光照通道，避免和下面这条一起画。
		techniques.clear();

		// 只要位置和纹理坐标。拖尾着色器不读法线。
		auto model = Plane::Make2D(1, 1);
		const std::string geometryTag = "$attack.trail.canvas";
		pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
		pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
		pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto vs = VertexShader::Resolve(gfx, "AttackTrail_VS.cso");
		state_ = std::make_shared<TrailStateCbuf>(gfx);

		Technique shade("Shade", Chan::main);
		{
			Step only("lambertianTrans");
			only.AddBindable(std::make_shared<CanvasTexture>(gfx));
			only.AddBindable(Sampler::Resolve(gfx, Sampler::Type::Point, Sampler::Address::Clamp));
			only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *vs));
			only.AddBindable(vs);
			only.AddBindable(PixelShader::Resolve(gfx, "AttackTrail_PS.cso"));
			only.AddBindable(Rasterizer::Resolve(gfx, true));
			only.AddBindable(Blender::Resolve(gfx, true));
			only.AddBindable(std::make_shared<TransformCbuf>(gfx));
			only.AddBindable(std::make_shared<TimeCbuf_VS>(gfx));
			only.AddBindable(state_);
			shade.AddStep(std::move(only));
		}
		AddTechnique(std::move(shade));
		// 模型保持单位。粒子坐标在着色器里已经是世界坐标。
		SetPosition({ 0.0f, 0.0f, 0.0f });
		SetRotationRad(0.0f, 0.0f, 0.0f);
		SetScale({ 1.0f, 1.0f, 1.0f });
	}

	// hostPos、direction 交给着色器。颗数用 kCount_。travel 是这一发已经飞过的距离。
	void SetState(DirectX::XMFLOAT3 hostPos, DirectX::XMFLOAT3 direction, float travel) noexcept
	{
		if (state_ != nullptr)
		{
			state_->SetState(hostPos, direction, static_cast<float>(kCount_), travel);
		}
	}

	// 这只外形一次画出整串。
	bool IsInstanced() const noexcept override { return true; }

	// 一次画出的颗数，只由 kCount_ 决定。
	UINT GetInstanceCount() const noexcept override { return kCount_; }

private:
	// 实心像素改成不透明的黑。透明的外面不动，形状仍是多边形。
	void PaintBlack_()
	{
		const unsigned width = GetCanvasWidth();
		const unsigned height = GetCanvasHeight();
		for (unsigned y = 0u; y < height; ++y)
		{
			for (unsigned x = 0u; x < width; ++x)
			{
				if (GetPixel(x, y).GetA() != 0u)
				{
					PutPixel(x, y, Colors::Black);
				}
			}
		}
	}

	// 贴图边长，单位像素。
	static constexpr unsigned kImageSize_{ 32u };
	// 一次画出的颗数。
	static constexpr UINT kCount_{ 30u };
	// 宿主位置、飞行方向和颗数。
	std::shared_ptr<Bind::TrailStateCbuf> state_;
};

// 太阴的拖尾特效。只把宿主的位置和飞行方向交给外形，粒子怎么散开由着色器决定。
// 不碰撞，不交给特效管理器。对象池拥有它。宿主关闭时整串跟着关掉，回到池里。
class AttackTrail : public EffectBase
{
public:
	AttackTrail(Graphics& gfx, Rgph::RenderGraph& rg)
		:
		EffectBase(effect_AttackTrail)
	{
		shape_ = std::make_unique<AttackTrailShape>(gfx);
		shape_->LinkTechniques(rg);
	}

	// pos 是没有宿主时的位置。flip 不用。
	void SpawnAt(XMFLOAT3 pos, bool flip) override
	{
		(void)flip;
		origin_ = pos;
	}

	// 回收进池时卸下父子，并忘掉上一发的飞行方向。
	void OnEnable() override
	{
		ClearParent();
		origin_ = {};
		lastPos_ = {};
		direction_ = { 0.0f, 0.0f, 1.0f };
		travel_ = 0.0f;
		hasLast_ = false;
		hasDirection_ = false;
	}

	// 按宿主这一帧的位移更新飞行方向。
	void Update(float dt) override
	{
		(void)dt;
		Sync_();
		ObjectBase::Update(dt);
	}

	// 提交这一串，再提交还挂着的变换子物体。
	void Submit() override
	{
		Sync_();
		if (shape_ != nullptr)
		{
			shape_->Submit(Chan::main);
		}
		ObjectBase::Submit();
	}

	// 拖尾不挡碰撞。
	void OnCollide(CharacterBase* other) override
	{
		(void)other;
	}

private:
	// 有宿主就用宿主的位置和朝向。位移够长时改用实际飞行方向。
	void Sync_()
	{
		DirectX::XMFLOAT3 pos = origin_;
		DirectX::XMFLOAT3 facing = { 0.0f, 0.0f, 1.0f };
		if (const ObjectBase* host = GetParent())
		{
			pos = host->GetWorldPosition();
			facing = host->GetForward();
		}
		RefreshDirection_(pos, facing);
		if (shape_ != nullptr)
		{
			shape_->SetState(pos, direction_, travel_);
		}
	}

	// facing 是宿主朝向。还没飞起来时用它，飞起来之后改用位移方向。
	void RefreshDirection_(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 facing)
	{
		if (hasLast_)
		{
			const float dx = pos.x - lastPos_.x;
			const float dy = pos.y - lastPos_.y;
			const float dz = pos.z - lastPos_.z;
			const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (dist > 1e-4f)
			{
				const float inv = 1.0f / dist;
				direction_ = { dx * inv, dy * inv, dz * inv };
				hasDirection_ = true;
				travel_ += dist;
			}
		}
		if (!hasDirection_)
		{
			const float len = std::sqrt(facing.x * facing.x + facing.y * facing.y + facing.z * facing.z);
			if (len > 1e-4f)
			{
				const float inv = 1.0f / len;
				direction_ = { facing.x * inv, facing.y * inv, facing.z * inv };
			}
			hasDirection_ = true;
		}
		lastPos_ = pos;
		hasLast_ = true;
	}

	// 没有宿主时的位置。
	XMFLOAT3 origin_{ 0.0f, 0.0f, 0.0f };
	// 上一帧用来算位移的位置。
	XMFLOAT3 lastPos_{ 0.0f, 0.0f, 0.0f };
	// 单位飞行方向。着色器沿它的反方向把粒子铺开，并把菱形尖角转到这个方向。
	XMFLOAT3 direction_{ 0.0f, 0.0f, 1.0f };
	// 这一发已经飞过的距离。拖尾只拉到这里，不会一生出来就铺满。
	float travel_{ 0.0f };
	// 还没有上一帧位置时为假。
	bool hasLast_{ false };
	// 还没有算出飞行方向时为假。
	bool hasDirection_{ false };
	// 朝向镜头的那张黑色多边形 Canvas。整串共用它。
	std::unique_ptr<AttackTrailShape> shape_;
};
