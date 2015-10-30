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

#include "tr_loader.h"

#include <glm/gtc/matrix_transform.hpp>

#include <assert.h>
#include <stdint.h>

#include <stdexcept>

static int8_t read8(FILE* fp)
{
    int8_t value = 0;
    fread(&value, sizeof(value), 1, fp);
    return value;
}

static int16_t read16(FILE* fp)
{
    int16_t value = 0;
    fread(&value, sizeof(value), 1, fp);
    return value;
}

static int32_t read32(FILE* fp)
{
    int32_t value = 0;
    fread(&value, sizeof(value), 1, fp);
    return value;
}

/*
 * tr::room_loader
 */

void tr::room_loader::load(FILE* fp, tr::level* level, tr::version version, tr::room_loader::params params)
{
    this->fp = fp;
    this->version = version;

    // static meshes
    fseek(fp, params.static_meshes_offset, SEEK_SET);
    std::vector<d_static_mesh> static_meshes(params.num_static_meshes);
    for (long i = 0; i < params.num_static_meshes; ++i)
        read_static_mesh(&static_meshes[i]);

    // rooms
    fseek(fp, params.rooms_offset, SEEK_SET);
    level->rooms.resize(params.num_rooms);
    for (long room_idx = 0; room_idx < params.num_rooms; ++room_idx) {
        tr::room& room = level->rooms[room_idx];
        room.id = room_idx;

        d_room droom;
        read_room(&droom);

        room.geometry.id = room.id;
        room.geometry.lightmode = tr::mesh_lightmode_internal;

        // vertices
        room.geometry.verts.reserve(droom.vertices.size());
        for (size_t i = 0; i < droom.vertices.size(); ++i) {
            room.geometry.verts.emplace_back();
            tr::mesh_vert& vert = room.geometry.verts.back();
            d_room_vertex& drv = droom.vertices[i];
            vert.position = drv.position + glm::vec3(droom.x, 0.0f, droom.z);
            vert.lightattrib = glm::vec3(1.0f - drv.lighting1 / 8191.0f);
        }

        // polygons
        room.geometry.polys.reserve(droom.quads.size() + droom.tris.size());
        for (size_t i = 0; i < droom.quads.size(); ++i) {
            room.geometry.polys.emplace_back();
            tr::mesh_poly& poly = room.geometry.polys.back();
            d_room_polygon& drp = droom.quads[i];
            for (int j = 0; j < 4; ++j)
                poly.verts[j] = drp.vertices[j];
            poly.texinfo = &level->texinfos.at((drp.texinfo & 0x7FFF) + 256);
        }
        for (size_t i = 0; i < droom.tris.size(); ++i) {
            room.geometry.polys.emplace_back();
            tr::mesh_poly& poly = room.geometry.polys.back();
            d_room_polygon& drp = droom.tris[i];
            for (int j = 0; j < 4; ++j)
                poly.verts[j] = drp.vertices[j];
            poly.texinfo = &level->texinfos.at((drp.texinfo & 0x7FFF) + 256);
        }

        // static sprites
        room.static_sprites.reserve(droom.static_sprites.size());
        for (size_t i = 0; i < droom.static_sprites.size(); ++i) {
            room.static_sprites.emplace_back();
            tr::room_static_sprite& static_sprite = room.static_sprites.back();
            d_room_static_sprite& drss = droom.static_sprites[i];
            static_sprite.position = droom.vertices[drss.vertex].position + glm::vec3(droom.x, 0.0f, droom.z);
            static_sprite.light_intensity = 1.0f - droom.vertices[drss.vertex].lighting1 / 8191.0f;
            static_sprite.sprite = &level->sprites.at(drss.sprite);
        }

        // ambient light intensity
        room.ambient_light_intensity = 1.0f - droom.ambient_lighting1 / 8191.0f;

        // lights
        room.lights.reserve(droom.lights.size());
        for (size_t i = 0; i < droom.lights.size(); ++i) {
            room.lights.emplace_back();
            tr::room_light& light = room.lights.back();
            d_room_light& drl = droom.lights[i];
            light.position = drl.position;
            light.intensity = (drl.intensity1 >= 0) ? (1.0f - drl.intensity1 / 8191.0f) : 0.0f; // WTF?
            light.falloff = drl.falloff1;
        }

        // static meshes
        room.static_meshes.reserve(droom.static_meshes.size());
        for (size_t i = 0; i < droom.static_meshes.size(); ++i) {
            room.static_meshes.emplace_back();
            tr::room_static_mesh& static_mesh = room.static_meshes.back();
            d_room_static_mesh& drsm = droom.static_meshes[i];

            static_mesh.transform = glm::translate(glm::mat4(), drsm.position) *
                glm::rotate(glm::mat4(), ((drsm.orientation >> 14) & 0x03) * glm::pi<float>() / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            static_mesh.light_intensity = 1.0f - drsm.lighting1 / 8191.0f;

            static_mesh.mesh = nullptr;
            for (const d_static_mesh& dsm : static_meshes) {
                if (dsm.id == drsm.static_mesh_id) {
                    static_mesh.mesh = &level->meshes.at(dsm.mesh);
                    break;
                }
            }
            assert(static_mesh.mesh);

            if (static_mesh.mesh->lightmode == tr::mesh_lightmode_external) {
                fprintf(stderr, "[WARNING] tr::room_loader::load(): static mesh references externally-lit mesh\n");
                room.static_meshes.pop_back();
            }
        }

        room.altroom = droom.alternate_room;
        room.flags = droom.flags;
    }
}

void tr::room_loader::read_room_vertex(tr::room_loader::d_room_vertex* room_vertex)
{
    room_vertex->position.x = read16(fp);
    room_vertex->position.y = read16(fp);
    room_vertex->position.z = read16(fp);

    room_vertex->lighting1 = read16(fp);

    if (version == tr::version_tr2) {
        room_vertex->attributes = read16(fp);
        room_vertex->lighting2 = read16(fp);
    } else {
        room_vertex->attributes = 0;
        room_vertex->lighting2 = 0;
    }
}

void tr::room_loader::read_room_polygon(tr::room_loader::d_room_polygon* room_polygon, int num_vertices)
{
    assert(num_vertices == 3 || num_vertices == 4);

    for (int i = 0; i < num_vertices; ++i)
        room_polygon->vertices[i] = read16(fp);
    for (int i = num_vertices; i < 4; ++i)
        room_polygon->vertices[i] = -1;

    room_polygon->texinfo = read16(fp);
}

void tr::room_loader::read_room_static_sprite(tr::room_loader::d_room_static_sprite* room_static_sprite)
{
    room_static_sprite->vertex = read16(fp);
    room_static_sprite->sprite = read16(fp);
}

void tr::room_loader::read_room_light(tr::room_loader::d_room_light* room_light)
{
    room_light->position.x = read32(fp);
    room_light->position.y = read32(fp);
    room_light->position.z = read32(fp);

    room_light->intensity1 = read16(fp);
    if (version == tr::version_tr2)
        room_light->intensity2 = read16(fp);
    else
        room_light->intensity2 = 0;

    room_light->falloff1 = read32(fp);
    if (version == tr::version_tr2)
        room_light->falloff2 = read32(fp);
    else
        room_light->falloff2 = 0;
}

void tr::room_loader::read_room_static_mesh(tr::room_loader::d_room_static_mesh* room_static_mesh)
{
    room_static_mesh->position.x = read32(fp);
    room_static_mesh->position.y = read32(fp);
    room_static_mesh->position.z = read32(fp);

    room_static_mesh->orientation = read16(fp);

    room_static_mesh->lighting1 = read16(fp);
    if (version == tr::version_tr2)
        room_static_mesh->lighting2 = read16(fp);
    else
        room_static_mesh->lighting2 = 0;

    room_static_mesh->static_mesh_id = read16(fp);
}

void tr::room_loader::read_room(tr::room_loader::d_room* room)
{
    // room info
    room->x = read32(fp);
    room->z = read32(fp);
    room->y_bottom = read32(fp);
    room->y_top = read32(fp);

    // begin room data
    uint32_t num_room_data_words = read32(fp);
    long room_data_offset = ftell(fp);

    // room data: vertices
    uint16_t num_vertices = read16(fp);
    room->vertices.resize(num_vertices);
    for (uint16_t i = 0; i < num_vertices; ++i)
        read_room_vertex(&room->vertices[i]);

    // room data: quads
    uint16_t num_quads = read16(fp);
    room->quads.resize(num_quads);
    for (uint16_t i = 0; i < num_quads; ++i)
        read_room_polygon(&room->quads[i], 4);

    // room data: tris
    uint16_t num_tris = read16(fp);
    room->tris.resize(num_tris);
    for (uint16_t i = 0; i < num_tris; ++i)
        read_room_polygon(&room->tris[i], 3);

    // room data: static sprites
    uint16_t num_static_sprites = read16(fp);
    room->static_sprites.resize(num_static_sprites);
    for (uint16_t i = 0; i < num_static_sprites; ++i)
        read_room_static_sprite(&room->static_sprites[i]);

    // end room data
    fseek(fp, room_data_offset + num_room_data_words * 2, SEEK_SET);

    // portals
    uint16_t num_portals = read16(fp);
    fseek(fp, num_portals * 32, SEEK_CUR);

    // sectors
    uint16_t num_z_sectors = read16(fp);
    uint16_t num_x_sectors = read16(fp);
    fseek(fp, num_z_sectors * num_x_sectors * 8, SEEK_CUR);

    // ambient lighting
    room->ambient_lighting1 = read16(fp);
    if (version == tr::version_tr2) {
        room->ambient_lighting2 = read16(fp);
        room->light_mode = read16(fp);
    } else {
        room->ambient_lighting2 = 0;
        room->light_mode = 0;
    }

    // lights
    uint16_t num_lights = read16(fp);
    room->lights.resize(num_lights);
    for (uint16_t i = 0; i < num_lights; ++i)
        read_room_light(&room->lights[i]);

    // static meshes
    uint16_t num_static_meshes = read16(fp);
    room->static_meshes.resize(num_static_meshes);
    for (uint16_t i = 0; i < num_static_meshes; ++i)
        read_room_static_mesh(&room->static_meshes[i]);

    room->alternate_room = read16(fp);
    room->flags = read16(fp);
}

void tr::room_loader::read_static_mesh(tr::room_loader::d_static_mesh* static_mesh)
{
    static_mesh->id = read32(fp);
    static_mesh->mesh = read16(fp);

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            static_mesh->aabb[i][j].x = read16(fp);
            static_mesh->aabb[i][j].y = read16(fp);
            static_mesh->aabb[i][j].z = read16(fp);
        }
    }

    static_mesh->flags = read16(fp);
}

/*
 * tr::loader
 */

std::unique_ptr<tr::level> tr::loader::load(const char* filename, tr::version version)
{
    tr::loader loader(filename, version);

    switch (version) {
        case tr::version_tr1:
            loader.build_tr1_level_directory();
            break;
        case tr::version_tr2:
            loader.build_tr2_level_directory();
            break;
        default:
            throw std::logic_error("tr::loader: bad version");
    }

    loader.load_palette();
    loader.load_texpages();
    loader.load_texinfos();
    loader.load_meshes();
    loader.load_animations();
    loader.load_models();
    loader.load_sprites();
    loader.load_sprite_sequences();
    loader.load_rooms();
    loader.load_objects();

    return std::move(loader.level);
}

tr::loader::loader(const char* filename, tr::version version) :
    fp(nullptr), version(version)
{
    if (!(fp = fopen(filename, "rb")))
        throw std::runtime_error("tr::loader: can't open file");
    level.reset(new tr::level());
}

tr::loader::~loader()
{
    fclose(fp);
}

void tr::loader::build_tr1_level_directory()
{
    fseek(fp, 0, SEEK_SET);

    // version
    fseek(fp, 4, SEEK_CUR);

    // texpages
    num_texpages = read32(fp);
    texpages8_offset = ftell(fp);
    fseek(fp, num_texpages * 256 * 256, SEEK_CUR);
    texpages16_offset = -1;

    // unused
    fseek(fp, 4, SEEK_CUR);

    // rooms
    num_rooms = read16(fp);
    rooms_offset = ftell(fp);
    for (long i = 0; i < num_rooms; ++i) {
        // room info
        fseek(fp, 16, SEEK_CUR);

        // room data
        uint32_t num_room_data_words = read32(fp);
        fseek(fp, num_room_data_words * 2, SEEK_CUR);

        // portals
        uint16_t num_portals = read16(fp);
        fseek(fp, num_portals * 32, SEEK_CUR);

        // sectors
        uint16_t num_z_sectors = read16(fp);
        uint16_t num_x_sectors = read16(fp);
        fseek(fp, num_z_sectors * num_x_sectors * 8, SEEK_CUR);

        // ambient light intensity
        fseek(fp, 2, SEEK_CUR);

        // room lights
        uint16_t num_room_lights = read16(fp);
        fseek(fp, num_room_lights * 18, SEEK_CUR);

        // room static meshes
        uint16_t num_room_static_meshes = read16(fp);
        fseek(fp, num_room_static_meshes * 18, SEEK_CUR);

        // alternate room
        fseek(fp, 2, SEEK_CUR);

        // flags
        fseek(fp, 2, SEEK_CUR);
    }

    // floor data
    uint32_t num_floor_data_words = read32(fp);
    fseek(fp, num_floor_data_words * 2, SEEK_CUR);

    // mesh data
    num_mesh_data_words = read32(fp);
    mesh_data_offset = ftell(fp);
    fseek(fp, num_mesh_data_words * 2, SEEK_CUR);

    // mesh pointers
    num_mesh_pointers = read32(fp);
    mesh_pointers_offset = ftell(fp);
    fseek(fp, num_mesh_pointers * 4, SEEK_CUR);

    // animations
    num_animations = read32(fp);
    animations_offset = ftell(fp);
    fseek(fp, num_animations * 32, SEEK_CUR);

    // anim structs
    num_anim_structs = read32(fp);
    anim_structs_offset = ftell(fp);
    fseek(fp, num_anim_structs * 6, SEEK_CUR);

    // anim ranges
    num_anim_ranges = read32(fp);
    anim_ranges_offset = ftell(fp);
    fseek(fp, num_anim_ranges * 8, SEEK_CUR);

    // anim command data
    num_anim_command_data_words = read32(fp);
    anim_command_data_offset = ftell(fp);
    fseek(fp, num_anim_command_data_words * 2, SEEK_CUR);

    // bone data
    num_bone_data_dwords = read32(fp);
    bone_data_offset = ftell(fp);
    fseek(fp, num_bone_data_dwords * 4, SEEK_CUR);

    // anim frame data
    num_anim_frame_data_words = read32(fp);
    anim_frame_data_offset = ftell(fp);
    fseek(fp, num_anim_frame_data_words * 2, SEEK_CUR);

    // models
    num_models = read32(fp);
    models_offset = ftell(fp);
    fseek(fp, num_models * 18, SEEK_CUR);

    // static meshes
    num_static_meshes = read32(fp);
    static_meshes_offset = ftell(fp);
    fseek(fp, num_static_meshes * 32, SEEK_CUR);

    // texinfos
    num_texinfos = read32(fp);
    texinfos_offset = ftell(fp);
    fseek(fp, num_texinfos * 20, SEEK_CUR);

    // sprites
    num_sprites = read32(fp);
    sprites_offset = ftell(fp);
    fseek(fp, num_sprites * 16, SEEK_CUR);

    // sprite sequences
    num_sprite_sequences = read32(fp);
    sprite_sequences_offset = ftell(fp);
    fseek(fp, num_sprite_sequences * 8, SEEK_CUR);

    // cameras
    uint32_t num_cameras = read32(fp);
    fseek(fp, num_cameras * 16, SEEK_CUR);

    // sound sources
    uint32_t num_sound_sources = read32(fp);
    fseek(fp, num_sound_sources * 16, SEEK_CUR);

    // boxes
    uint32_t num_boxes = read32(fp);
    fseek(fp, num_boxes * 20, SEEK_CUR);

    // overlap data
    uint32_t num_overlap_data_words = read32(fp);
    fseek(fp, num_overlap_data_words * 2, SEEK_CUR);

    // zones
    fseek(fp, num_boxes * 12, SEEK_CUR);

    // texanimchain data
    num_texanimchain_data_words = read32(fp);
    texanimchain_data_offset = ftell(fp);
    fseek(fp, num_texanimchain_data_words * 2, SEEK_CUR);

    // objects
    num_objects = read32(fp);
    objects_offset = ftell(fp);
    fseek(fp, num_objects * 22, SEEK_CUR);

    // light map
    fseek(fp, 32 * 256, SEEK_CUR);

    // palette
    palette8_offset = ftell(fp);
    fseek(fp, 256 * 3, SEEK_CUR);
    palette16_offset = -1;

    // cinematic frames
    uint16_t num_cinematic_frames = read16(fp);
    fseek(fp, num_cinematic_frames * 16, SEEK_CUR);

    // demo data
    uint16_t num_demo_data_bytes = read16(fp);
    fseek(fp, num_demo_data_bytes, SEEK_CUR);

    // sound map
    fseek(fp, 256 * 2, SEEK_CUR);

    // sound details
    uint32_t num_sound_details = read32(fp);
    fseek(fp, num_sound_details * 8, SEEK_CUR);

    // samples
    uint32_t num_samples = read32(fp);
    fseek(fp, num_samples, SEEK_CUR);

    // sample indices
    uint32_t num_sample_indices = read32(fp);
    fseek(fp, num_sample_indices * 4, SEEK_CUR);

    // sanity check
    long end_offset = ftell(fp);
    fseek(fp, 0, SEEK_END);
    long real_end_offset = ftell(fp);
    assert(!feof(fp) && !ferror(fp) && (end_offset == real_end_offset));
}

void tr::loader::build_tr2_level_directory()
{
    fseek(fp, 0, SEEK_SET);

    // version
    fseek(fp, 4, SEEK_CUR);

    // palette
    palette8_offset = ftell(fp);
    fseek(fp, 256 * 3, SEEK_CUR);
    palette16_offset = ftell(fp);
    fseek(fp, 256 * 4, SEEK_CUR);

    // texpages
    num_texpages = read32(fp);
    texpages8_offset = ftell(fp);
    fseek(fp, num_texpages * 256 * 256, SEEK_CUR);
    texpages16_offset = ftell(fp);
    fseek(fp, num_texpages * 256 * 256 * 2, SEEK_CUR);

    // unused
    fseek(fp, 4, SEEK_CUR);

    // rooms
    num_rooms = read16(fp);
    rooms_offset = ftell(fp);
    for (long i = 0; i < num_rooms; ++i) {
        // room info
        fseek(fp, 16, SEEK_CUR);

        // room data
        uint32_t num_room_data_words = read32(fp);
        fseek(fp, num_room_data_words * 2, SEEK_CUR);

        // portals
        uint16_t num_portals = read16(fp);
        fseek(fp, num_portals * 32, SEEK_CUR);

        // sectors
        uint16_t num_z_sectors = read16(fp);
        uint16_t num_x_sectors = read16(fp);
        fseek(fp, num_z_sectors * num_x_sectors * 8, SEEK_CUR);

        // ambient light intensity
        fseek(fp, 2, SEEK_CUR);
        // ambient light intensity 2
        fseek(fp, 2, SEEK_CUR);
        // light mode
        fseek(fp, 2, SEEK_CUR);

        // room lights
        uint16_t num_room_lights = read16(fp);
        fseek(fp, num_room_lights * 24, SEEK_CUR);

        // room static meshes
        uint16_t num_room_static_meshes = read16(fp);
        fseek(fp, num_room_static_meshes * 20, SEEK_CUR);

        // alternate room
        fseek(fp, 2, SEEK_CUR);

        // flags
        fseek(fp, 2, SEEK_CUR);
    }

    // floor data
    uint32_t num_floor_data_words = read32(fp);
    fseek(fp, num_floor_data_words * 2, SEEK_CUR);

    // mesh data
    num_mesh_data_words = read32(fp);
    mesh_data_offset = ftell(fp);
    fseek(fp, num_mesh_data_words * 2, SEEK_CUR);

    // mesh pointers
    num_mesh_pointers = read32(fp);
    mesh_pointers_offset = ftell(fp);
    fseek(fp, num_mesh_pointers * 4, SEEK_CUR);

    // animations
    num_animations = read32(fp);
    animations_offset = ftell(fp);
    fseek(fp, num_animations * 32, SEEK_CUR);

    // anim structs
    num_anim_structs = read32(fp);
    anim_structs_offset = ftell(fp);
    fseek(fp, num_anim_structs * 6, SEEK_CUR);

    // anim ranges
    num_anim_ranges = read32(fp);
    anim_ranges_offset = ftell(fp);
    fseek(fp, num_anim_ranges * 8, SEEK_CUR);

    // anim command data
    num_anim_command_data_words = read32(fp);
    anim_command_data_offset = ftell(fp);
    fseek(fp, num_anim_command_data_words * 2, SEEK_CUR);

    // bones
    num_bone_data_dwords = read32(fp);
    bone_data_offset = ftell(fp);
    fseek(fp, num_bone_data_dwords * 4, SEEK_CUR);

    // anim frame data
    num_anim_frame_data_words = read32(fp);
    anim_frame_data_offset = ftell(fp);
    fseek(fp, num_anim_frame_data_words * 2, SEEK_CUR);

    // models
    num_models = read32(fp);
    models_offset = ftell(fp);
    fseek(fp, num_models * 18, SEEK_CUR);

    // static meshes
    num_static_meshes = read32(fp);
    static_meshes_offset = ftell(fp);
    fseek(fp, num_static_meshes * 32, SEEK_CUR);

    // texinfos
    num_texinfos = read32(fp);
    texinfos_offset = ftell(fp);
    fseek(fp, num_texinfos * 20, SEEK_CUR);

    // sprites
    num_sprites = read32(fp);
    sprites_offset = ftell(fp);
    fseek(fp, num_sprites * 16, SEEK_CUR);

    // sprite sequences
    num_sprite_sequences = read32(fp);
    sprite_sequences_offset = ftell(fp);
    fseek(fp, num_sprite_sequences * 8, SEEK_CUR);

    // cameras
    uint32_t num_cameras = read32(fp);
    fseek(fp, num_cameras * 16, SEEK_CUR);

    // sound sources
    uint32_t num_sound_sources = read32(fp);
    fseek(fp, num_sound_sources * 16, SEEK_CUR);

    // boxes
    uint32_t num_boxes = read32(fp);
    fseek(fp, num_boxes * 8, SEEK_CUR);

    // overlap data
    uint32_t num_overlap_data_words = read32(fp);
    fseek(fp, num_overlap_data_words * 2, SEEK_CUR);

    // zones
    fseek(fp, num_boxes * 20, SEEK_CUR);

    // texanimchain data
    num_texanimchain_data_words = read32(fp);
    texanimchain_data_offset = ftell(fp);
    fseek(fp, num_texanimchain_data_words * 2, SEEK_CUR);

    // objects
    num_objects = read32(fp);
    objects_offset = ftell(fp);
    fseek(fp, num_objects * 24, SEEK_CUR);

    // light map
    fseek(fp, 32 * 256, SEEK_CUR);

    // cinematic frames
    uint16_t num_cinematic_frames = read16(fp);
    fseek(fp, num_cinematic_frames * 16, SEEK_CUR);

    // demo data
    uint16_t num_demo_data_bytes = read16(fp);
    fseek(fp, num_demo_data_bytes, SEEK_CUR);

    // sound map
    fseek(fp, 370 * 2, SEEK_CUR);

    // sound details
    uint32_t num_sound_details = read32(fp);
    fseek(fp, num_sound_details * 8, SEEK_CUR);

    // sample indices
    uint32_t num_sample_indices = read32(fp);
    fseek(fp, num_sample_indices * 4, SEEK_CUR);

    // sanity check
    long end_offset = ftell(fp);
    fseek(fp, 0, SEEK_END);
    long real_end_offset = ftell(fp);
    assert(!feof(fp) && !ferror(fp) && (end_offset == real_end_offset));
}

long tr::loader::emit_anim_frame_tr1(const std::vector<uint16_t>& rawdata, long offset)
{
    assert(offset >= 0);

    if ((long)rawdata.size() < offset + 10)
        throw std::runtime_error("loader::emit_anim_frame_tr1: bad frame offset");

    uint16_t num_anglesets = rawdata.at(offset + 9);
    if ((long)rawdata.size() < offset + 10 + num_anglesets * 2)
        throw std::runtime_error("loader::emit_anim_frame_tr1: bad number of angle sets");

    level->anim_frame_data.push_back(9 + num_anglesets*2);
    for (int i = 0; i < 9; ++i)
        level->anim_frame_data.push_back(rawdata.at(offset + i));
    offset += 10;

    for (uint16_t i = 0; i < num_anglesets; ++i) {
        uint16_t tmp1 = rawdata.at(offset++);
        uint16_t tmp2 = rawdata.at(offset++);
        level->anim_frame_data.push_back(tmp2);
        level->anim_frame_data.push_back(tmp1);
    }

    return offset;
}

long tr::loader::emit_anim_frame_tr2(const std::vector<uint16_t>& rawdata, long offset, long stride)
{
    assert(offset >= 0 && stride >= 0);

    if ((long)rawdata.size() < offset + stride)
        throw std::runtime_error("loader::emit_anim_frame_tr2: bad frame offset/stride");

    level->anim_frame_data.push_back(stride);
    for (long i = 0; i < stride; ++i)
        level->anim_frame_data.push_back(rawdata.at(offset++));

    return offset;
}

void tr::loader::load_palette()
{
    assert(level->texpages.empty());
    assert(level->texinfos.empty());

    uint8_t palette[256][3];
    fseek(fp, palette8_offset, SEEK_SET);
    fread(palette, 3, 256, fp);

    level->texpages.emplace_back();
    tr::texpage& palpage = level->texpages.at(0);
    for (int i = 0; i < 256; ++i) {
        palpage.pixels[0][i][0] = palette[i][0] * 4;
        palpage.pixels[0][i][1] = palette[i][1] * 4;
        palpage.pixels[0][i][2] = palette[i][2] * 4;
        palpage.pixels[0][i][3] = (i == 0) ? 0 : 255;

        level->texinfos.emplace_back();
        tr::texinfo& palinfo = level->texinfos.at(i);
        palinfo.texanimchain = nullptr;
        palinfo.texalphamode = 0;
        palinfo.texpage = 0;
        for (int j = 0; j < 4; ++j) {
            palinfo.texcoord[j][0] = (i + 0.5f) / 256.0f;
            palinfo.texcoord[j][1] = 0.5f / 256.0f;
        }
    }

    assert(level->texpages.size() == 1);
    assert(level->texinfos.size() == 256);
}

void tr::loader::load_texpages()
{
    assert(level->texpages.size() == 1);

    fseek(fp, texpages8_offset, SEEK_SET);
    for (int p = 0; p < num_texpages; ++p) {
        uint8_t pixels[256][256];
        fread(pixels, 1, 256 * 256, fp);
        level->texpages.emplace_back();
        tr::texpage& texpage = level->texpages.back();
        for (int i = 0; i < 256; ++i) {
            for (int j = 0; j < 256; ++j) {
                uchar* color = level->texpages.front().pixels[0][pixels[i][j]];
                std::copy(color, color + 4, texpage.pixels[i][j]);
            }
        }
    }
}

void tr::loader::load_texinfos()
{
    assert(level->texinfos.size() == 256);

    fseek(fp, texinfos_offset, SEEK_SET);
    for (long i = 0; i < num_texinfos; ++i) {
        level->texinfos.emplace_back();
        tr::texinfo& texinfo = level->texinfos.back();
        texinfo.texanimchain = nullptr;
        texinfo.texalphamode = (uint16_t)read16(fp);
        texinfo.texpage = (uint16_t)read16(fp) + 1;
        for (int j = 0; j < 4; ++j) {
            fseek(fp, 1, SEEK_CUR);
            texinfo.texcoord[j][0] = ((uint8_t)read8(fp) + 0.5f) / 256.0f;
            fseek(fp, 1, SEEK_CUR);
            texinfo.texcoord[j][1] = ((uint8_t)read8(fp) + 0.5f) / 256.0f;
        }
    }

    fseek(fp, texanimchain_data_offset, SEEK_SET);
    uint16_t num_texanimchains = read16(fp);
    for (uint16_t i = 0; i < num_texanimchains; ++i) {
        uint32_t num_texinfos = read16(fp) + 1;
        std::vector<uint16_t> texinfos(num_texinfos);
        for (uint32_t j = 0; j < num_texinfos; ++j)
            texinfos[j] = read16(fp) + 256;
        for (uint32_t j = 0; j < num_texinfos; ++j) {
            uint16_t src = texinfos[j], dest = texinfos[(j + 1) % num_texinfos];
            level->texinfos.at(src).texanimchain = &level->texinfos.at(dest);
        }
    }
}

void tr::loader::load_meshes()
{
    fseek(fp, mesh_pointers_offset, SEEK_SET);
    std::vector<uint32_t> mesh_pointers(num_mesh_pointers);
    fread(mesh_pointers.data(), 4, num_mesh_pointers, fp);

    level->meshes.resize(num_mesh_pointers);
    for (long i = 0; i < num_mesh_pointers; ++i) {
        tr::mesh& mesh = level->meshes.at(i);
        mesh.id = i;

        fseek(fp, mesh_data_offset + mesh_pointers[i], SEEK_SET);

        // bounding sphere
        fseek(fp, 10, SEEK_CUR);

        // positions
        int16_t num_verts = read16(fp);
        mesh.verts.resize(num_verts);
        for (int16_t j = 0; j < num_verts; ++j) {
            mesh.verts[j].position.x = read16(fp);
            mesh.verts[j].position.y = read16(fp);
            mesh.verts[j].position.z = read16(fp);
        }

        // light attribs
        int16_t num_lightattribs = read16(fp);
        if (num_lightattribs > 0) {
            // normals
            assert(num_lightattribs == num_verts);
            mesh.lightmode = tr::mesh_lightmode_external;
            for (int16_t j = 0; j < num_lightattribs; ++j) {
                mesh.verts[j].lightattrib.x = read16(fp);
                mesh.verts[j].lightattrib.y = read16(fp);
                mesh.verts[j].lightattrib.z = read16(fp);
            }
        } else {
            // colors
            assert(-num_lightattribs == num_verts);
            mesh.lightmode = tr::mesh_lightmode_internal;
            for (int16_t j = 0; j < -num_lightattribs; ++j) {
                float lightintensity = 1.0f - read16(fp) / 8191.0f;
                mesh.verts[j].lightattrib.r = lightintensity;
                mesh.verts[j].lightattrib.g = lightintensity;
                mesh.verts[j].lightattrib.b = lightintensity;
            }
        }

        // textured quads
        int16_t num_textured_quads = read16(fp);
        for (int16_t j = 0; j < num_textured_quads; ++j) {
            mesh.polys.emplace_back();
            tr::mesh_poly& poly = mesh.polys.back();
            for (int k = 0; k < 4; ++k)
                poly.verts[k] = read16(fp);
            poly.texinfo = &level->texinfos.at((read16(fp) & 0x7FFF) + 256);
        }

        // textured tris
        int16_t num_textured_tris = read16(fp);
        for (int16_t j = 0; j < num_textured_tris; ++j) {
            mesh.polys.emplace_back();
            tr::mesh_poly& poly = mesh.polys.back();
            for (int k = 0; k < 3; ++k)
                poly.verts[k] = read16(fp);
            poly.verts[3] = -1;
            poly.texinfo = &level->texinfos.at((read16(fp) & 0x7FFF) + 256);
        }

        // colored quads
        int16_t num_colored_quads = read16(fp);
        for (int16_t j = 0; j < num_colored_quads; ++j) {
            mesh.polys.emplace_back();
            tr::mesh_poly& poly = mesh.polys.back();
            for (int k = 0; k < 4; ++k)
                poly.verts[k] = read16(fp);
            poly.texinfo = &level->texinfos.at(read16(fp) & 0xFF);
        }

        // colored tris
        int16_t num_colored_tris = read16(fp);
        for (int16_t j = 0; j < num_colored_tris; ++j) {
            mesh.polys.emplace_back();
            tr::mesh_poly& poly = mesh.polys.back();
            for (int k = 0; k < 3; ++k)
                poly.verts[k] = read16(fp);
            poly.verts[3] = -1;
            poly.texinfo = &level->texinfos.at(read16(fp) & 0xFF);
        }
    }
}

void tr::loader::load_animations()
{
    // anim frame data
    fseek(fp, anim_frame_data_offset, SEEK_SET);
    std::vector<uint16_t> frame_data(num_anim_frame_data_words);
    fread(frame_data.data(), 2, num_anim_frame_data_words, fp);

    // anim command data
    fseek(fp, anim_command_data_offset, SEEK_SET);
    level->anim_command_data.resize(num_anim_command_data_words);
    fread(level->anim_command_data.data(), 2, num_anim_command_data_words, fp);

    // anim ranges
    fseek(fp, anim_ranges_offset, SEEK_SET);
    level->anim_ranges.resize(num_anim_ranges);
    for (long i = 0; i < num_anim_ranges; ++i) {
        tr::anim_range& anim_range = level->anim_ranges.at(i);
        anim_range.first_tick = read16(fp);
        anim_range.last_tick = read16(fp);
        anim_range.next_anim = read16(fp);
        anim_range.next_anim_tick = read16(fp);
    }

    // anim structs
    fseek(fp, anim_structs_offset, SEEK_SET);
    level->anim_structs.resize(num_anim_structs);
    for (long i = 0; i < num_anim_structs; ++i) {
        tr::anim_struct& anim_struct = level->anim_structs.at(i);
        anim_struct.state_id = read16(fp);
        anim_struct.num_anim_ranges = read16(fp);
        anim_struct.anim_range_offset = read16(fp);
    }

    // animations
    fseek(fp, animations_offset, SEEK_SET);
    level->animations.resize(num_animations);

    struct d_anim_extra
    {
        uint32_t frame_offset; // in bytes
        uint8_t frame_size; // in words
    };
    std::vector<d_anim_extra> anim_extras(num_animations);

    for (long i = 0; i < num_animations; ++i) {
        tr::animation& animation = level->animations.at(i);
        d_anim_extra& anim_extra = anim_extras.at(i);

        anim_extra.frame_offset = read32(fp);
        assert(anim_extra.frame_offset % 2 == 0);

        animation.ticks_per_frame = read8(fp);

        anim_extra.frame_size = read8(fp);
        assert((version == tr::version_tr1 && anim_extra.frame_size == 0)
            || (version == tr::version_tr2 && anim_extra.frame_size != 0));

        animation.state_id = read16(fp);
        fseek(fp, 8, SEEK_CUR); // unknown
        animation.first_tick = read16(fp);
        animation.last_tick = read16(fp);
        animation.next_anim = read16(fp);
        animation.next_anim_tick = read16(fp);
        animation.num_anim_structs = read16(fp);
        animation.anim_struct_offset = read16(fp);
        animation.num_anim_commands = read16(fp);
        animation.anim_command_offset = read16(fp);
    }

    // convert anim frames to a common format
    uint32_t frame_offset = 0;
    for (long i = 0; i < num_animations; ++i) {
        level->animations.at(i).frame_offset = level->anim_frame_data.size();

        const d_anim_extra& anim_extra = anim_extras.at(i);
        assert(frame_offset * 2 == anim_extra.frame_offset);

        uint32_t next_anim_frame_offset = ((i == num_animations-1) ? frame_data.size() : anim_extras.at(i+1).frame_offset / 2);
        while (frame_offset < next_anim_frame_offset) {
            switch (version) {
                case tr::version_tr1:
                    frame_offset = emit_anim_frame_tr1(frame_data, frame_offset);
                    break;
                case tr::version_tr2:
                    frame_offset = emit_anim_frame_tr2(frame_data, frame_offset, anim_extra.frame_size);
                    break;
                default:
                    throw std::logic_error("loader::load_animations: bad version");
            }
        }
        assert(frame_offset == next_anim_frame_offset);
    }
}

void tr::loader::load_models()
{
    fseek(fp, bone_data_offset, SEEK_SET);
    std::vector<int32_t> bone_data(num_bone_data_dwords);
    fread(bone_data.data(), 4, num_bone_data_dwords, fp);

    struct d_model
    {
        uint32_t id;
        uint16_t num_meshes;
        uint16_t first_mesh;
        uint32_t bone_data_offset;
        uint32_t frame_data_offset;
        uint16_t animation;
    };

    fseek(fp, models_offset, SEEK_SET);
    for (long i = 0; i < num_models; ++i) {
        d_model dmodel;
        dmodel.id = read32(fp);
        dmodel.num_meshes = read16(fp);
        dmodel.first_mesh = read16(fp);
        dmodel.bone_data_offset = read32(fp);
        dmodel.frame_data_offset = read32(fp);
        dmodel.animation = read16(fp);

        level->models.emplace_back();
        tr::model& model = level->models.back();
        model.id = dmodel.id;
        model.animation = (dmodel.animation != (uint16_t)-1) ? &level->animations.at(dmodel.animation) : nullptr;

        std::vector<int> node_stack;
        node_stack.reserve(8);
        for (uint16_t j = 0; j < dmodel.num_meshes; ++j) {
            model.nodes.emplace_back();
            tr::model_node& node = model.nodes.back();
            node.parent = j-1;
            node.offset = glm::vec3(0.0f, 0.0f, 0.0f);
            node.mesh = &level->meshes.at(dmodel.first_mesh + j);

            if (j != 0) {
                uint32_t bone_op = bone_data[dmodel.bone_data_offset + (j-1) * 4];
                int32_t bone_x = bone_data[dmodel.bone_data_offset + (j-1) * 4 + 1];
                int32_t bone_y = bone_data[dmodel.bone_data_offset + (j-1) * 4 + 2];
                int32_t bone_z = bone_data[dmodel.bone_data_offset + (j-1) * 4 + 3];

                if (bone_op & 0x01) {
                    node.parent = node_stack.back();
                    node_stack.pop_back();
                }
                if (bone_op & 0x02) {
                    node_stack.push_back(node.parent);
                }

                node.offset = glm::vec3(bone_x, bone_y, bone_z);
            }
        }
    }
}

void tr::loader::load_sprites()
{
    struct d_sprite
    {
        uint16_t texpage;
        uint8_t x, y;
        uint16_t w, h;
        int16_t left, top, right, bottom;
    };

    fseek(fp, sprites_offset, SEEK_SET);
    level->sprites.resize(num_sprites);
    for (long i = 0; i < num_sprites; ++i) {
        d_sprite dsprite;
        dsprite.texpage = read16(fp);
        dsprite.x = read8(fp);
        dsprite.y = read8(fp);
        dsprite.w = read16(fp);
        dsprite.h = read16(fp);
        dsprite.left = read16(fp);
        dsprite.top = read16(fp);
        dsprite.right = read16(fp);
        dsprite.bottom = read16(fp);

        tr::sprite& sprite = level->sprites.at(i);
        sprite.id = i;
        sprite.texpage = dsprite.texpage + 1;

        sprite.texcoord[0][0] = (dsprite.x + 0.5f) / 256.0f;
        sprite.texcoord[0][1] = (dsprite.y + 0.5f + (dsprite.h - 255.0f) / 256.0f) / 256.0f;
        sprite.texcoord[1][0] = (dsprite.x + 0.5f) / 256.0f;
        sprite.texcoord[1][1] = (dsprite.y + 0.5f) / 256.0f;
        sprite.texcoord[2][0] = (dsprite.x + 0.5f + (dsprite.w - 255.0f) / 256.0f) / 256.0f;
        sprite.texcoord[2][1] = (dsprite.y + 0.5f) / 256.0f;
        sprite.texcoord[3][0] = (dsprite.x + 0.5f + (dsprite.w - 255.0f) / 256.0f) / 256.0f;
        sprite.texcoord[3][1] = (dsprite.y + 0.5f + (dsprite.h - 255.0f) / 256.0f) / 256.0f;

        sprite.position[0][0] = dsprite.left;
        sprite.position[0][1] = -dsprite.bottom;
        sprite.position[1][0] = dsprite.left;
        sprite.position[1][1] = -dsprite.top;
        sprite.position[2][0] = dsprite.right;
        sprite.position[2][1] = -dsprite.top;
        sprite.position[3][0] = dsprite.right;
        sprite.position[3][1] = -dsprite.bottom;
    }
}

void tr::loader::load_sprite_sequences()
{
    struct d_sprite_sequence
    {
        uint32_t id;
        uint16_t num_frames;
        uint16_t first_frame;
    };

    fseek(fp, sprite_sequences_offset, SEEK_SET);
    level->sprite_sequences.resize(num_sprite_sequences);
    for (long i = 0; i < num_sprite_sequences; ++i) {
        d_sprite_sequence dspritesequence;
        dspritesequence.id = read32(fp);
        dspritesequence.num_frames = -read16(fp);
        dspritesequence.first_frame = read16(fp);

        tr::sprite_sequence& sprite  = level->sprite_sequences.at(i);
        sprite.id = dspritesequence.id;
        for (uint16_t j = 0; j < dspritesequence.num_frames; ++j)
            sprite.sprites.push_back(&level->sprites.at(dspritesequence.first_frame + j));
    }
}

void tr::loader::load_rooms()
{
    tr::room_loader::params params;
    params.num_rooms = num_rooms;
    params.rooms_offset = rooms_offset;
    params.num_static_meshes = num_static_meshes;
    params.static_meshes_offset = static_meshes_offset;

    tr::room_loader room_loader;
    room_loader.load(fp, level.get(), version, params);
}

void tr::loader::load_objects()
{
    struct d_object
    {
        uint16_t id;
        uint16_t room;

        glm::vec3 position;
        uint16_t orientation;

        uint16_t light_intensity;
        uint16_t flags;
    };

    fseek(fp, objects_offset, SEEK_SET);
    for (long i = 0; i < num_objects; ++i) {
        d_object dobject;
        dobject.id = read16(fp);
        dobject.room = read16(fp);
        dobject.position.x = read32(fp);
        dobject.position.y = read32(fp);
        dobject.position.z = read32(fp);
        dobject.orientation = read16(fp);
        dobject.light_intensity = read16(fp);
        if (version == tr::version_tr2)
            fseek(fp, 2, SEEK_CUR); // light_intensity2
        dobject.flags = read16(fp);

        for (const tr::model& model : level->models) {
            if (model.id == dobject.id) {
                tr::model_object modelobj(level.get(), &model);

                modelobj.room = &level->rooms.at(dobject.room);

                modelobj.transform =
                        glm::translate(glm::mat4(), dobject.position) *
                        glm::rotate(glm::mat4(), ((dobject.orientation >> 14) & 0xFF) * glm::pi<float>() / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));

                if (dobject.light_intensity == 0xFFFF)
                    modelobj.light_intensity = 1.0f;
                else
                    modelobj.light_intensity = 1.0f - dobject.light_intensity / 8191.0f;

                level->model_objects.push_back(modelobj);

                break;
            }
        }

        for (const tr::sprite_sequence& sequence : level->sprite_sequences) {
            if (sequence.id == dobject.id) {
                tr::sprite_object spriteobj;

                spriteobj.sequence = &sequence;
                spriteobj.frame = 0;

                spriteobj.room = &level->rooms.at(dobject.room);

                spriteobj.position = dobject.position;

                if (dobject.light_intensity == 0xFFFF)
                    spriteobj.light_intensity = 1.0f;
                else
                    spriteobj.light_intensity = 1.0f - dobject.light_intensity / 8191.0f;

                level->sprite_objects.push_back(spriteobj);

                break;
            }
        }
    }
}
