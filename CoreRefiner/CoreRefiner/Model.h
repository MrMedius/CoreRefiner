#pragma once
#include "Graphics.h"
#include <string>
#include <memory>
#include <filesystem>
#include "Transformation.h"

class Node;
class Mesh;
struct aiMesh;
struct aiMaterial;
struct aiNode;
namespace Rgph
{
	class RenderGraph;
}

class Model
{
public:
	Model(Graphics& gfx, const std::string& pathString, float scale = 1.0f);
	void Update(float dt) noexcept;
	void Submit(size_t channels) const noxnd;
	void SetRootTransform(DirectX::FXMMATRIX tf) noexcept;
	void Accept(class ModelProbe& probe);
	void LinkTechniques(Rgph::RenderGraph&);
	~Model() noexcept;
private:
	static std::unique_ptr<Mesh> ParseMesh(Graphics& gfx, const aiMesh& mesh, const aiMaterial* const* pMaterials, const std::filesystem::path& path, float scale);
	std::unique_ptr<Node> ParseNode(int& nextId, const aiNode& node, float scale) noexcept;
private:
	std::unique_ptr<Node> pRoot;
	std::vector<std::unique_ptr<Mesh>> meshPtrs;

public:
	void Translate(float x, float y, float z) noexcept;
	void SetPosition(float x, float y, float z) noexcept;
	void Rotate(float x, float y, float z) noexcept;
	void SetRotation(float x, float y, float z) noexcept;
private:
	Transformation trans;
};