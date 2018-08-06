#ifndef _FRAME_PLAYER_H_
#define _FRAME_PLAYER_H_

#include <iostream>
#include <thread>
#include <list>

#include <GLES/gl.h>
#include <EGL/egl.h>

#include <SkCanvas.h>
#include <SkBitmap.h>
#include <gui/Surface.h>
#include <gui/SurfaceControl.h>

#include "FrameInfo.h"

using namespace std;

namespace frame_animation {

class FramePlayer : public thread {
	shared_ptr<FrameInfo> frame_info;
	bool request_exit;
	bool request_stop;

	static void animation_thread (FramePlayer* const player);
	bool init_display_surface();
	void unint_display_surface();
public:
	FramePlayer (shared_ptr<FrameInfo> info):thread(bind(animation_thread, this)) {
		request_stop = false;
		request_exit = false;
		frame_info   = info;
	}
	virtual ~FramePlayer() {}

	void start();
	void stop();
	void pause();
protected:
	sp<Surface> surface;
	sp<SurfaceControl> control;

	virtual bool init_frame(const shared_ptr<FrameInfo>&) = 0;
	virtual bool flush_frame(shared_ptr<istream>, int) const = 0;
	virtual void unint_frame(const shared_ptr<FrameInfo>&) = 0;
};

// ------------------------------------
struct SkiaPlayer : public FramePlayer {
	SkiaPlayer (shared_ptr<FrameInfo> info):FramePlayer(info) {}
	virtual ~SkiaPlayer() {}	
protected:
	virtual bool init_frame (const shared_ptr<FrameInfo>&);
	virtual bool flush_frame(shared_ptr<istream>, int) const;
	virtual void unint_frame (const shared_ptr<FrameInfo>&);
private:
	static inline SkColorType convertPixelFormat (PixelFormat format);

	vector<SkBitmap> frames;
	auto_ptr<SkCanvas> canvas;
};

// -----------------------------------
struct GLPlayer : public FramePlayer {
	GLPlayer(shared_ptr<FrameInfo> info);
	virtual ~GLPlayer() {}
protected:
	virtual bool init_frame (const shared_ptr<FrameInfo>&);
	virtual bool flush_frame(shared_ptr<istream>, int) const;
	virtual void unint_frame (const shared_ptr<FrameInfo>&);
private:
	vector<GLuint> frame_names;
	EGLDisplay display;
	EGLDisplay context;
	EGLDisplay egl_surface;
	EGLConfig  config;
};

}; //namespace frame_animation

#endif
