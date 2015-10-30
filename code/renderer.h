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

#ifndef RENDERER_H
#define RENDERER_H

// TODO: this only works on linux, retrieve function pointers instead
#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "shaders.h"
#include "tr_types.h"

#include <vector>

/*
 * Renderer
 */

class Renderer
{
public:
    struct FrameInfo
    {
        glm::mat4 projection_matrix;
        glm::mat4 view_matrix;

        std::vector<tr::room*> rooms;
        std::vector<tr::model_object*> model_objects;
        std::vector<tr::sprite_object*> sprite_objects;

        bool debug_draw_all_meshes;
        bool debug_draw_all_sprites;
    };

public:
    Renderer();
    ~Renderer();

    void RegisterLevel(const tr::level& level);
    void RenderFrame(const FrameInfo& frameinfo);

    void NotifyRoomMeshUpdated(const tr::mesh& mesh);

private:
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // NOTE: rooms use a separate shader because their vertices
    // are already in world space

    RoomShader room_shader;
    void DrawRooms(const FrameInfo& frameinfo);

    MeshConstantShader mesh_constant_shader;
    MeshInternalShader mesh_internal_shader;
    MeshExternalShader mesh_external_shader;
    void DrawStaticMeshes(const FrameInfo& frameinfo);
    void DrawModelObjects(const FrameInfo& frameinfo);
    void DebugDrawAllMeshes();

    SpriteShader sprite_shader;
    void DrawStaticSprites(const FrameInfo& frameinfo);
    void DrawSpriteObjects(const FrameInfo& frameinfo);
    void DebugDrawAllSprites();

    GLuint transform_ubo;

    std::vector<GLuint> room_lighting_ubos;
    void InitRoomLightingUniformBuffers(const tr::level& level);

    GLuint texpages;
    void InitTexPages(const tr::level& level);

    struct RenderData
    {
        GLuint vao, vbo;
        GLuint num_objects;
        std::vector<GLint> first_vertex;
        std::vector<GLsizei> num_vertices;
    };

    RenderData room_render_data;
    RenderData mesh_render_data;
    RenderData sprite_render_data;

    void AllocateMeshBuffers(RenderData* render_data, const std::vector<const tr::mesh*>& meshes);
    void UploadMeshData(RenderData* render_data, const tr::mesh* mesh);

    void AllocateSpriteBuffers(RenderData* render_data, const std::vector<const tr::sprite*>& sprites);
    void UploadSpriteData(RenderData* render_data, const tr::sprite* sprite);
};

#endif
