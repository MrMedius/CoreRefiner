#pragma once
#include "Graphics.h"
#include <vector>
#include "Technique.h"
#include "DynamicVertex.h"
#include <filesystem>

struct aiMaterial;
struct aiMesh;
struct aiScene;
struct aiTexture;

namespace Bind
{
	class VertexBuffer;
	class IndexBuffer;
}

class Material
{
public:
	Material(Graphics& gfx, const aiScene* pScene, const aiMaterial& material, const std::filesystem::path& path) noxnd;
	Dvtx::VertexBuffer ExtractVertices( const aiMesh& mesh ) const noexcept;
	std::vector<unsigned short> ExtractIndices(const aiMesh& mesh) const noexcept;
	std::shared_ptr<Bind::VertexBuffer> MakeVertexBindable(Graphics& gfx, const aiMesh& mesh, float scale = 1.0f) const noxnd;
	std::shared_ptr<Bind::IndexBuffer> MakeIndexBindable(Graphics& gfx, const aiMesh& mesh) const noxnd;
	std::vector<Technique> GetTechniques() const noexcept;
private:
	std::string MakeMeshTag(const aiMesh& mesh) const noexcept;
	static const aiTexture* GetEmbeddedTexture(const std::string& texPath, const aiScene* pScene) noexcept;
private:
	Dvtx::VertexLayout vtxLayout;
	std::vector<Technique> techniques;
	std::string modelPath;
	std::string name;
};