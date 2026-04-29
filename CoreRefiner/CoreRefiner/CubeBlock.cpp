#include "CubeBlock.h"
#include "Cube.h"
#include "BindableCommon.h"
#include "ConstantBuffersEx.h"
#include "imgui/imgui.h"
#include "Stencil.h"
#include "DynamicConstant.h"
#include "Channels.h"

CubeLock::CubeLock(Graphics& gfx, DirectX::XMFLOAT3 size)
{
	using namespace Bind;
	namespace dx = DirectX;

	auto model = Cube::MakeIndependentBlockTextured();
	model.Transform(dx::XMMatrixScaling(size.x, size.y, size.z));
	model.SetNormalsIndependentFlat();
	const auto geometryTag = "$cube." + std::to_string(size.x);
	pVertices = VertexBuffer::Resolve(gfx, geometryTag, model.vertices);
	pIndices = IndexBuffer::Resolve(gfx, geometryTag, model.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto tcb = std::make_shared<TransformCbuf>(gfx);

	{
		Technique shade("Shade", Chan::main);
		{
			Step only("lambertian");

			only.AddBindable(Texture::Resolve(gfx, "asset\\Images\\block_field.png"));
			only.AddBindable(Sampler::Resolve(gfx));

			auto pvs = VertexShader::Resolve(gfx, "General_VS.cso");
			only.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
			only.AddBindable(std::move(pvs));

			only.AddBindable(PixelShader::Resolve(gfx, "General_PS.cso"));

			Dcb::RawLayout lay;
			lay.Add<Dcb::Float3>("specularColor");
			lay.Add<Dcb::Float>("specularWeight");
			lay.Add<Dcb::Float>("specularGloss");
			auto buf = Dcb::Buffer(std::move(lay));
			buf["specularColor"] = dx::XMFLOAT3{ 1.0f,1.0f,1.0f };
			buf["specularWeight"] = 0.1f;
			buf["specularGloss"] = 20.0f;
			only.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEX>(gfx, buf, 1u));

			only.AddBindable(Rasterizer::Resolve(gfx, false));

			only.AddBindable(tcb);

			shade.AddStep(std::move(only));
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
			buf["color"] = DirectX::XMFLOAT4{ 4.0f,0.4f,0.4f,1.0f };
			draw.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEX>(gfx, buf, 1u));

			draw.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *VertexShader::Resolve(gfx, "Solid_VS.cso")));

			draw.AddBindable(std::make_shared<TransformCbuf>(gfx));

			outline.AddStep(std::move(draw));
		}
		AddTechnique(std::move(outline));
		// shadow map technique
		{
			Technique map{ "ShadowMap",Chan::shadow,true };
			{
				Step draw("shadowMap");

				auto pvs = VertexShader::Resolve(gfx, "Solid_VS.cso");
				draw.AddBindable(InputLayout::Resolve(gfx, model.vertices.GetLayout(), *pvs));
				draw.AddBindable(std::move(pvs));

				draw.AddBindable(NullPixelShader::Resolve(gfx));

				draw.AddBindable(std::make_shared<TransformCbuf>(gfx));

				map.AddStep(std::move(draw));
			}
			AddTechnique(std::move(map));
		}
	}
}

void CubeLock::Update(float dt)
{
	trans.RotateDegreeToRad(1.0f, 1.0f, 1.0f);
}

void CubeLock::SetPosition(DirectX::XMFLOAT3 pos) noexcept
{
	trans.SetPosition(pos.x, pos.y, pos.z);
}

void CubeLock::SetRotation(float roll, float pitch, float yaw) noexcept
{
	trans.SetRotationDegreeToRad(roll, pitch, yaw);
}

void CubeLock::SetScale(DirectX::XMFLOAT3 size) noexcept
{
	trans.SetScale(size.x, size.y, size.z);
}

DirectX::XMMATRIX CubeLock::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}

void CubeLock::SpawnControlWindow(Graphics& gfx, const char* name) noexcept
{
	if (ImGui::Begin(name))
	{
		auto translation = trans.GetPosition();
		auto angles = trans.GetRotation();
		auto scales = trans.GetScale();

		bool dirty = false;
		ImGui::Text("Position");
		dirty |= ImGui::SliderFloat("X", &translation.x, -80.0f, 80.0f, "%.1f");
		dirty |= ImGui::SliderFloat("Y", &translation.y, -80.0f, 80.0f, "%.1f");
		dirty |= ImGui::SliderFloat("Z", &translation.z, -80.0f, 80.0f, "%.1f");
		ImGui::Text("Orientation");
		dirty |= ImGui::SliderAngle("Roll", &angles.x, -180.0f, 180.0f);
		dirty |= ImGui::SliderAngle("Pitch", &angles.y, -180.0f, 180.0f);
		dirty |= ImGui::SliderAngle("Yaw", &angles.z, -180.0f, 180.0f);
		ImGui::Text("Scale");
		dirty |= ImGui::SliderFloat("ScaleX", &scales.x, 1.0f, 100.0f);
		dirty |= ImGui::SliderFloat("ScaleY", &scales.y, 1.0f, 100.0f);
		dirty |= ImGui::SliderFloat("ScaleZ", &scales.z, 1.0f, 100.0f);
		if (dirty)
		{
			trans.SetPosition(translation.x, translation.y, translation.z);
			trans.SetRotationRad(angles.x, angles.y, angles.z);
			trans.SetScale(scales.x, scales.y, scales.z);
		}
		
		class Probe : public TechniqueProbe
		{
		public:
			void OnSetTechnique() override
			{
				using namespace std::string_literals;
				ImGui::TextColored({ 0.4f,1.0f,0.6f,1.0f }, pTech->GetName().c_str());
				bool active = pTech->IsActive();
				ImGui::Checkbox(("Tech Active##"s + std::to_string(techIdx)).c_str(), &active);
				if(pTech->IsActive()!= active)
					pTech->SetActiveState(active);
			}
			bool OnVisitBuffer(Dcb::Buffer& buf) override 
			{
				namespace dx = DirectX;
				float dirty = false;
				const auto dcheck = [&dirty](bool changed) {dirty = dirty || changed;};
				auto tag = [tagScratch = std::string{}, tagString = "##" + std::to_string(bufIdx)]
				(const char* label) mutable
					{
						tagScratch = label + tagString;
						return tagScratch.c_str();
					};

				if (auto v = buf["scale"]; v.Exists())
				{
					dcheck(ImGui::SliderFloat(tag("Scale"), &v, 1.0f, 2.0f, "%.3f", 3.5f));
				}
				if (auto v = buf["color"]; v.Exists())
				{
					dcheck(ImGui::ColorPicker4(tag("Color"), reinterpret_cast<float*>(&static_cast<dx::XMFLOAT4&>(v))));
				}
				if (auto v = buf["specularIntensity"]; v.Exists())
				{
					dcheck(ImGui::SliderFloat(tag("Spec. Intens."), &v, 0.0f, 1.0f));
				}
				if (auto v = buf["specularPower"]; v.Exists())
				{
					dcheck(ImGui::SliderFloat(tag("Glossiness"), &v, 1.0f, 100.0f, "%.1f", 1.5f));
				}
				return dirty;
			}
		} probe;

		Accept(probe);
	}
	ImGui::End();
}