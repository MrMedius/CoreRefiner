#pragma once
#include "Graphics.h"
#include <DirectXMath.h>
#include "ConditionalNoexcept.h"
#include <memory>
#include "Technique.h"
#include "Transformation.h"

class TechniqueProbe;
class Material;
struct aiMesh;
namespace Rgph
{
	class RenderGraph;
}

namespace Bind
{
	class IndexBuffer;
	class VertexBuffer;
	class Topology;
	class InputLayout;
}

class Drawable
{
public:
	Drawable() = default;
	Drawable(Graphics& gfx, const Material& mat, const aiMesh& mesh, float scale = 1.0f) noexcept;
	void AddTechnique(Technique tech_in) noexcept;
	virtual DirectX::XMMATRIX GetTransformXM() const noexcept = 0;
	void Submit(size_t channelFilter) const noexcept;
	void Bind(Graphics& gfx) const noxnd;
	void Accept(TechniqueProbe& probe);
	UINT GetIndexCount() const noxnd;
	// 这只外形是否一次画出多颗。默认否，仍走 DrawIndexed。
	virtual bool IsInstanced() const noexcept { return false; }
	// 这一帧要画的颗数。不是实例外形时不用。
	virtual UINT GetInstanceCount() const noexcept { return 0u; }
	void LinkTechniques(Rgph::RenderGraph&);
	virtual ~Drawable();
	void SetPosition(DirectX::XMFLOAT3 pos) noexcept;
	void SetRotation(float roll, float pitch, float yaw) noexcept;
	// 直接写入弧度。x 是 pitch，y 是 yaw，z 是 roll，不再从角度换算。
	void SetRotationRad(float x, float y, float z) noexcept;
	void SetScale(DirectX::XMFLOAT3 pos) noexcept;
	// 交出 Codex 里已有的网格。没有网格时是空指针。
	[[nodiscard]] std::shared_ptr<Bind::VertexBuffer> ShareVertices() const noexcept;
	[[nodiscard]] std::shared_ptr<Bind::IndexBuffer> ShareIndices() const noexcept;
	[[nodiscard]] std::shared_ptr<Bind::Topology> ShareTopology() const noexcept;
	// 这只外形当前的位置。
	[[nodiscard]] DirectX::XMFLOAT3 GetPosition() const noexcept;
	// 这只外形当前的旋转，分量原样返回。
	[[nodiscard]] DirectX::XMFLOAT3 GetRotationRaw() const noexcept;
	// 这只外形当前的缩放。
	[[nodiscard]] DirectX::XMFLOAT3 GetScale() const noexcept;
	// 写出这只外形自己的颜色。没有单独颜色时返回 false，不改 out。
	virtual bool GetMaterialColor(DirectX::XMFLOAT4& out) const noexcept
	{
		(void)out;
		return false;
	}
protected:
	std::shared_ptr<Bind::IndexBuffer> pIndices;
	std::shared_ptr<Bind::VertexBuffer> pVertices;
	std::shared_ptr<Bind::Topology> pTopology;
	std::vector<Technique> techniques;
	Transformation trans;
};