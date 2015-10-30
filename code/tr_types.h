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

#ifndef TR_TYPES_H
#define TR_TYPES_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <vector>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

namespace tr
{
    struct level;

    struct texpage
    {
        uchar pixels[256][256][4];
    };

    struct texinfo
    {
        float texcoord[4][2];
        ushort texpage, texalphamode;
        tr::texinfo* texanimchain;
    };

    enum mesh_lightmode
    {
        mesh_lightmode_internal,
        mesh_lightmode_external
    };

    struct mesh_vert
    {
        glm::vec3 position;
        glm::vec3 lightattrib;
    };

    struct mesh_poly
    {
        ushort verts[4];
        tr::texinfo* texinfo;
    };

    struct mesh
    {
        ulong id;
        tr::mesh_lightmode lightmode;
        std::vector<tr::mesh_vert> verts;
        std::vector<tr::mesh_poly> polys;
    };

    struct anim_frame
    {
        glm::vec3 translation;
        glm::quat rotation[32];
    };

    struct anim_range
    {
        ushort first_tick, last_tick;
        ushort next_anim, next_anim_tick;
    };

    struct anim_struct
    {
        ushort state_id;
        ushort num_anim_ranges;
        ushort anim_range_offset;
    };

    struct animation
    {
        ushort state_id;
        ushort ticks_per_frame;

        ushort first_tick, last_tick;
        ushort next_anim, next_anim_tick;

        ulong frame_offset;

        ushort num_anim_structs;
        ushort anim_struct_offset;
        ushort num_anim_commands;
        ushort anim_command_offset;
    };

    struct model_node
    {
        int parent;
        glm::vec3 offset;
        const tr::mesh* mesh;
    };

    struct model
    {
        ulong id;
        std::vector<tr::model_node> nodes;
        const tr::animation* animation;
    };

    struct sprite
    {
        ulong id;
        float position[4][2];
        float texcoord[4][2];
        ushort texpage;
    };

    struct sprite_sequence
    {
        ulong id;
        std::vector<const tr::sprite*> sprites;
    };

    struct room_light
    {
        glm::vec3 position;
        float intensity, falloff;
    };

    struct room_static_mesh
    {
        const tr::mesh* mesh;
        glm::mat4 transform;
        float light_intensity;
    };

    struct room_static_sprite
    {
        const tr::sprite* sprite;
        glm::vec3 position;
        float light_intensity;
    };

    struct room
    {
        ulong id;

        tr::mesh geometry;
        float ambient_light_intensity;

        std::vector<tr::room_light> lights;
        std::vector<tr::room_static_mesh> static_meshes;
        std::vector<tr::room_static_sprite> static_sprites;

        ushort altroom, flags;
    };

    struct model_object
    {
        const tr::model* model;
        std::vector<glm::mat4> node_transforms;

        const tr::room* room;
        glm::mat4 transform;
        float light_intensity;

        model_object(const tr::level* level, const tr::model* model);
        void tick(float dt);

    private:
        const tr::level* level;

        // TODO: cache current/next frames
        // TODO: use real time instead of engine ticks
        const tr::animation* animation;
        ushort anim_tick;
        float anim_tick_time;

        void update_node_transforms();
        tr::anim_frame smooth_anim_frame() const;
        tr::anim_frame parse_anim_frame(ulong offset) const;
    };

    struct sprite_object
    {
        const tr::sprite_sequence* sequence;
        ushort frame;

        const tr::room* room;
        glm::vec3 position;
        float light_intensity;
    };

    enum version
    {
        version_invalid,

        version_tr1,
        version_tr2
    };

    struct level
    {
        std::vector<tr::room> rooms;

        std::vector<tr::texpage> texpages;
        std::vector<tr::texinfo> texinfos;

        std::vector<tr::mesh> meshes;
        std::vector<tr::model> models;

        std::vector<tr::sprite> sprites;
        std::vector<tr::sprite_sequence> sprite_sequences;

        std::vector<tr::animation> animations;
        std::vector<tr::anim_struct> anim_structs;
        std::vector<tr::anim_range> anim_ranges;
        std::vector<ushort> anim_command_data;
        std::vector<ushort> anim_frame_data;

        std::vector<tr::model_object> model_objects;
        std::vector<tr::sprite_object> sprite_objects;

        static std::unique_ptr<tr::level> load(const char* filename, tr::version version);
    };
}

#endif
