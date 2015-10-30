/*
 * TR Level Viewer
 * Copyright (C) 2015  Milan Izai <milan.izai@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "shaders.h"

#include <assert.h>
#include <stdio.h>

static std::vector<char> ReadEntireFile(const char* filename)
{
    std::vector<char> data;

    FILE* fp = fopen(filename, "rb");
    assert(fp);
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data.resize(size+1);
    fread(data.data(), 1, size, fp);
    data.back() = '\0';
    fclose(fp);

    return data;
}

/*
 * RoomShader
 */

RoomShader::RoomShader()
{
    program = ShaderBuilder()
        .AddShader(GL_VERTEX_SHADER, "shaders/mesh_room.vert")
        .AddShader(GL_FRAGMENT_SHADER, "shaders/mesh.frag")
        .BindAttrib("VertPosition", ATTRIB_POSITION)
        .BindAttrib("VertTexCoord", ATTRIB_TEXCOORD)
        .BindAttrib("VertColor", ATTRIB_COLOR)
        .BindAttrib("VertTexAttrib", ATTRIB_TEXATTRIB)
        .BindFragData("FragColor", FRAGDATA_COLOR)
        .BindUniformBlock("TransformBlock", UNIFORMBLOCK_TRANSFORM)
        .Build();

    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "TexPages"), 0);
}

RoomShader::~RoomShader()
{
    glDeleteProgram(program);
}

RoomShader::RoomShader(RoomShader&& other)
{
    std::swap(program, other.program);
}

RoomShader& RoomShader::operator=(RoomShader&& other)
{
    std::swap(program, other.program);
    return *this;
}

/*
 * MeshConstantShader
 */

MeshConstantShader::MeshConstantShader()
{
    program = ShaderBuilder()
        .AddShader(GL_VERTEX_SHADER, "shaders/mesh_constant.vert")
        .AddShader(GL_FRAGMENT_SHADER, "shaders/mesh.frag")
        .BindAttrib("VertPosition", ATTRIB_POSITION)
        .BindAttrib("VertTexCoord", ATTRIB_TEXCOORD)
        .BindAttrib("VertTexAttrib", ATTRIB_TEXATTRIB)
        .BindFragData("FragColor", FRAGDATA_COLOR)
        .BindUniformBlock("TransformBlock", UNIFORMBLOCK_TRANSFORM)
        .Build();

    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "TexPages"), 0);
    uniforms.model_matrix = glGetUniformLocation(program, "ModelMatrix");
    uniforms.light_intensity = glGetUniformLocation(program, "LightIntensity");

}

MeshConstantShader::~MeshConstantShader()
{
    glDeleteProgram(program);
}

MeshConstantShader::MeshConstantShader(MeshConstantShader&& other)
{
    std::swap(program, other.program);
}

MeshConstantShader& MeshConstantShader::operator=(MeshConstantShader&& other)
{
    std::swap(program, other.program);
    return *this;
}

/*
 * MeshInternalShader
 */

MeshInternalShader::MeshInternalShader()
{
    program = ShaderBuilder()
        .AddShader(GL_VERTEX_SHADER, "shaders/mesh_internal.vert")
        .AddShader(GL_FRAGMENT_SHADER, "shaders/mesh.frag")
        .BindAttrib("VertPosition", ATTRIB_POSITION)
        .BindAttrib("VertTexCoord", ATTRIB_TEXCOORD)
        .BindAttrib("VertColor", ATTRIB_COLOR)
        .BindAttrib("VertTexAttrib", ATTRIB_TEXATTRIB)
        .BindFragData("FragColor", FRAGDATA_COLOR)
        .BindUniformBlock("TransformBlock", UNIFORMBLOCK_TRANSFORM)
        .BindUniformBlock("RoomLightingBlock", UNIFORMBLOCK_ROOMLIGHTING)
        .Build();

    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "TexPages"), 0);
    uniforms.model_matrix = glGetUniformLocation(program, "ModelMatrix");
    uniforms.light_intensity = glGetUniformLocation(program, "LightIntensity");
}

MeshInternalShader::~MeshInternalShader()
{
    glDeleteProgram(program);
}

MeshInternalShader::MeshInternalShader(MeshInternalShader&& other)
{
    std::swap(program, other.program);
}

MeshInternalShader& MeshInternalShader::operator=(MeshInternalShader&& other)
{
    std::swap(program, other.program);
    return *this;
}

/*
 * MeshExternalShader
 */

MeshExternalShader::MeshExternalShader()
{
    program = ShaderBuilder()
        .AddShader(GL_VERTEX_SHADER, "shaders/mesh_external.vert")
        .AddShader(GL_FRAGMENT_SHADER, "shaders/mesh.frag")
        .BindAttrib("VertPosition", ATTRIB_POSITION)
        .BindAttrib("VertTexCoord", ATTRIB_TEXCOORD)
        .BindAttrib("VertNormal", ATTRIB_NORMAL)
        .BindAttrib("VertTexAttrib", ATTRIB_TEXATTRIB)
        .BindFragData("FragColor", FRAGDATA_COLOR)
        .BindUniformBlock("TransformBlock", UNIFORMBLOCK_TRANSFORM)
        .BindUniformBlock("RoomLightingBlock", UNIFORMBLOCK_ROOMLIGHTING)
        .Build();

    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "TexPages"), 0);
    uniforms.model_matrix = glGetUniformLocation(program, "ModelMatrix");
    uniforms.light_intensity = glGetUniformLocation(program, "LightIntensity");
}

MeshExternalShader::~MeshExternalShader()
{
    glDeleteProgram(program);
}

MeshExternalShader::MeshExternalShader(MeshExternalShader&& other)
{
    std::swap(program, other.program);
}

MeshExternalShader& MeshExternalShader::operator=(MeshExternalShader&& other)
{
    std::swap(program, other.program);
    return *this;
}

/*
 * SpriteShader
 */

SpriteShader::SpriteShader()
{
    program = ShaderBuilder()
        .AddShader(GL_VERTEX_SHADER, "shaders/sprite.vert")
        .AddShader(GL_FRAGMENT_SHADER, "shaders/sprite.frag")
        .BindAttrib("VertPosition", ATTRIB_POSITION)
        .BindAttrib("VertTexCoord", ATTRIB_TEXCOORD)
        .BindAttrib("VertTexLayer", ATTRIB_TEXATTRIB)
        .BindFragData("FragColor", FRAGDATA_COLOR)
        .BindUniformBlock("TransformBlock", UNIFORMBLOCK_TRANSFORM)
        .Build();

    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "TexPages"), 0);
    uniforms.sprite_position = glGetUniformLocation(program, "SpritePosition");
    uniforms.sprite_light_intensity = glGetUniformLocation(program, "SpriteLightIntensity");
}

SpriteShader::~SpriteShader()
{
    glDeleteProgram(program);
}

SpriteShader::SpriteShader(SpriteShader&& other)
{
    std::swap(program, other.program);
}

SpriteShader& SpriteShader::operator=(SpriteShader&& other)
{
    std::swap(program, other.program);
    return *this;
}

/*
 * ShaderBuilder
 */

GLuint ShaderBuilder::Build()
{
    std::vector<GLuint> shaders;
    for (auto it : shader_sources)
        shaders.push_back(CreateShader(it.first, it.second.c_str()));

    GLuint program = CreateProgram(shaders);

    for (auto it : shaders)
        glDeleteShader(it);

    return program;
}

ShaderBuilder& ShaderBuilder::AddShader(GLenum type, const std::string& filename)
{
    std::vector<char> source_vector = ReadEntireFile(filename.c_str());
    std::string source_string(source_vector.data(), source_vector.size());
    shader_sources.insert(std::make_pair(type, source_string));
    return *this;
}

ShaderBuilder& ShaderBuilder::BindAttrib(const std::string& name, GLuint location)
{
    attrib_bindings[name] = location;
    return *this;
}

ShaderBuilder& ShaderBuilder::BindFragData(const std::string& name, GLuint location)
{
    frag_data_bindings[name] = location;
    return *this;
}

ShaderBuilder& ShaderBuilder::BindUniformBlock(const std::string& name, GLuint binding_point)
{
    uniform_block_bindings[name] = binding_point;
    return *this;
}

GLuint ShaderBuilder::CreateShader(GLenum type, const GLchar* source)
{
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint info_log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
        std::vector<char> info_log(info_log_length+1);
        glGetShaderInfoLog(shader, info_log_length, nullptr, info_log.data());
        info_log.back() = '\0';
        fprintf(stderr, "ShaderBuilder::CreateShader():\n%s\n", info_log.data());
    }

    return shader;
}

GLuint ShaderBuilder::CreateProgram(const std::vector<GLuint>& shaders)
{
    GLuint program = glCreateProgram();
    for (GLuint shader : shaders)
        glAttachShader(program, shader);

    for (auto attrib_binding : attrib_bindings)
        glBindAttribLocation(program, attrib_binding.second, attrib_binding.first.c_str());
    for (auto frag_data_binding : frag_data_bindings)
        glBindFragDataLocation(program, frag_data_binding.second, frag_data_binding.first.c_str());

    glLinkProgram(program);

    for (auto uniform_block_binding : uniform_block_bindings) {
        GLuint blockIndex = glGetUniformBlockIndex(program, uniform_block_binding.first.c_str());
        glUniformBlockBinding(program, blockIndex, uniform_block_binding.second);
    }

    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLint info_log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
        std::vector<char> info_log(info_log_length+1);
        glGetProgramInfoLog(program, info_log_length, nullptr, info_log.data());
        info_log.back() = '\0';
        fprintf(stderr, "ShaderBuilder::CreateProgram():\n%s\n", info_log.data());
    }

    return program;
}
