#pragma once
#include "Win.h"
#include "ExceptionBase.h"
#include <d3d11.h>
#include "WRL.h"
#include <vector>
#include "DxgiInfoManager.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <memory>
#include <random>
#include "ConditionalNoexcept.h"

#define SCREEN_WIDTH	(1280)	
#define SCREEN_HEIGHT	(720)	

namespace Bind
{
	class Bindable;
	class Bindable_Ex;
	class RenderTarget;
	class DepthStencil;
}

class Graphics
{
	friend class GraphicsResource;
public:
	class Exception : public ExceptionBase
	{
		using ExceptionBase::ExceptionBase;
	};
	class HrException : public Exception
	{
	public:
		HrException( int line,const char* file,HRESULT hr,std::vector<std::string> infoMsgs = {} ) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorString() const noexcept;
		std::string GetErrorDescription() const noexcept;
		std::string GetErrorInfo() const noexcept;
	private:
		HRESULT hr;
		std::string info;
	};
	class InfoException : public Exception
	{
	public:
		InfoException( int line,const char* file,std::vector<std::string> infoMsgs ) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		std::string GetErrorInfo() const noexcept;
	private:
		std::string info;
	};
	class DeviceRemovedException : public HrException
	{
		using HrException::HrException;
	public:
		const char* GetType() const noexcept override;
	private:
		std::string reason;
	};
public:
	Graphics( HWND hWnd,int width,int height );
	Graphics( const Graphics& ) = delete;
	Graphics& operator=( const Graphics& ) = delete;
	~Graphics();
	void EndFrame();
	void BeginFrame() noexcept;
	void DrawIndexed( UINT count ) noxnd;
	void DrawIndexed( UINT count, UINT startIndex, INT baseVertex = 0) noxnd;
	void SetProjection( DirectX::FXMMATRIX proj ) noexcept;
	DirectX::XMMATRIX GetProjection() const noexcept;
	void SetCamera( DirectX::FXMMATRIX cam ) noexcept;
	DirectX::XMMATRIX GetCamera() const noexcept;
	void EnableImgui() noexcept;
	void DisableImgui() noexcept;
	bool IsImguiEnabled() const noexcept;
	UINT GetWidth() const noexcept;
	UINT GetHeight() const noexcept;
	std::shared_ptr<Bind::RenderTarget> GetTarget() const noexcept;
	std::shared_ptr<Bind::DepthStencil> GetMasterDepth() const noexcept;
	// screen related
	void OnWindowResize(UINT newClientW, UINT newClientH) noexcept;
	void ResizeBackbuffer(UINT newClientW, UINT newClientH) noxnd;
	void SetViewportFull(UINT w, UINT h) noexcept;
	void SetViewportForRenderTarget(const Bind::RenderTarget& rt) noexcept;
	D3D11_VIEWPORT GetMainViewport() const noexcept { return mainViewport; }
private:
	void UpdateViewport() noexcept;
public:
	// screen related
	HWND hWnd = nullptr;
	UINT width;
	UINT height;
	UINT windowWidth;
	UINT windowHeight;
	D3D11_VIEWPORT mainViewport{};
	DirectX::XMMATRIX projection;
	DirectX::XMMATRIX camera;
	bool imguiEnabled = true;
#ifndef NDEBUG
	DxgiInfoManager infoManager;
#endif
	Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
	Microsoft::WRL::ComPtr<IDXGISwapChain> pSwap;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext;
	std::shared_ptr<Bind::RenderTarget> pTarget;
	std::shared_ptr<Bind::DepthStencil> pMasterDepth;
};