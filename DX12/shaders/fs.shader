struct vs_out {
	float4 clip : SV_POSITION;
	float2 uv : Uv;
};

float4 main(vs_out input) : SV_TARGET 
{
    return float4(input.uv, 0.2f, 1.0f);
}