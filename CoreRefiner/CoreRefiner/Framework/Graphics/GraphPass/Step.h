#pragma once
#include <vector>
#include <memory>
#include "Bindable.h"
#include "Graphics.h"

class TechniqueProbe;
class Drawable;
namespace Rgph
{
	class RenderQueuePass;
	class RenderGraph;
}

class Step
{
public:
	Step( std::string targetPassName );
	Step( Step&& ) = default;
	Step( const Step& src ) noexcept;
	Step& operator=( const Step& ) = delete;
	Step& operator=( Step&& ) = delete;
	void AddBindable( std::shared_ptr<Bind::Bindable> bind_in ) noexcept;
	void Submit(const Drawable& drawable) const;
	void Bind( Graphics& gfx ) const noxnd;
	void InitializeParentReferences( const class Drawable& parent ) noexcept;
	void Accept( TechniqueProbe& probe );
	void Link(Rgph::RenderGraph& rg);
	void SetDrawIndexed(UINT count, UINT startIndex = 0u, INT baseVertex = 0) noexcept;
	void ClearDrawOverride() noexcept { draw.reset(); }
	struct DrawCommand
	{
		UINT count;
		UINT startIndex;
		INT  baseVertex;
	};
	const std::optional<DrawCommand>& GetDrawOverride() const noexcept { return draw; }
private:
	std::vector<std::shared_ptr<Bind::Bindable>> bindables;
	Rgph::RenderQueuePass* pTargetPass = nullptr;;
	std::string targetPassName;

	std::optional<DrawCommand> draw;
};