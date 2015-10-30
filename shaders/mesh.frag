#version 150 core

uniform sampler2DArray TexPages;

in VertexData
{
    vec3 Color;
    vec2 TexCoord;
    flat ivec2 TexAttrib;
};

out vec4 FragColor;

void main()
{
    int TexLayer = TexAttrib[0];
    int TexAlphaMode = TexAttrib[1];

    vec4 TexColor = texture(TexPages, vec3(TexCoord, TexLayer));
    if (TexAlphaMode == 1 && TexColor.a < 0.5)
        discard;
    FragColor = vec4(Color * TexColor.rgb, 1.0);
}
