#version 150 core

uniform sampler2DArray TexPages;

in VertexData
{
    float LightIntensity;
    vec2 TexCoord;
    flat int TexLayer;
};

out vec4 FragColor;

void main()
{
    vec4 TexColor = texture(TexPages, vec3(TexCoord, TexLayer));
    if (TexColor.a < 0.5)
        discard;
    FragColor = vec4(LightIntensity * TexColor.rgb, 1.0);
}
