#pragma once

struct Color
{
	float r, g, b, a;

	Color()
		:r(0.0f)
		, g(0.0f)
		, b(0.0f)
		, a(0.0f)
	{
	}

	Color(float inR, float inG, float inB, float inA)
		:r(inR)
		,g(inG)
		,b(inB)
		,a(inA)
	{
	}

};

struct TextureColor
{
	unsigned char r, g, b, a;
};