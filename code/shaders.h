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

#ifndef SHADER_H
#define SHADER_H

// TODO: this only works on linux, retrieve function pointers instead
#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>

#include <map>
#include <string>
#include <vector>

#define ATTRIB_POSITION             0
#define ATTRIB_TEXCOORD             1
#define ATTRIB_COLOR                2
#define ATTRIB_NORMAL               2
#define ATTRIB_TEXATTRIB            3

#define FRAGDATA_COLOR              0

#define UNIFORMBLOCK_TRANSFORM      0
#define UNIFORMBLOCK_ROOMLIGHTING   1

/*
 * RoomShader
 */

struct RoomShader
{
    GLuint program;

    RoomShader();
    ~RoomShader();
    RoomShader(RoomShader&&);
    RoomShader& operator=(RoomShader&&);
    RoomShader(const RoomShader&) = delete;
    RoomShader& operator=(const RoomShader&) = delete;
};

/*
 * MeshConstantShader
 */

struct MeshConstantShader
{
    GLuint program;

    struct {
        GLuint model_matrix;
        GLuint light_intensity;
    } uniforms;

    MeshConstantShader();
    ~MeshConstantShader();
    MeshConstantShader(MeshConstantShader&&);
    MeshConstantShader& operator=(MeshConstantShader&&);
    MeshConstantShader(const MeshConstantShader&) = delete;
    MeshConstantShader& operator=(const MeshConstantShader&) = delete;
};

/*
 * MeshInternalShader
 */

struct MeshInternalShader
{
    GLuint program;

    struct {
        GLuint model_matrix;
        GLuint light_intensity;
    } uniforms;

    MeshInternalShader();
    ~MeshInternalShader();
    MeshInternalShader(MeshInternalShader&&);
    MeshInternalShader& operator=(MeshInternalShader&&);
    MeshInternalShader(const MeshInternalShader&) = delete;
    MeshInternalShader& operator=(const MeshInternalShader&) = delete;
};

/*
 * MeshExternalShader
 */

struct MeshExternalShader
{
    GLuint program;

    struct {
        GLuint model_matrix;
        GLuint light_intensity;
    } uniforms;

    MeshExternalShader();
    ~MeshExternalShader();
    MeshExternalShader(MeshExternalShader&&);
    MeshExternalShader& operator=(MeshExternalShader&&);
    MeshExternalShader(const MeshExternalShader&) = delete;
    MeshExternalShader& operator=(const MeshExternalShader&) = delete;
};

/*
 * SpriteShader
 */

struct SpriteShader
{
    GLuint program;

    struct {
        GLuint sprite_position;
        GLuint sprite_light_intensity;
    } uniforms;

    SpriteShader();
    ~SpriteShader();
    SpriteShader(SpriteShader&&);
    SpriteShader& operator=(SpriteShader&&);
    SpriteShader(const SpriteShader&) = delete;
    SpriteShader& operator=(const SpriteShader&) = delete;
};

/*
 * ShaderBuilder
 */

class ShaderBuilder
{
public:
    GLuint Build();

    ShaderBuilder& AddShader(GLenum type, const std::string& filename);

    ShaderBuilder& BindAttrib(const std::string& name, GLuint location);
    ShaderBuilder& BindFragData(const std::string& name, GLuint location);
    ShaderBuilder& BindUniformBlock(const std::string& name, GLuint binding_point);

private:
    GLuint CreateShader(GLenum type, const GLchar* source);
    GLuint CreateProgram(const std::vector<GLuint>& shaders);

    std::multimap<GLenum, std::string> shader_sources;

    std::map<std::string, GLuint> attrib_bindings;
    std::map<std::string, GLuint> frag_data_bindings;
    std::map<std::string, GLuint> uniform_block_bindings;
};

#endif
