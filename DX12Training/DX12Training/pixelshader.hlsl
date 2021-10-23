#include "shaderheader.hlsli"

Texture2D<float4> tex:register(t0);
SamplerState smp:register(s0);

float4 main(Output inOutput) : SV_TARGET
{
//	return float4(inOutput.uv,1.0f,1.0f);
//	float4 result = tex.Sample(smp,inOutput.uv);
//	return float4(result.z,result.y,result.x,result.w);
	return float4(tex.Sample(smp,inOutput.uv));
}