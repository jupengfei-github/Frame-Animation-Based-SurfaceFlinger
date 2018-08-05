#ifndef _FRAME_PLAYER_H_
#define _FRAME_PLAYER_H_

#include <iostream>
#include <thread>
#include <list>

#include <GLES/gl.h>
#include <EGL/egl.h>

#include "FrameInfo.h"

using namespace std;

class SkBitmap;
namespace frame_animation {

class FramePlayer : public thread {
	shared_ptr<FrameInfo> frame_info;
	bool request_exit;
	bool request_stop;

	static void animation_thread (FramePlayer *player);
public:
	FramePlayer (shared_ptr<FrameInfo> info) {
		request_stop = true;
		frame_info   = info;
	}
	virtual ~FramePlayer() {}

	void start();
	void stop();
	void pause();
protected:
	virtual void init_frame(const shared_ptr<FrameInfo>&) = 0;
	virtual void flush_frame(shared_ptr<istream>, int) const = 0;
	virtual void unint_frame(const shared_ptr<FrameInfo>&) = 0;
};

// ------------------------------------
struct SkiaPlayer : public FramePlayer {
	virtual ~SkiaPlayer() {}	
protected:
	virtual void init_frame (const shared_ptr<FrameInfo>&);
	virtual void flush_frame(shared_ptr<istream>, int) const;
	virtual void unint_frame (const shared_ptr<FrameInfo>&);
private:
	list<SkBitmap> frames;
};

// -----------------------------------
struct GLPlayer : public FramePlayer {
	GLPlayer(shared_ptr<FrameInfo> info);
	virtual ~GLPlayer() {}
protected:
	virtual void init_frame (const shared_ptr<FrameInfo>&);
	virtual void flush_frame(shared_ptr<istream>, int) const;
	virtual void unint_frame (const shared_ptr<FrameInfo>&);
private:
	vector<GLuint> frame_names;
	EGLDisplay display;
	EGLDisplay context;
	EGLDisplay surface;
};

}; //namespace frame_animation

#endif
