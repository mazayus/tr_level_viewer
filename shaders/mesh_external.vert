#version 150 core

layout (std140) uniform TransformBlock
{
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
};

struct Light
{
    vec3 Position;
    vec2 Attribs; // [0] - intensity, [1] - falloff
};

layout (std140) uniform RoomLightingBlock
{
    float AmbientLightIntensity;

    int NumLights;
    Light Lights[8];
};

uniform mat4 ModelMatrix;
uniform float LightIntensity;

in vec4 VertPosition;
in vec2 VertTexCoord;
in vec3 VertNormal;
in ivec2 VertTexAttrib;

out VertexData
{
    vec3 Color;
    vec2 TexCoord;
    flat ivec2 TexAttrib;
};

void main()
{
    vec4 WorldSpacePosition = ModelMatrix * VertPosition;
    vec3 WorldSpaceNormal = mat3(ModelMatrix) * VertNormal;

    // I have no idea what illumination model TR actually uses,
    // but this looks good enough. At least for TR1...

    float DiffuseIntensity = 0.0;
    for (int i = 0; i < NumLights; ++i) {
        float LightDistance = length(Lights[i].Position - WorldSpacePosition.xyz);
        float LightAttenuation = 1.0 / (1.0 + LightDistance / Lights[i].Attribs[1]);
        // wtf? some normals are zero...
        if (dot(VertNormal, VertNormal) < 0.01) {
            DiffuseIntensity += 0.5 * Lights[i].Attribs[0] * LightAttenuation;
        } else {
            vec3 LightVec = normalize(Lights[i].Position - WorldSpacePosition.xyz);
            vec3 NormalVec = normalize(WorldSpaceNormal);
            DiffuseIntensity += Lights[i].Attribs[0] * LightAttenuation * (0.5 + max(dot(LightVec, NormalVec), 0.0));
        }
    }

    Color = AmbientLightIntensity + vec3(DiffuseIntensity);

    TexCoord = VertTexCoord;
    TexAttrib = VertTexAttrib;

    gl_Position = ProjectionMatrix * ViewMatrix * WorldSpacePosition;
}
