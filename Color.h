#ifndef __COLOR_H_
#define __COLOR_H_

#include <windows.h>

class Color {
public:
	BOOL _bHSV;
	float _H, _S, _V, _L;
	float _R, _G, _B;

public:
	static const float Ratio;
	static const Color White;
	static const Color Black;
	static const Color Gray;
	static const Color WhiteGray;
	static const Color LightGray;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Cyan;
	static const Color Magenta;

public:
	const Color operator +(const Color& Other) const 
    {
		return Color(_R + Other._R,
				_G + Other._G,
				_B + Other._B);
	}

	const Color operator -(const Color& Other) const 
    {
		return Color(_R - Other._R,
				_G - Other._G,
				_B - Other._B);
	}

	const Color operator *(const Color& Other) const
    {
		return Color(_R * Other._R,
				_G * Other._G,
				_B * Other._B);
	}

	const Color operator /(const Color& Other) const 
    {
		return Color(_R / Other._R,
				_G / Other._G,
				_B / Other._B);
	}

	const Color operator +(const float& Value) const
    {
		return Color(_R + Value,
				_G + Value,
				_B + Value);
	}

	const Color operator -(const float& Value) const
    {
			return Color(_R - Value,
				_G - Value,
				_B - Value);
	}

	const Color operator *(const float& Value) const
    {
		return Color(_R * Value,
				_G * Value,
				_B * Value);
	}

	const Color operator /(const float& Value) const 
    {
		return Color(_R / Value,
				_G / Value,
				_B / Value);
	}

public:
	float MaxColor() const;

public:
	COLORREF ToColorRef();
	Color ToColor();
    void ToHSV();
    void ToHSL();

public:
	Color(float R = 0.f, float G = 0.f, float B = 0.f, BOOL bHSV = FALSE);

	operator int()
    {
        return (int)ToColorRef();
    }

	explicit Color(COLORREF ColorRef);
	~Color();
};

#endif
