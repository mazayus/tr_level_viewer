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

#ifndef TR_LOADER_H
#define TR_LOADER_H

#include "tr_types.h"

#include <stdio.h>
#include <stdint.h>

#include <vector>

// TODO: clean up the loader

namespace tr
{
    /*
     * tr::room_loader
     */

    class room_loader
    {
    public:
        struct params
        {
            long num_rooms, rooms_offset;
            long num_static_meshes, static_meshes_offset;
        };

        void load(FILE* fp, tr::level* level, tr::version version, tr::room_loader::params params);

    private:
        FILE* fp;
        tr::version version;

        struct d_room_vertex
        {
            glm::vec3 position;
            uint16_t lighting1;
            uint16_t attributes;
            uint16_t lighting2;
        };
        void read_room_vertex(d_room_vertex* room_vertex);

        struct d_room_polygon
        {
            uint16_t vertices[4];
            uint16_t texinfo;
        };
        void read_room_polygon(d_room_polygon* room_polygon, int num_vertices);

        struct d_room_static_sprite
        {
            uint16_t vertex;
            uint16_t sprite;
        };
        void read_room_static_sprite(d_room_static_sprite* room_static_sprite);

        struct d_room_light
        {
            glm::vec3 position;
            int16_t intensity1;
            int16_t intensity2;
            int32_t falloff1;
            int32_t falloff2;
        };
        void read_room_light(d_room_light* room_light);

        struct d_room_static_mesh
        {
            glm::vec3 position;
            uint16_t orientation;
            uint16_t lighting1;
            uint16_t lighting2;
            uint16_t static_mesh_id;
        };
        void read_room_static_mesh(d_room_static_mesh* room_static_mesh);

        struct d_room
        {
            int32_t x, z, y_bottom, y_top;
            std::vector<d_room_vertex> vertices;
            std::vector<d_room_polygon> quads;
            std::vector<d_room_polygon> tris;
            std::vector<d_room_static_sprite> static_sprites;
            int16_t ambient_lighting1;
            int16_t ambient_lighting2;
            uint16_t light_mode;
            std::vector<d_room_light> lights;
            std::vector<d_room_static_mesh> static_meshes;
            uint16_t alternate_room;
            uint16_t flags;
        };
        void read_room(d_room* room);

        struct d_static_mesh
        {
            uint32_t id;
            uint16_t mesh;
            glm::vec3 aabb[2][2];
            uint16_t flags;
        };
        void read_static_mesh(d_static_mesh* static_mesh);
    };

    /*
     * tr::loader
     */

    class loader
    {
    public:
        static std::unique_ptr<tr::level> load(const char* filename, tr::version version);

    private:
        loader(const char* filename, tr::version version);
        ~loader();
        loader(const loader&) = delete;
        loader& operator=(const loader&) = delete;

        FILE* fp;
        tr::version version;
        std::unique_ptr<tr::level> level;

        void build_tr1_level_directory();
        void build_tr2_level_directory();

        long emit_anim_frame_tr1(const std::vector<uint16_t>& rawdata, long offset);
        long emit_anim_frame_tr2(const std::vector<uint16_t>& rawdata, long offset, long stride);

        long palette8_offset, palette16_offset;
        long num_texpages, texpages8_offset, texpages16_offset;

        long num_texinfos, texinfos_offset;
        long num_texanimchain_data_words, texanimchain_data_offset;

        long num_mesh_data_words, mesh_data_offset;
        long num_mesh_pointers, mesh_pointers_offset;

        long num_animations, animations_offset;
        long num_anim_structs, anim_structs_offset;
        long num_anim_ranges, anim_ranges_offset;
        long num_anim_command_data_words, anim_command_data_offset;
        long num_anim_frame_data_words, anim_frame_data_offset;

        long num_bone_data_dwords, bone_data_offset;
        long num_models, models_offset;

        long num_sprites, sprites_offset;
        long num_sprite_sequences, sprite_sequences_offset;

        long num_rooms, rooms_offset;
        long num_static_meshes, static_meshes_offset;

        long num_objects, objects_offset;

        void load_palette();
        void load_texpages();
        void load_texinfos();
        void load_meshes();
        void load_animations();
        void load_models();
        void load_sprites();
        void load_sprite_sequences();
        void load_rooms();
        void load_objects();
    };
}

#endif
