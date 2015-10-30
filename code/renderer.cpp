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

#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stddef.h>
#include <stdio.h>

/*
 * Renderer
 *
 * TODO: portal rendering
 * TODO: cull invisible objects
 * TODO: sort meshes by shader
 */

Renderer::Renderer()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // transform uniform buffer
    glGenBuffers(1, &transform_ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORMBLOCK_TRANSFORM, transform_ubo);
    glBufferData(GL_UNIFORM_BUFFER, 128, nullptr, GL_DYNAMIC_DRAW);

    // texpages
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texpages);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texpages);

    // room
    glGenVertexArrays(1, &room_render_data.vao);
    glBindVertexArray(room_render_data.vao);
    glGenBuffers(1, &room_render_data.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, room_render_data.vbo);
    room_render_data.num_objects = 0;

    // mesh
    glGenVertexArrays(1, &mesh_render_data.vao);
    glBindVertexArray(mesh_render_data.vao);
    glGenBuffers(1, &mesh_render_data.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh_render_data.vbo);
    mesh_render_data.num_objects = 0;

    // sprite
    glGenVertexArrays(1, &sprite_render_data.vao);
    glBindVertexArray(sprite_render_data.vao);
    glGenBuffers(1, &sprite_render_data.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sprite_render_data.vbo);
    sprite_render_data.num_objects = 0;
}

Renderer::~Renderer()
{
    // TODO: release GL resources
}

void Renderer::RegisterLevel(const tr::level& level)
{
    InitRoomLightingUniformBuffers(level);

    InitTexPages(level);

    // room render data
    std::vector<const tr::mesh*> rooms;
    for (const tr::room& room : level.rooms)
        rooms.push_back(&room.geometry);
    AllocateMeshBuffers(&room_render_data, rooms);
    for (const tr::mesh* mesh : rooms)
        UploadMeshData(&room_render_data, mesh);

    // mesh render data
    std::vector<const tr::mesh*> meshes;
    for (const tr::mesh& mesh : level.meshes)
        meshes.push_back(&mesh);
    AllocateMeshBuffers(&mesh_render_data, meshes);
    for (const tr::mesh* mesh : meshes)
        UploadMeshData(&mesh_render_data, mesh);

    // sprite render data
    std::vector<const tr::sprite*> sprites;
    for (const tr::sprite& sprite : level.sprites)
        sprites.push_back(&sprite);
    AllocateSpriteBuffers(&sprite_render_data, sprites);
    for (const tr::sprite* sprite : sprites)
        UploadSpriteData(&sprite_render_data, sprite);
}

void Renderer::RenderFrame(const Renderer::FrameInfo& frameinfo)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindBuffer(GL_UNIFORM_BUFFER, transform_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 64, glm::value_ptr(frameinfo.projection_matrix));
    glBufferSubData(GL_UNIFORM_BUFFER, 64, 64, glm::value_ptr(frameinfo.view_matrix));

    DrawRooms(frameinfo);

    DrawStaticMeshes(frameinfo);
    DrawModelObjects(frameinfo);
    if (frameinfo.debug_draw_all_meshes)
        DebugDrawAllMeshes();

    DrawStaticSprites(frameinfo);
    DrawSpriteObjects(frameinfo);
    if (frameinfo.debug_draw_all_sprites)
        DebugDrawAllSprites();
}

void Renderer::NotifyRoomMeshUpdated(const tr::mesh& mesh)
{
    UploadMeshData(&room_render_data, &mesh);
}

// room rendering

void Renderer::DrawRooms(const Renderer::FrameInfo& frameinfo)
{
    glUseProgram(room_shader.program);
    glBindVertexArray(room_render_data.vao);

    std::vector<GLint> first_vertex(frameinfo.rooms.size());
    std::vector<GLsizei> num_vertices(frameinfo.rooms.size());
    for (size_t i = 0; i < frameinfo.rooms.size(); ++i) {
        const tr::room* room = frameinfo.rooms[i];
        first_vertex[i] = room_render_data.first_vertex.at(room->id);
        num_vertices[i] = room_render_data.num_vertices.at(room->id);
    }

    glMultiDrawArrays(GL_TRIANGLES, first_vertex.data(), num_vertices.data(), frameinfo.rooms.size());
}

// mesh rendering

void Renderer::DrawStaticMeshes(const Renderer::FrameInfo& frameinfo)
{
    glUseProgram(mesh_internal_shader.program);
    glBindVertexArray(mesh_render_data.vao);

    for (const tr::room* room : frameinfo.rooms) {
        glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORMBLOCK_ROOMLIGHTING,
                         room_lighting_ubos.at(room->id));

        for (const tr::room_static_mesh& static_mesh : room->static_meshes) {
            assert(static_mesh.mesh->lightmode == tr::mesh_lightmode_internal);

            glUniformMatrix4fv(mesh_internal_shader.uniforms.model_matrix,
                               1, GL_FALSE, glm::value_ptr(static_mesh.transform));
            glUniform1f(mesh_internal_shader.uniforms.light_intensity,
                        static_mesh.light_intensity);
            glDrawArrays(GL_TRIANGLES,
                         mesh_render_data.first_vertex[static_mesh.mesh->id],
                         mesh_render_data.num_vertices[static_mesh.mesh->id]);
        }
    }
}

void Renderer::DrawModelObjects(const Renderer::FrameInfo& frameinfo)
{
    glBindVertexArray(mesh_render_data.vao);

    for (const tr::model_object* model_object : frameinfo.model_objects) {
        glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORMBLOCK_ROOMLIGHTING, room_lighting_ubos.at(model_object->room->id));
        const tr::model* model = model_object->model;
        for (unsigned int i = 0; i < model->nodes.size(); ++i) {
            const tr::mesh* mesh = model->nodes[i].mesh;
            if (mesh->lightmode == tr::mesh_lightmode_internal) {
                glUseProgram(mesh_internal_shader.program);
                glUniformMatrix4fv(mesh_internal_shader.uniforms.model_matrix,
                                1, GL_FALSE, glm::value_ptr(model_object->transform * model_object->node_transforms[i]));
                glUniform1f(mesh_internal_shader.uniforms.light_intensity,
                            model_object->light_intensity);
                glDrawArrays(GL_TRIANGLES,
                    mesh_render_data.first_vertex[mesh->id],
                    mesh_render_data.num_vertices[mesh->id]
                );
            } else {
                glUseProgram(mesh_external_shader.program);
                glUniformMatrix4fv(mesh_external_shader.uniforms.model_matrix,
                                1, GL_FALSE, glm::value_ptr(model_object->transform * model_object->node_transforms[i]));
                glUniform1f(mesh_external_shader.uniforms.light_intensity,
                            model_object->light_intensity);
                glDrawArrays(GL_TRIANGLES,
                    mesh_render_data.first_vertex[mesh->id],
                    mesh_render_data.num_vertices[mesh->id]
                );
            }
        }
    }
}

void Renderer::DebugDrawAllMeshes()
{
    glUseProgram(mesh_constant_shader.program);
    glBindVertexArray(mesh_render_data.vao);
    for (GLuint i = 0; i < mesh_render_data.num_objects; ++i) {
        glm::vec3 position(2048.0f * i, 0.0f, -2048.0f);
        glm::mat4 model_matrix = glm::translate(glm::mat4(), position);
        glUniformMatrix4fv(mesh_constant_shader.uniforms.model_matrix,
                            1, GL_FALSE, glm::value_ptr(model_matrix));
        glUniform1f(mesh_constant_shader.uniforms.light_intensity,
                    1.0f);
        glDrawArrays(GL_TRIANGLES, mesh_render_data.first_vertex[i], mesh_render_data.num_vertices[i]);
    }
}

// sprite rendering

void Renderer::DrawStaticSprites(const Renderer::FrameInfo& frameinfo)
{
    glUseProgram(sprite_shader.program);
    glBindVertexArray(sprite_render_data.vao);

    for (const tr::room* room : frameinfo.rooms) {
        for (const tr::room_static_sprite& static_sprite : room->static_sprites) {
            glm::vec4 position = glm::vec4(static_sprite.position, 1.0f);
            glUniform4fv(sprite_shader.uniforms.sprite_position,
                         1, glm::value_ptr(position));
            glUniform1f(sprite_shader.uniforms.sprite_light_intensity,
                        static_sprite.light_intensity);
            glDrawArrays(GL_TRIANGLE_FAN,
                         sprite_render_data.first_vertex[static_sprite.sprite->id],
                         sprite_render_data.num_vertices[static_sprite.sprite->id]);
        }
    }
}

void Renderer::DrawSpriteObjects(const Renderer::FrameInfo& frameinfo)
{
    glUseProgram(sprite_shader.program);
    glBindVertexArray(sprite_render_data.vao);

    for (const tr::sprite_object* sprite_object : frameinfo.sprite_objects) {
        glm::vec4 position = glm::vec4(sprite_object->position, 1.0f);
        glUniform4fv(sprite_shader.uniforms.sprite_position,
                     1, glm::value_ptr(position));
        glUniform1f(sprite_shader.uniforms.sprite_light_intensity,
                    sprite_object->light_intensity);
        glDrawArrays(GL_TRIANGLE_FAN,
            sprite_render_data.first_vertex[sprite_object->sequence->sprites.at(sprite_object->frame)->id],
            sprite_render_data.num_vertices[sprite_object->sequence->sprites.at(sprite_object->frame)->id]
        );
    }
}

void Renderer::DebugDrawAllSprites()
{
    glUseProgram(sprite_shader.program);
    glBindVertexArray(sprite_render_data.vao);

    for (GLuint i = 0; i < sprite_render_data.num_objects; ++i) {
        glm::vec4 position(2048.0f * i, 0.0f, -4096.0f, 1.0f);
        glUniform4fv(sprite_shader.uniforms.sprite_position,
                     1, glm::value_ptr(position));
        glUniform1f(sprite_shader.uniforms.sprite_light_intensity,
                    1.0f);
        glDrawArrays(GL_TRIANGLE_FAN,
                        sprite_render_data.first_vertex[i],
                        sprite_render_data.num_vertices[i]);
    }
}

// room lighting

void Renderer::InitRoomLightingUniformBuffers(const tr::level& level)
{
    // NOTE: room lighting buffers use std140 layout,
    // see shader source code for details

    static const int ROOM_LIGHTING_BUFFER_SIZE = 272;

    static const int AMBIENT_LIGHT_INTENSITY_OFFSET = 0;
    static const int NUM_LIGHTS_OFFSET = 4;
    static const int LIGHTS_OFFSET = 16;

    static const int LIGHT_SIZE = 32;
    static const int LIGHT_POSITION_OFFSET = 0;
    static const int LIGHT_INTENSITY_OFFSET = 16;
    static const int LIGHT_FALLOFF_OFFSET = 20;

    if (!room_lighting_ubos.empty()) {
        glDeleteBuffers(room_lighting_ubos.size(), room_lighting_ubos.data());
        room_lighting_ubos.clear();
    }
    room_lighting_ubos.resize(level.rooms.size());
    glGenBuffers(room_lighting_ubos.size(), room_lighting_ubos.data());

    for (size_t i = 0; i < level.rooms.size(); ++i) {
        const tr::room& room = level.rooms[i];
        assert(room.lights.size() <= 8);

        glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORMBLOCK_ROOMLIGHTING, room_lighting_ubos[i]);
        glBufferData(GL_UNIFORM_BUFFER, ROOM_LIGHTING_BUFFER_SIZE, nullptr, GL_STATIC_DRAW);

        char* base_ptr = (char*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);

        GLfloat* ambient_light_intensity_ptr = (GLfloat*)(base_ptr + AMBIENT_LIGHT_INTENSITY_OFFSET);
        *ambient_light_intensity_ptr = room.ambient_light_intensity;

        GLint* num_lights_ptr = (GLint*)(base_ptr + NUM_LIGHTS_OFFSET);
        *num_lights_ptr = room.lights.size();

        for (size_t j = 0; j < room.lights.size(); ++j) {
            const tr::room_light& light = room.lights[j];

            char* light_base_ptr = base_ptr + LIGHTS_OFFSET + j * LIGHT_SIZE;

            GLfloat* light_position_ptr = (GLfloat*)(light_base_ptr + LIGHT_POSITION_OFFSET);
            light_position_ptr[0] = light.position.x;
            light_position_ptr[1] = light.position.y;
            light_position_ptr[2] = light.position.z;

            GLfloat* light_intensity_ptr = (GLfloat*)(light_base_ptr + LIGHT_INTENSITY_OFFSET);
            *light_intensity_ptr = light.intensity;

            GLfloat* light_falloff_ptr = (GLfloat*)(light_base_ptr + LIGHT_FALLOFF_OFFSET);
            *light_falloff_ptr = light.falloff;
        }

        glUnmapBuffer(GL_UNIFORM_BUFFER);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// texpages

void Renderer::InitTexPages(const tr::level& level)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texpages);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA,
        256, 256, level.texpages.size(), 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr
    );

    for (size_t i = 0; i < level.texpages.size(); ++i)
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
            0, 0, i, 256, 256, 1,
            GL_RGBA, GL_UNSIGNED_BYTE, level.texpages[i].pixels
        );

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// mesh data

struct MeshVertex
{
    float position[3];
    float texcoord[2];
    float lightattrib[3];
    uint16_t texpage;
    uint16_t texalphamode;
};

void Renderer::AllocateMeshBuffers(Renderer::RenderData* render_data, const std::vector<const tr::mesh*>& meshes)
{
    render_data->first_vertex.clear();
    render_data->num_vertices.clear();
    render_data->num_objects = meshes.size();

    long total_num_vertices = 0;
    for (const tr::mesh* mesh : meshes) {
        long num_vertices = 0;
        for (const tr::mesh_poly& poly: mesh->polys)
            num_vertices += (poly.verts[3] == (ushort)-1) ? 3 : 6;
        render_data->first_vertex.push_back(total_num_vertices);
        render_data->num_vertices.push_back(num_vertices);
        total_num_vertices += num_vertices;
    }

    glBindVertexArray(render_data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, render_data->vbo);
    glBufferData(GL_ARRAY_BUFFER, total_num_vertices * sizeof(MeshVertex), nullptr, GL_STATIC_DRAW);

    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, position));
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, texcoord));
    glEnableVertexAttribArray(ATTRIB_TEXCOORD);
    glVertexAttribPointer(ATTRIB_COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, lightattrib));
    glEnableVertexAttribArray(ATTRIB_COLOR);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, lightattrib));
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glVertexAttribIPointer(ATTRIB_TEXATTRIB, 2, GL_SHORT, sizeof(MeshVertex), (void*)offsetof(MeshVertex, texpage));
    glEnableVertexAttribArray(ATTRIB_TEXATTRIB);
}

void Renderer::UploadMeshData(Renderer::RenderData* render_data, const tr::mesh* mesh)
{
    glBindVertexArray(render_data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, render_data->vbo);

    MeshVertex* ptr = (MeshVertex*)glMapBufferRange(GL_ARRAY_BUFFER,
        render_data->first_vertex.at(mesh->id) * sizeof(MeshVertex),
        render_data->num_vertices.at(mesh->id) * sizeof(MeshVertex),
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT
    );

    for (const tr::mesh_poly& poly : mesh->polys) {
        int num_vertices = (poly.verts[3] == (ushort)-1) ? 3 : 4;
        for (int i = 2; i < num_vertices; ++i) {
            int indices[] = {0, i-1, i};
            for (int index : indices) {
                tr::mesh_vert vert = mesh->verts[poly.verts[index]];
                ptr->position[0] = vert.position.x;
                ptr->position[1] = vert.position.y;
                ptr->position[2] = vert.position.z;
                ptr->texcoord[0] = poly.texinfo->texcoord[index][0];
                ptr->texcoord[1] = poly.texinfo->texcoord[index][1];
                ptr->lightattrib[0] = vert.lightattrib[0];
                ptr->lightattrib[1] = vert.lightattrib[1];
                ptr->lightattrib[2] = vert.lightattrib[2];
                ptr->texpage = poly.texinfo->texpage;
                ptr->texalphamode = poly.texinfo->texalphamode;
                ++ptr;
            }
        }
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
}

// sprite data

struct SpriteVertex
{
    float position[2];
    float texcoord[2];
    uint16_t texpage;
};

void Renderer::AllocateSpriteBuffers(Renderer::RenderData* render_data, const std::vector<const tr::sprite*>& sprites)
{
    render_data->first_vertex.clear();
    render_data->num_vertices.clear();
    render_data->num_objects = sprites.size();

    long total_num_vertices = 0;
    for (size_t i = 0; i < sprites.size(); ++i) {
        render_data->first_vertex.push_back(total_num_vertices);
        render_data->num_vertices.push_back(4);
        total_num_vertices += render_data->num_vertices.back();
    }

    glBindVertexArray(render_data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, render_data->vbo);
    glBufferData(GL_ARRAY_BUFFER, total_num_vertices * sizeof(SpriteVertex), nullptr, GL_STATIC_DRAW);

    glVertexAttribPointer(ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, position));
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, texcoord));
    glEnableVertexAttribArray(ATTRIB_TEXCOORD);
    glVertexAttribIPointer(ATTRIB_TEXATTRIB, 1, GL_SHORT, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, texpage));
    glEnableVertexAttribArray(ATTRIB_TEXATTRIB);
}

void Renderer::UploadSpriteData(Renderer::RenderData* render_data, const tr::sprite* sprite)
{
    glBindVertexArray(render_data->vao);
    glBindBuffer(GL_ARRAY_BUFFER, render_data->vbo);

    SpriteVertex* ptr = (SpriteVertex*)glMapBufferRange(GL_ARRAY_BUFFER,
        render_data->first_vertex.at(sprite->id) * sizeof(SpriteVertex),
        render_data->num_vertices.at(sprite->id) * sizeof(SpriteVertex),
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT
    );

    for (int i = 0; i < 4; ++i) {
        ptr->position[0] = sprite->position[i][0];
        ptr->position[1] = sprite->position[i][1];
        ptr->texcoord[0] = sprite->texcoord[i][0];
        ptr->texcoord[1] = sprite->texcoord[i][1];
        ptr->texpage = sprite->texpage;
        ++ptr;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
}
