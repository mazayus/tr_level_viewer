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

#ifndef CAMERA_H
#define CAMERA_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

/*
 * Camera
 *
 * NOTE: x - right, y - down, z - out of the screen
 */

class Camera
{
public:
    Camera();

    void SetPerspective(float fovy, float aspect, float znear, float zfar);
    void SetTransform(glm::vec3 position, float yaw, float pitch);

    glm::mat4 ProjectionMatrix() const;
    glm::mat4 ViewMatrix() const;

    void Move(float forward_speed, float right_speed);
    void Look(float delta_yaw, float delta_pitch);

private:
    glm::mat4 projection_matrix;
    glm::mat4 view_matrix;

    glm::vec3 position;
    float yaw, pitch;
};

#endif
