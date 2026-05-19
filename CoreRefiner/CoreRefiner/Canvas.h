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
		Empty,              /**< 全 None（Canvas2D 等默认） */
		Rectangle,          /**< 整幅矩形 White */
		RoundedRectangle,   /**< 圆角矩形，圆角半径 = 25% * min(w,h) */
		Ellipse,            /**< 与四边相切的内接椭圆 */
		Polygon,           /**< 尖角朝上：底边在下，顶点在上边中点 */
	};

	Canvas(unsigned width, unsigned height, Form form = Empty, float formParam = 0.0f);
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