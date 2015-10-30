#version 150 core

layout (std140) uniform TransformBlock
{
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
};

in vec4 VertPosition;
in vec2 VertTexCoord;
in vec3 VertColor;
in ivec2 VertTexAttrib;

out VertexData
{
    vec3 Color;
    vec2 TexCoord;
    flat ivec2 TexAttrib;
};

void main()
{
    gl_Position = ProjectionMatrix * ViewMatrix * VertPosition;
    Color = VertColor;
    TexCoord = VertTexCoord;
    TexAttrib = VertTexAttrib;
}
