// ps.hlsl
StructuredBuffer<float> vertices : register(t0);
RWByteAddressBuffer triangles : register(u1);

struct VSOutput {
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

// Simple constant color output
[earlydepthstencil] float4 main(VSOutput input) : SV_TARGET {
	return input.color; // Solid red
}
