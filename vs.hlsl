StructuredBuffer<float> vertices : register(t0);

struct VSOutput {
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

#define rx radians(0.0h)
#define ry 0.0h

VSOutput main(uint vertexID : SV_VertexID) {
    uint starting_index = vertexID * 7;
    VSOutput output;
    
    half3x3 mx = half3x3(
        half3(1.0h, 0.0h, 0.0h),
        half3(0.0h, cos(rx), -sin(rx)),
        half3(0.0h, sin(rx), cos(rx))
    );
    
    half3x3 my = half3x3(
        half3(cos(ry), 0.0h, sin(ry)),
        half3(0.0h, 1.0h, 0.0h),
        half3(-sin(ry), 0.0h, cos(ry))
    );
    
    half3 coordinates = half3(vertices[starting_index], vertices[starting_index + 1], vertices[starting_index + 2] * 0.01h);
    coordinates = mul(mul(my, mx), coordinates);
    
    output.position = float4(coordinates, 0.1h * (coordinates.z * 100.0h + 10.0h));
    output.color = float4(vertices[starting_index + 3], vertices[starting_index + 4], vertices[starting_index + 5], vertices[starting_index + 6]);
    return output;
}
