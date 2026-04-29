#pragma once
#include <DirectXMath.h>

using namespace DirectX;

XMFLOAT3 ExtractEulerAngles(const XMFLOAT4X4& matrix);

XMFLOAT3 ExtractTranslation(const XMFLOAT4X4& matrix);

XMMATRIX ScaleTranslation(FXMMATRIX matrix, float scale);

bool Normalize3(XMFLOAT3& v, float eps = 1e-8f) noexcept;
bool NormalizeXZ(XMFLOAT3& v, float eps = 1e-8f) noexcept;