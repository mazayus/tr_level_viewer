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

#include "camera.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

/*
 * Camera
 */

Camera::Camera()
    : position(0.0f, 0.0f, 0.0f), yaw(0), pitch(0)
{

}

void Camera::SetPerspective(float fovy, float aspect, float znear, float zfar)
{
    projection_matrix = glm::perspective(fovy, aspect, znear, zfar);
}

void Camera::SetTransform(glm::vec3 position, float yaw, float pitch)
{
    const float epsilon = 1e-3f;
    const float pi = glm::pi<float>();

    yaw = glm::mod(yaw, pi*2.0f);
    pitch = glm::clamp(pitch, -pi/2.0f + epsilon, pi/2.0f - epsilon);

    this->position = position;
    this->yaw = yaw;
    this->pitch = pitch;

    glm::vec3 forward = glm::vec3(
        - glm::cos(pitch) * glm::sin(-yaw),
        - glm::sin(pitch),
        - glm::cos(pitch) * glm::cos(-yaw)
    );
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, -1.0f, 0.0f)));
    glm::vec3 up = glm::cross(right, forward);

    view_matrix = glm::mat4(
        right.x,                    up.x,                    -forward.x,                    0.0f,
        right.y,                    up.y,                    -forward.y,                    0.0f,
        right.z,                    up.z,                    -forward.z,                    0.0f,
        -glm::dot(right, position), -glm::dot(up, position), -glm::dot(-forward, position), 1.0f
    );
}

glm::mat4 Camera::ProjectionMatrix() const
{
    return projection_matrix;
}

glm::mat4 Camera::ViewMatrix() const
{
    return view_matrix;
}

void Camera::Move(float forward_speed, float right_speed)
{
    glm::vec3 forward(-view_matrix[0][2], -view_matrix[1][2], -view_matrix[2][2]);
    glm::vec3 right(view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]);
    glm::vec3 delta_position = forward_speed * forward + right_speed * right;
    SetTransform(position + delta_position, yaw, pitch);
}

void Camera::Look(float delta_yaw, float delta_pitch)
{
    SetTransform(position, yaw + delta_yaw, pitch + delta_pitch);
}
