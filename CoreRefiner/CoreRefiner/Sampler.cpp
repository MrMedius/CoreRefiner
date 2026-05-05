#include "Sampler.h"
#include "GraphicsThrowMacros.h"
#include "BindableCodex.h"

namespace Bind
{
	static D3D11_FILTER ToFilter(Sampler::Type type)
	{
		switch (type)
		{
		case Sampler::Type::Anisotropic: return D3D11_FILTER_ANISOTROPIC;
		case Sampler::Type::Point:       return D3D11_FILTER_MIN_MAG_MIP_POINT;
		default:
		case Sampler::Type::Bilinear:    return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		}
	}

	static D3D11_TEXTURE_ADDRESS_MODE ToAddress(Sampler::Address address)
	{
		switch (address)
		{
		case Sampler::Address::Mirror: return D3D11_TEXTURE_ADDRESS_MIRROR;
		case Sampler::Address::Clamp:  return D3D11_TEXTURE_ADDRESS_CLAMP;
		default:
		case Sampler::Address::Wrap:   return D3D11_TEXTURE_ADDRESS_WRAP;
		}
	}

	Sampler::Sampler(Graphics& gfx, Type type, Address address, UINT slot)
		:
		type(type),
		address(address),
		slot(slot)
	{
		INFOMAN(gfx);

		D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC{ CD3D11_DEFAULT{} };
		samplerDesc.Filter = ToFilter(type);
		samplerDesc.AddressU = ToAddress(address);
		samplerDesc.AddressV = ToAddress(address);
		samplerDesc.AddressW = ToAddress(address);
		samplerDesc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;

		GFX_THROW_INFO(GetDevice(gfx)->CreateSamplerState(&samplerDesc, &pSampler));
	}

	Sampler::Sampler(Graphics& gfx, Type type, bool reflect, UINT slot)
		:
		Sampler(gfx, type, reflect ? Address::Mirror : Address::Wrap, slot)
	{}

	void Sampler::Bind(Graphics& gfx) noxnd
	{
		INFOMAN_NOHR(gfx);
		GFX_THROW_INFO_ONLY(GetContext(gfx)->PSSetSamplers(slot, 1, pSampler.GetAddressOf()));
	}

	std::shared_ptr<Sampler> Sampler::Resolve(Graphics& gfx, Type type, Address address, UINT slot)
	{
		return Codex::Resolve<Sampler>(gfx, type, address, slot);
	}

	std::shared_ptr<Sampler> Sampler::Resolve(Graphics& gfx, Type type, bool reflect, UINT slot)
	{
		return Resolve(gfx, type, reflect ? Address::Mirror : Address::Wrap, slot);
	}

	std::string Sampler::GenerateUID(Type type, Address address, UINT slot)
	{
		using namespace std::string_literals;
		return typeid(Sampler).name() + "#"s
			+ std::to_string((int)type) + "@" + std::to_string((int)address)
			+ "@" + std::to_string(slot);
	}

	std::string Sampler::GenerateUID(Type type, bool reflect, UINT slot)
	{
		return GenerateUID(type, reflect ? Address::Mirror : Address::Wrap, slot);
	}

	std::string Sampler::GetUID() const noexcept
	{
		return GenerateUID(type, address, slot);
	}
}