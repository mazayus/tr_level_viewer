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

#include <SDL2/SDL.h>

#include <math.h>
#include <stdio.h>

#include <string>

#include "camera.h"
#include "renderer.h"
#include "tr_types.h"

static bool SYS_ParseOptions(int argc, char* argv[]);
static void SYS_PrintUsageInfo();

static bool SYS_Init();
static bool SYS_Frame();
static void SYS_Shutdown();

static SDL_Window* window = nullptr;
static SDL_GLContext glcontext = nullptr;

static Renderer* renderer = nullptr;
static Renderer::FrameInfo frameinfo;

static Camera camera;

static struct {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
} inputstate;

static struct {
    std::string level;
    tr::version version = tr::version_invalid;
    bool debug_draw_all_meshes = false;
    bool debug_draw_all_sprites = false;
} cmdopts;

int main(int argc, char* argv[])
{
    if (!SYS_ParseOptions(argc, argv)) {
        SYS_PrintUsageInfo();
        return 1;
    }

    if (!SYS_Init())
        return 1;

    renderer = new Renderer();

    camera.SetPerspective(M_PI/3.0f, 1366.0f/768.0f, 10.0f, 1000000.0f);
    camera.SetTransform(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);

    std::unique_ptr<tr::level> level = tr::level::load(cmdopts.level.c_str(), cmdopts.version);
    renderer->RegisterLevel(*level);

    // TODO: don't add altrooms to the render list
    for (tr::room& room : level->rooms)
        frameinfo.rooms.push_back(&room);
    for (tr::model_object& modelobj : level->model_objects)
        frameinfo.model_objects.push_back(&modelobj);
    for (tr::sprite_object& spriteobj : level->sprite_objects)
        frameinfo.sprite_objects.push_back(&spriteobj);

    for (const tr::model_object& modelobj : level->model_objects) {
        if (modelobj.model->id == 0) {
            // TODO: set camera orientation
            glm::vec3 position = glm::vec3(modelobj.transform[3]) + glm::vec3(0.0f, -1024.0f, 0.0f);
            camera.SetTransform(position, 0.0f, 0.0f);
        }
    }

    frameinfo.debug_draw_all_meshes = cmdopts.debug_draw_all_meshes;
    frameinfo.debug_draw_all_sprites = cmdopts.debug_draw_all_sprites;

    // TODO: implement framerate-independent main loop
    long last_frame_ticks = SDL_GetTicks();
    float texanim_time = 0;
    while (SYS_Frame()) {
        long cur_frame_ticks = SDL_GetTicks();
        float dt = (cur_frame_ticks - last_frame_ticks) / 1000.0f;
        last_frame_ticks = cur_frame_ticks;

        // TODO: move this to tr::level?
        texanim_time += dt;
        if (texanim_time >= 0.1f) {
            texanim_time -= 0.1f;
            for (tr::room& room : level->rooms) {
                bool updated = false;
                for (tr::mesh_poly& polygon : room.geometry.polys) {
                    if (polygon.texinfo->texanimchain) {
                        updated = true;
                        polygon.texinfo = polygon.texinfo->texanimchain;
                    }
                }
                // TODO: reupload only updated polygons
                if (updated)
                    renderer->NotifyRoomMeshUpdated(room.geometry);
            }
        }
        for (tr::model_object* modelobj: frameinfo.model_objects)
            modelobj->tick(dt);
    }

    delete renderer;

    SYS_Shutdown();
    return 0;
}

bool SYS_ParseOptions(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.empty()) {
            continue;
        } else if (arg == "-debug_draw_all_meshes") {
            cmdopts.debug_draw_all_meshes = true;
        } else if (arg == "-debug_draw_all_sprites") {
            cmdopts.debug_draw_all_sprites = true;
        } else if (arg == "-tr1") {
            if (cmdopts.version == tr::version_invalid) {
                cmdopts.version = tr::version_tr1;
            } else {
                return false;
            }
        } else if (arg == "-tr2") {
            if (cmdopts.version == tr::version_invalid) {
                cmdopts.version = tr::version_tr2;
            } else {
                return false;
            }
        } else if (arg[0] != '-') {
            // level name
            if (cmdopts.level.empty()) {
                cmdopts.level = arg;
            } else {
                return false;
            }
        } else {
            // unknown option
            return false;
        }
    }

    if (cmdopts.level.empty())
        return false;
    if (cmdopts.version == tr::version_invalid)
        return false;

    return true;
}

void SYS_PrintUsageInfo()
{
    fprintf(stderr, "usage: ./tr_level_viewer {-tr1|-tr2} [OPTION]... LEVEL\n\n");
    fprintf(stderr, "OPTIONS\n");
    fprintf(stderr, "  -debug_draw_all_meshes\n");
    fprintf(stderr, "  -debug_draw_all_sprites\n");
    fprintf(stderr, "\n");
}

bool SYS_Init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);

    window = SDL_CreateWindow("TR Level Viewer", 0, 0, 1366, 768, SDL_WINDOW_OPENGL);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SYS_Shutdown();
        return false;
    }

    SDL_SetRelativeMouseMode(SDL_TRUE);

    glcontext = SDL_GL_CreateContext(window);
    if (!glcontext) {
        fprintf(stderr, "SDL_GL_CreateContext: %s\n", SDL_GetError());
        SYS_Shutdown();
        return false;
    }

    return true;
}

bool SYS_Frame()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE: return false;
                case SDLK_w: inputstate.up = true; break;
                case SDLK_s: inputstate.down = true; break;
                case SDLK_a: inputstate.left = true; break;
                case SDLK_d: inputstate.right = true; break;
            }
        } else if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
                case SDLK_w: inputstate.up = false; break;
                case SDLK_s: inputstate.down = false; break;
                case SDLK_a: inputstate.left = false; break;
                case SDLK_d: inputstate.right = false; break;
            }
        } else if (event.type == SDL_MOUSEMOTION) {
            float dyaw = -event.motion.xrel/1000.0f;
            float dpitch = -event.motion.yrel/1000.0f;
            camera.Look(dyaw, dpitch);
        }
    }

    float speed = (SDL_GetModState() & KMOD_SHIFT) ? 1000.0f : 100.0f;
    float forward = speed * (inputstate.up - inputstate.down);
    float right = speed * (inputstate.right - inputstate.left);
    camera.Move(forward, right);

    frameinfo.projection_matrix = camera.ProjectionMatrix();
    frameinfo.view_matrix = camera.ViewMatrix();
    renderer->RenderFrame(frameinfo);

    SDL_GL_SwapWindow(window);

    return true;
}

void SYS_Shutdown()
{
    if (glcontext) {
        SDL_GL_DeleteContext(glcontext);
        glcontext = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}
