/*
 * Copyright (C) 2018-2024 The Surface Frame-Animation Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _FRAME_PLAYER_H_
#define _FRAME_PLAYER_H_

#include <iostream>
#include <thread>
#include <list>

#include <GLES3/gl3.h>
#include <EGL/egl.h>

#include <SkCanvas.h>
#include <SkBitmap.h>
#include <gui/Surface.h>
#include <gui/SurfaceControl.h>

#include "FrameInfo.h"

using namespace std;

namespace frame_animation {

class FramePlayer : public thread {
    bool request_exit;
    bool request_stop;

    static void animation_thread (FramePlayer* const player);
    bool init_display_surface();
    void unint_display_surface();
public:
    FramePlayer (shared_ptr<FrameInfo> info):thread(bind(animation_thread, this)) {
        request_stop = false;
        request_exit = false;

        xoff = yoff = 0;
        this->info  = info;
    }
    virtual ~FramePlayer() {}

    void start();
    void stop();
    void pause();
protected:
    sp<Surface> surface;
    sp<SurfaceControl> control;
    shared_ptr<FrameInfo> info;
    int width, height;
    int xoff, yoff;

    virtual bool init_frame() = 0;
    virtual bool flush_frame(int) const = 0;
    virtual void unint_frame() = 0;
};

// ------------------------------------
struct SkiaPlayer : public FramePlayer {
    SkiaPlayer (shared_ptr<FrameInfo> info):FramePlayer(info) {}
    virtual ~SkiaPlayer() {}    
protected:
    virtual bool init_frame ();
    virtual bool flush_frame(int) const;
    virtual void unint_frame ();
private:
    static inline SkColorType convertPixelFormat (PixelFormat format);

    vector<SkBitmap> frames;
};

// -----------------------------------
struct GLPlayer : public FramePlayer {
    GLPlayer(shared_ptr<FrameInfo> info):FramePlayer(info) {}
    virtual ~GLPlayer() {}
protected:
    virtual bool init_frame ();
    virtual bool flush_frame(int) const;
    virtual void unint_frame ();
private:
    static const GLchar* VERTEX_STR;
    static const GLchar* FRAGMENT_STR;

    vector<GLuint> frame_textures;
    vector<pair<int, int>> frame_size;
    EGLDisplay egl_display;
    EGLDisplay egl_context;
    EGLDisplay egl_surface;
    EGLConfig  egl_config;

    GLuint vShader;
    GLuint fShader;
    GLuint program;

    static const GLfloat vertex_position[12];
    static const GLfloat texture_position[8];

    GLuint loadShader (GLenum type, const char* shader);
};

}; //namespace frame_animation

#endif
