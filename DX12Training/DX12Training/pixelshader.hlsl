#include "shaderheader.hlsli"

Texture2D<float4> tex:register(t0);
SamplerState smp:register(s0);

float4 main(Output inOutput) : SV_TARGET
{
//	return float4(inOutput.uv,1.0f,1.0f);
	return float4(tex.Sample(smp,inOutput.uv));
}