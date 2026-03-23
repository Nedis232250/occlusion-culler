// ps.hlsl
StructuredBuffer<float> vertices : register(t0);
RWByteAddressBuffer triangles : register(u1);

struct VSOutput {
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

// Simple constant color output
[earlydepthstencil] float4 main(VSOutput input) : SV_TARGET {
    uint3 t = input.color.rgb * 255;
    triangles.Store(((t.r << 16) | (t.g << 8) | t.b) * 4, 1);
    
	return input.color; // Solid red
}
