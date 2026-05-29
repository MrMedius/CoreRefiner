#pragma once
#include "Drawable.h"
#include "Surface.h"
#include "Provides.h"
#include "Colors.h"

namespace dx = DirectX;

namespace Bind
{
	class CanvasTexture;
}

class Canvas : public Drawable
{
	friend class Bind::CanvasTexture;

public:
	enum Form
	{
		Empty,				// formParam is ignored
		Rectangle,			// formParam is ignored
		RoundedRectangle,	// formParam: corner radius fraction (0.0f to 0.5f for percentage)
		Ellipse,			// formParam is ignored
		Polygon,			// formParam: number of sides (can be float but will be rounded)
		Triangle,			// formParam is ignored; tip at top edge center, base at bottom corners
	};

	Canvas(unsigned width, unsigned height, Form form = Rectangle, float formParam = 0.0f);
	Canvas(const Canvas&) = delete;
	Canvas& operator=(const Canvas&) = delete;
	Canvas(Canvas&&) = delete;
	Canvas& operator=(Canvas&&) = delete;
	~Canvas() override = default;

	dx::XMMATRIX GetTransformXM() const noexcept override;

	void PutPixel(unsigned x, unsigned y, Color c) noxnd;
	Color GetPixel(unsigned x, unsigned y) const noxnd;
	void Clear(Color fill = Colors::None) noexcept;
	void Resize(unsigned width, unsigned height);

	void ApplyForm(Form form, float formParam = 0.0f) noexcept;
	void ReapplyForm() noexcept;

	Surface& GetSurface() noexcept;
	const Surface& GetSurface() const noexcept;

	[[nodiscard]] Form GetForm() const noexcept { return form_; }
	[[nodiscard]] float GetFormParam() const noexcept { return formParam_; }

	unsigned GetCanvasWidth() const noexcept;
	unsigned GetCanvasHeight() const noexcept;

	void SetPosition(dx::XMFLOAT3 pos) noexcept { Drawable::SetPosition(pos); }
	void SetRotation(float rollDeg, float pitchDeg, float yawDeg) noexcept
	{
		Drawable::SetRotation(rollDeg, pitchDeg, yawDeg);
	}
	void SetScale(dx::XMFLOAT3 scale) noexcept { Drawable::SetScale(scale); }

	dx::XMFLOAT3 GetPosition() const noexcept { return trans.GetPosition(); }
	dx::XMFLOAT3 GetRotation() const noexcept { return trans.GetRotation(); }
	dx::XMFLOAT3 GetScale() const noexcept { return trans.GetScale(); }

	void NotifyPixelsChanged() noexcept;

	bool IsGpuDirty() const noexcept { return gpuDirty; }

private:
	void MarkDirtyPixel(unsigned x, unsigned y) noexcept;
	void MarkDirtyAll() noexcept;
	void ClearGpuDirty() noexcept;

	Surface surface;
	Form form_ = Empty;
	float formParam_ = 0.0f;
	bool gpuDirty = true;

	// dirty rect: [dirtyMinX, dirtyMaxX], [dirtyMinY, dirtyMaxY]
	bool hasDirtyRect = false;
	unsigned dirtyMinX = 0u;
	unsigned dirtyMinY = 0u;
	unsigned dirtyMaxX = 0u;
	unsigned dirtyMaxY = 0u;
};