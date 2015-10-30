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

#include "tr_types.h"

#include "tr_loader.h"

#include <glm/gtc/matrix_transform.hpp>

static glm::quat AxisAngleToQuaternion(glm::vec3 axis, float angle)
{
    float s = glm::sin(angle/2), c = glm::cos(angle/2);
    return glm::quat(c, s*axis.x, s*axis.y, s*axis.z);
}

static glm::quat EulerAnglesToQuaternion(glm::vec3 angles)
{
    float sx = glm::sin(angles.x/2), sy = glm::sin(angles.y/2), sz = glm::sin(angles.z/2),
          cx = glm::cos(angles.x/2), cy = glm::cos(angles.y/2), cz = glm::cos(angles.z/2);
    float sxsy = sx * sy, cxcy = cx * cy,
          sxcy = sx * cy, cxsy = cx * sy;
    return glm::quat(
        sxsy*sz + cxcy*cz,
        sxcy*cz + cxsy*sz,
        cxsy*cz - sxcy*sz,
        cxcy*sz - sxsy*cz
    );
}

tr::model_object::model_object(const tr::level* level, const tr::model* model) :
    model(model), level(level)
{
    animation = model->animation;
    anim_tick = animation->first_tick;
    anim_tick_time = 0;
    update_node_transforms();
}

void tr::model_object::tick(float dt)
{
    anim_tick_time += dt;
    if (anim_tick_time >= 1.0f / 30.0f) {
        anim_tick_time -= 1.0f / 30.0f;
        ++anim_tick;
        if (anim_tick > animation->last_tick)
            anim_tick = animation->first_tick;
    }

    update_node_transforms();
}

void tr::model_object::update_node_transforms()
{
    tr::anim_frame af = smooth_anim_frame();

    node_transforms.clear();
    for (size_t i = 0; i < model->nodes.size(); ++i) {
        glm::mat4 transform;
        if (i == 0)
            transform = glm::translate(glm::mat4(), af.translation);
        else
            transform = node_transforms.at(model->nodes[i].parent);

        glm::mat4 translation = glm::translate(glm::mat4(), model->nodes[i].offset);
        transform = transform * translation;

        glm::mat4 rotation = glm::mat4_cast(af.rotation[i]);
        transform = transform * rotation;

        node_transforms.push_back(transform);
    }
}

tr::anim_frame tr::model_object::smooth_anim_frame() const
{
    int frame = (anim_tick - animation->first_tick) / animation->ticks_per_frame;
    int num_frames = (animation->last_tick - animation->first_tick) / animation->ticks_per_frame + 1;

    ulong offset = animation->frame_offset;
    for (int i = 0; i < frame; ++i)
        offset += level->anim_frame_data.at(offset) + 1;
    tr::anim_frame af0 = parse_anim_frame(offset);
    offset += level->anim_frame_data.at(offset) + 1;
    if (frame >= num_frames - 1)
        offset = animation->frame_offset;
    tr::anim_frame af1 = parse_anim_frame(offset);

    ushort cur_frame_tick = (anim_tick - animation->first_tick) % animation->ticks_per_frame;
    float alpha = (cur_frame_tick + anim_tick_time * 30.0f) / animation->ticks_per_frame;
    tr::anim_frame af;
    af.translation = glm::mix(af0.translation, af1.translation, alpha);
    for (size_t i = 0; i < model->nodes.size(); ++i)
        af.rotation[i] = glm::slerp(af0.rotation[i], af1.rotation[i], alpha);
    return af;
}

tr::anim_frame tr::model_object::parse_anim_frame(ulong offset) const
{
    static const float CONVERSION_FACTOR = glm::pi<float>() / 2.0f / 0x100;

    tr::anim_frame af;

    long frame_size = (uint16_t)level->anim_frame_data.at(offset++);

    offset += 6; // skip bounding box
    frame_size -= 6;

    af.translation.x = (int16_t)level->anim_frame_data.at(offset++);
    af.translation.y = (int16_t)level->anim_frame_data.at(offset++);
    af.translation.z = (int16_t)level->anim_frame_data.at(offset++);
    frame_size -= 3;

    for (size_t i = 0; i < model->nodes.size(); ++i) {
        assert(frame_size > 0);
        uint16_t tmp1 = level->anim_frame_data.at(offset++);
        --frame_size;

        if ((tmp1 & 0xC000) == 0) {
            assert(frame_size > 0);
            uint16_t tmp2 = level->anim_frame_data.at(offset++);
            --frame_size;

            glm::vec3 angles = CONVERSION_FACTOR * glm::vec3(
                (float)((tmp1 & 0x3ff0) >> 4),
                (float)(((tmp1 & 0x000f) << 6) | ((tmp2 & 0xfc00) >> 10)),
                (float)(tmp2 & 0x03ff)
            );
            af.rotation[i] = EulerAnglesToQuaternion(angles);
        } else {
            glm::vec3 axis = glm::vec3(
                (float)((tmp1 & 0xC000) == 0x4000),
                (float)((tmp1 & 0xC000) == 0x8000),
                (float)((tmp1 & 0xC000) == 0xC000)
            );
            float angle = CONVERSION_FACTOR * (tmp1 & 0x03FF);
            af.rotation[i] = AxisAngleToQuaternion(axis, angle);
        }
    }

    return af;
}

std::unique_ptr<tr::level> tr::level::load(const char* filename, tr::version version)
{
    return tr::loader::load(filename, version);
}
