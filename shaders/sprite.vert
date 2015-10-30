#version 150 core

layout (std140) uniform TransformBlock
{
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
};

uniform vec4 SpritePosition;
uniform float SpriteLightIntensity;

in vec2 VertPosition;
in vec2 VertTexCoord;
in int VertTexLayer;

out VertexData
{
    float LightIntensity;
    vec2 TexCoord;
    flat int TexLayer;
};

void main()
{
    vec4 RightVec = vec4(ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0], 0.0);
    vec4 UpVec = vec4(ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1], 0.0);
    vec4 VertOffset = VertPosition.x * RightVec + VertPosition.y * UpVec;

    gl_Position = ProjectionMatrix * ViewMatrix * (SpritePosition + VertOffset);
    LightIntensity = SpriteLightIntensity;
    TexCoord = VertTexCoord;
    TexLayer = VertTexLayer;
}
