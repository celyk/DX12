struct vs_out {
	float4 clip : SV_POSITION;
	float2 uv : Uv;
};

vs_out main(float3 pos : Position, float2 uv : Uv, float3 normal : Normal, float3 tangent : Tangent)
{
	vs_out output = (vs_out)0;
	output.clip = float4(pos, 1);
	output.uv = uv;
	return output;
}