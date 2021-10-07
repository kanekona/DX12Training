#include "shaderheader.hlsli"

Output main(float4 pos : POSITION,float2 uv : TEXCOORD)
{
	Output result;
	result.svpos = pos;
	result.uv = uv;
	return result;
}