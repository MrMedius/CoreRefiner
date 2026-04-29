#include "Sprite.h"

void Sprite::Update(float dt)
{
	accum += dt;
	while (accum >= frameTime)
	{
		accum -= frameTime;

		if (isLoop)
		{
			frame++;
			if (frame > totalTex) frame = 1;
		}
		else
		{
			if (frame < totalTex) frame++;
		}
	}
	if (!isLoop && frame > totalTex) frame = totalTex;

	UpdateUV();
}

void Sprite::SetPosition(float x, float y, float z) noexcept
{
	trans.SetPosition(x, y, z);
	transBase.position = trans.GetPosition();
}

void Sprite::SetRotation(float x, float y, float z) noexcept
{
	trans.SetRotationDegreeToRad(x, y, z);
	transBase.rotation = trans.GetRotation();
}

void Sprite::SetScale(float x, float y, float z) noexcept
{
	trans.SetScale(x, y, z);
	transBase.scale = trans.GetScale();
}

DirectX::XMMATRIX Sprite::GetTransformXM() const noexcept
{
	return trans.GetTransformXM();
}



// auto animation
void Sprite::SetFrameAuto(int numU_, int numV_, int startTex_, int totalTex_, int indexTex_, float fps, bool isFlip_, bool isLoop_)
{
	SetAtlas(numU_, numV_, startTex_, totalTex_);
	SetFPS(fps);
	SetTextureFrameIndex(indexTex_);
	SetFlip(isFlip_);
	SetLoop(isLoop_);
}

void Sprite::SetAtlas(int numU_, int numV_, int startTex_, int totalTex_) noexcept
{
	numU = (numU_ > 0) ? numU_ : 1;
	numV = (numV_ > 0) ? numV_ : 1;
	startTex = (startTex_ > 0) ? startTex_ : 1;
	totalTex = (totalTex_ > 0) ? totalTex_ : 1;

	frame = 1;
	accum = 0.0f;

	UpdateUV();
}

void Sprite::SetFPS(float fps) noexcept
{
	frameTime = (fps > 0.0f) ? (1.0f / fps) : 0.1f;
}



// manual animation
void Sprite::SetFrame(int numU_, int numV_, int startTex_, int totalTex_, int countTex_, int indexTex_, bool isFlip_) noexcept
{
	SetTextureFrameIndex(indexTex_);
	SetFlip(isFlip_);

	numU = (numU_ > 0) ? numU_ : 1;
	numV = (numV_ > 0) ? numV_ : 1;
	startTex = (startTex_ > 0) ? startTex_ : 1;
	totalTex = (totalTex_ > 0) ? totalTex_ : 1;

	frame = countTex_;

	UpdateUV();
}

void Sprite::SetRatioOffset(float ratioX, float ratioY) noexcept
{
	ratioX = std::clamp(ratioX, 0.0f, 1.0f);
	ratioY = std::clamp(ratioY, 0.0f, 1.0f);

	auto pos = transBase.position;
	auto size = transBase.scale;
	auto rad = transBase.rotation.z;
	const float c = cosf(rad);
	const float s = sinf(rad);

	float offsetX = size.x * 0.5f * (1.0f - ratioX);
	float offsetY = size.y * 0.5f * (1.0f - ratioY);

	float rotatedOffsetX = c * offsetX - s * offsetY;
	float rotatedOffsetY = s * offsetX + c * offsetY;

	trans.SetPosition(pos.x - rotatedOffsetX, pos.y - rotatedOffsetY, 0.0f);
	trans.SetScale(size.x * ratioX, size.y * ratioY, 1.0f);

	uvScale = { (float)1 / numU * ratioX, (float)1 / numV * ratioY };
}



void Sprite::UpdateUV(void) noexcept
{
	const int frameNo = (frame - 1) % totalTex + (startTex - 1);
	const int u = frameNo % numU;
	const int v = frameNo / numU;

	if (isFlip)
	{
		uvScale = { -1.0f / float(numU), 1.0f / float(numV) };
		uvOffset = { float(u + 1) / float(numU), float(v) / float(numV) };
	}
	else
	{
		uvScale = { 1.0f / float(numU), 1.0f / float(numV) };
		uvOffset = { float(u) / float(numU), float(v) / float(numV) };
	}
}
