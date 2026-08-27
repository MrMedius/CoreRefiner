#pragma once
#include <math.h>

constexpr float PI = 3.14159265f;
constexpr double PI_D = 3.1415926535897932;

template <typename T>
constexpr auto sq( const T& x ) noexcept
{
	return x * x;
}

template<typename T>
constexpr T clamp(T v, T lo, T hi) noexcept
{
	return v < lo ? lo : (v > hi ? hi : v);
}

template<typename T>
constexpr T clamp01(T v) noexcept
{
	return clamp(v, T(0), T(1));
}

template<typename T>
T wrap_angle( T theta ) noexcept
{
	constexpr T twoPi = (T)2 * (T)PI_D;
	const T mod = (T)fmod( theta,twoPi );
	if( mod > (T)PI_D )
	{
		return mod - twoPi;
	}
	else if( mod < -(T)PI_D )
	{
		return mod + twoPi;
	}
	return mod;
}

template<typename T>
constexpr T interpolate( const T& src,const T& dst,float alpha ) noexcept
{
	return src + (dst - src) * alpha;
}

template<typename T>
constexpr T to_rad( T deg ) noexcept
{
	return deg * PI / (T)180.0;
}

template<typename T>
constexpr T gauss( T x,T sigma ) noexcept
{
	const auto ss = sq( sigma );
	return ((T)1.0 / sqrt( (T)2.0 * (T)PI_D * ss )) * exp( -sq( x ) / ((T)2.0 * ss) );
}

template<typename T>
constexpr T ease_in_back(T t, T backPortion, T sharpness = (T)0) noexcept
{
	t = clamp01(t);
	backPortion = clamp(backPortion, (T)0.0001, (T)0.9999);

	auto hump = [](T u) noexcept
	{
		return (T)4 * u * ((T)1 - u); // u in [0,1] => [0,1]
	};

	const T k = sharpness;
	const T wDash = (T)pow(2.0, (double)k);   // sharpness > 0 bigger dash
	const T wBack = (T)pow(2.0, (double)(-k)); // sharpness < 0 bigger back

	auto toExponent = [](T w) noexcept
	{
		return (T)2 / w;
	};

	const T pBack = toExponent(wBack);
	const T pDash = toExponent(wDash);

	auto pow01 = [&](T x, T p) noexcept
	{
		x = clamp01(x);
		if (p <= (T)0) p = (T)1;
		return (T)pow((double)x, (double)p);
	};

	if (t <= backPortion)
	{
		T u = t / backPortion;     // 0..1
		u = pow01(u, pBack);       // back
		return -hump(u);           // minus accel
	}
	else
	{
		T u = (t - backPortion) / ((T)1 - backPortion);
		u = pow01(u, pDash);       // dash
		return +hump(u);           // plus accel
	}
}

template<typename T>
constexpr T ease_in_pow(T t, T exponent) noexcept
{
	return (T)pow((double)t, (double)exponent);
}

template<typename T>
constexpr T ease_out_pow(T t, T exponent) noexcept
{
	return (T)1 - (T)pow((double)(1 - t), (double)exponent);
}