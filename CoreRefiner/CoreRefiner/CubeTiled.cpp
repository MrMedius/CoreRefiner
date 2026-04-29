#include "CubeTiled.h"
#include "IndexedTriangleList.h"
#include "DynamicVertex.h"
#include "BindableCommon.h"
#include "ConstantBuffersEx.h"
#include "FieldTransitionCbuf.h"
#include "Channels.h"

using dvL = Dvtx::VertexLayout;

CubeTiled::CubeTiled(Graphics& gfx, XMFLOAT3 size, XMFLOAT2 numTiled)
{
	using namespace Bind;

	const auto geometryTag = "$cube." + std::to_string(numTiled.x);
	Dvtx::VertexLayout layout;
	layout.Append(dvL::Position3D).
		   Append(dvL::Normal).
		   Append(dvL::Texture2D);
	
	Dvtx::VertexBuffer vertices{ std::move(layout) };

	vertices.Resize(24u);
	{
		float side = 0.5f;

		// upper face
		vertices[ 0].Attr<dvL::Position3D>() = XMFLOAT3{ -side,  side,  side }; vertices[ 0].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		0.0f };
		vertices[ 1].Attr<dvL::Position3D>() = XMFLOAT3{  side,  side,  side }; vertices[ 1].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,	0.0f };
		vertices[ 2].Attr<dvL::Position3D>() = XMFLOAT3{ -side,  side, -side }; vertices[ 2].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		numTiled.y };
		vertices[ 3].Attr<dvL::Position3D>() = XMFLOAT3{  side,  side, -side }; vertices[ 3].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x, numTiled.y };
		// front face																		 
		vertices[ 4].Attr<dvL::Position3D>() = XMFLOAT3{ -side,  side, -side }; vertices[ 4].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		0.0f };
		vertices[ 5].Attr<dvL::Position3D>() = XMFLOAT3{  side,  side, -side }; vertices[ 5].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,	0.0f };
		vertices[ 6].Attr<dvL::Position3D>() = XMFLOAT3{ -side, -side, -side }; vertices[ 6].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		1.0f };
		vertices[ 7].Attr<dvL::Position3D>() = XMFLOAT3{  side, -side, -side }; vertices[ 7].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,	1.0f };
		// down face																		 
		vertices[ 8].Attr<dvL::Position3D>() = XMFLOAT3{ -side, -side, -side }; vertices[ 8].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		0.0f };
		vertices[ 9].Attr<dvL::Position3D>() = XMFLOAT3{  side, -side, -side }; vertices[ 9].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,	0.0f };
		vertices[10].Attr<dvL::Position3D>() = XMFLOAT3{ -side, -side,  side }; vertices[10].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		1.0f };
		vertices[11].Attr<dvL::Position3D>() = XMFLOAT3{  side, -side,  side }; vertices[11].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,	1.0f };
		// back face																							    
		vertices[12].Attr<dvL::Position3D>() = XMFLOAT3{  side,  side,  side }; vertices[12].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		0.0f };
		vertices[13].Attr<dvL::Position3D>() = XMFLOAT3{ -side,  side,  side }; vertices[13].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,	0.0f };
		vertices[14].Attr<dvL::Position3D>() = XMFLOAT3{  side, -side,  side }; vertices[14].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		numTiled.y };
		vertices[15].Attr<dvL::Position3D>() = XMFLOAT3{ -side, -side,  side }; vertices[15].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.x,	numTiled.y };
		// left face																		 
		vertices[16].Attr<dvL::Position3D>() = XMFLOAT3{ -side,  side,  side }; vertices[16].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		0.0f };
		vertices[17].Attr<dvL::Position3D>() = XMFLOAT3{ -side,  side, -side }; vertices[17].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.y,	0.0f };
		vertices[18].Attr<dvL::Position3D>() = XMFLOAT3{ -side, -side,  side }; vertices[18].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		1.0f };
		vertices[19].Attr<dvL::Position3D>() = XMFLOAT3{ -side, -side, -side }; vertices[19].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.y,	1.0f };
		// right face																		 
		vertices[20].Attr<dvL::Position3D>() = XMFLOAT3{  side,  side, -side }; vertices[20].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		0.0f };
		vertices[21].Attr<dvL::Position3D>() = XMFLOAT3{  side,  side,  side }; vertices[21].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.y,	0.0f };
		vertices[22].Attr<dvL::Position3D>() = XMFLOAT3{  side, -side, -side }; vertices[22].Attr<dvL::Texture2D>() = XMFLOAT2{	0.0f,		1.0f };
		vertices[23].Attr<dvL::Position3D>() = XMFLOAT3{  side, -side,  side }; vertices[23].Attr<dvL::Texture2D>() = XMFLOAT2{ numTiled.y,	1.0f };
	}

	std::vector<unsigned short> indices;
	{
		for (int i = 0;i < 6;i++)
		{
			indices.push_back(i * 4 + 0);
			indices.push_back(i * 4 + 1);
			indices.push_back(i * 4 + 2);
			indices.push_back(i * 4 + 1);
			indices.push_back(i * 4 + 3);
			indices.push_back(i * 4 + 2);
		}
	}
	IndexedTriangleList model(vertices, indices);
	model.Transform(XMMatrixScaling(size.x, size.y, size.z));
	model.SetNormalsIndependentFlat();

	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tcb = std::make_shared<TransformCbuf>(gfx);
	auto ftcb = std::make_shared<FieldTransitionCbuf>(gfx);

	{
		Technique shade("Shade", Chan::main);
		{
			Step top("lambertianTrans");

			top.AddBindable(Sampler::Resolve(gfx));

			auto pvs = VertexShader::Resolve(gfx, "CubeTiled_VS.cso");
			top.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			top.AddBindable(std::move(pvs));

			top.AddBindable(PixelShader::Resolve(gfx, "CubeTiled_PS.cso"));
			Dcb::RawLayout lay;
			lay.Add<Dcb::Float3>("specularColor");
			lay.Add<Dcb::Float>("specularWeight");
			lay.Add<Dcb::Float>("specularGloss");
			auto buf = Dcb::Buffer(std::move(lay));
			buf["specularColor"] = XMFLOAT3{ 1.0f,1.0f,1.0f };
			buf["specularWeight"] = 0.1f;
			buf["specularGloss"] = 20.0f;
			top.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEX>(gfx, buf, 1u));

			top.AddBindable(Rasterizer::Resolve(gfx, false));
			top.AddBindable(Blender::Resolve(gfx, true));

			top.AddBindable(tcb);
			top.AddBindable(ftcb);
			top.AddBindable(Texture::Resolve(gfx, "asset\\Images\\Environment\\block_noise_cloud.png", 1u));

			top.AddBindable(Texture::Resolve(gfx, "asset\\Images\\Environment\\block_field.png"));
			shade.AddStep(std::move(top));


			//// separate top and body to allow different textures
			//Step body = top;

			//// top face
			//top.AddBindable(Texture::Resolve(gfx, "asset\\Images\\block_field_top.png"));
			//top.SetDrawIndexed(6u, 0u);
			//shade.AddStep(std::move(top));

			//// body faces
			//body.AddBindable(Texture::Resolve(gfx, "asset\\Images\\block_field.png"));
			//body.SetDrawIndexed(30u, 6u);
			//shade.AddStep(std::move(body));
		}
		AddTechnique(std::move(shade));
	}
	{
		Technique outline("Outline", Chan::main);
		{
			Step mask("outlineMask");

			mask.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *VertexShader::Resolve(gfx, "Solid_VS.cso")));
			mask.AddBindable(NullPixelShader::Resolve(gfx));

			mask.AddBindable(std::move(tcb));

			outline.AddStep(std::move(mask));
		}
		{
			Step draw("outlineDraw");

			Dcb::RawLayout lay;
			lay.Add<Dcb::Float4>("color");
			auto buf = Dcb::Buffer(std::move(lay));
			buf["color"] = XMFLOAT4{ 4.0f,0.4f,0.4f,1.0f };
			draw.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEX>(gfx, buf, 1u));

			draw.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *VertexShader::Resolve(gfx, "Solid_VS.cso")));

			draw.AddBindable(std::make_shared<TransformCbuf>(gfx));

			outline.AddStep(std::move(draw));
		}
		AddTechnique(std::move(outline));
		// shadow map technique
		//{
		//	Technique map{ "ShadowMap",Chan::shadow,true };
		//	{
		//		Step draw("shadowMap");

		//		auto pvs = VertexShader::Resolve(gfx, "Solid_VS.cso");
		//		draw.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
		//		draw.AddBindable(std::move(pvs));

		//		draw.AddBindable(NullPixelShader::Resolve(gfx));

		//		draw.AddBindable(std::make_shared<TransformCbuf>(gfx));

		//		map.AddStep(std::move(draw));
		//	}
		//	AddTechnique(std::move(map));
		//}
	}
}

void CubeTiled::SetPosition(XMFLOAT3 pos) noexcept
{
	trans.SetPosition(pos.x, pos.y, pos.z);
}

void CubeTiled::SetRotation(float roll, float pitch, float yaw) noexcept
{
	trans.SetRotationDegreeToRad(roll, pitch, yaw);
}

void CubeTiled::SetScale(XMFLOAT3 size) noexcept
{
	trans.SetScale(size.x, size.y, size.z);
}

XMMATRIX CubeTiled::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}


// transition related
void CubeTiled::Update(float dt, const XMFLOAT3& centerWorld)
{
	switch (FT.state)
	{
	case FieldTransitionState::None:
		break;

	case FieldTransitionState::Expand:
		FT.t += dt * FT.interval;
		FT.t = (FT.t > 1.0f) ? 1.0f : FT.t;

		FT.radius = FT.t * FT.maxRadius;

		if (FT.t >= 1.0f)
		{
			// start Recover automatically after Expend
			StartRecover(FT.mode);
		}
		break;

	case FieldTransitionState::Recover:
		FT.center = centerWorld;

		FT.t -= dt * FT.interval;
		FT.t = (FT.t < 0.0f) ? 0.0f : FT.t;

		FT.radius = FT.t * FT.maxRadius;

		if (FT.t <= 0.0f)
		{
			FinishRecover();
			FT.colorTo = FT.colorBase;
		}
		break;
	}
}

void CubeTiled::StartExpand(const XMFLOAT3& centerWorld, int mode)
{
	FT.colorFrom = FT.colorTo;
	switch (mode)
	{
	case 0: FT.colorTo = FT.colorBase; break;
	case 1: FT.colorTo = XMFLOAT3{ 0.75f,0.0f,0.30f }; break;
	case 2: FT.colorTo = XMFLOAT3{ 0.0f,0.75f,0.45f }; break;
	case 3: FT.colorTo = XMFLOAT3{ 0.0f,0.30f,0.75f }; break;
	}
	// core
	FT.state = FieldTransitionState::Expand;
	FT.mode = mode;
	// animation control
	FT.center = centerWorld;
	FT.t = 0.0f;
	FT.maxRadius = FT.effectRange;
	// shader params
	FT.radius = 0.0f;
}


void CubeTiled::StartRecover(int mode)
{
	FT.colorFrom = FT.colorBase;
	// core
	FT.state = FieldTransitionState::Recover;
	FT.mode = mode;
	// animation control
	FT.maxRadius = FT.effectRange;
	FT.t = 1.0f;
	// shader params
	FT.radius = FT.maxRadius;
}

void CubeTiled::FinishRecover()
{
	FT.state = FieldTransitionState::None;
	FT.mode = 0;
}

void CubeTiled::GameStartExpend()
{
	FT.colorTo = { 0.0f,0.0f,0.0f };
	StartExpand({ 0.0f,0.0f,0.0f }, 0);
}