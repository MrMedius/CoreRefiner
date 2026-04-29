#include "XMath.h"

DirectX::XMFLOAT3 ExtractEulerAngles( const XMFLOAT4X4& mat )
{
	XMFLOAT3 euler;
	
	euler.x = asinf( -mat._32 );                  // Pitch
	if( cosf( euler.x ) > 0.0001 )                // Not at poles
	{
		euler.y = atan2f( mat._31,mat._33 );      // Yaw
		euler.z = atan2f( mat._12,mat._22 );      // Roll
	}
	else
	{
		euler.y = 0.0f;                           // Yaw
		euler.z = atan2f( -mat._21,mat._11 );     // Roll
	}

	return euler;
}

DirectX::XMFLOAT3 ExtractTranslation( const XMFLOAT4X4& matrix )
{
	return { matrix._41,matrix._42,matrix._43 };
}

DirectX::XMMATRIX ScaleTranslation( XMMATRIX matrix,float scale )
{
	matrix.r[3].m128_f32[0] *= scale;
	matrix.r[3].m128_f32[1] *= scale;
	matrix.r[3].m128_f32[2] *= scale;
	return matrix;
}

bool Normalize3(XMFLOAT3& v, float eps) noexcept
{
    XMVECTOR vec = XMLoadFloat3(&v);
    XMVECTOR lenSq = XMVector3LengthSq(vec);

    if (XMVectorGetX(lenSq) <= eps)
    {
        v = { 0.0f, 0.0f, 0.0f };
        return false;
    }

    vec = XMVector3Normalize(vec);
    XMStoreFloat3(&v, vec);
    return true;
}

bool NormalizeXZ(XMFLOAT3& v, float eps) noexcept
{
    XMVECTOR vec = XMVectorSet(v.x, 0.0f, v.z, 0.0f);
    XMVECTOR lenSq = XMVector3LengthSq(vec);

    if (XMVectorGetX(lenSq) <= eps)
    {
        v = { 0.0f, 0.0f, 0.0f };
        return false;
    }

    vec = XMVector3Normalize(vec);
    XMStoreFloat3(&v, vec);
    v.y = 0.0f;
    return true;
}
