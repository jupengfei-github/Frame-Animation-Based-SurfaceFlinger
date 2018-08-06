#include "FramePlayer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <SkBitmap.h>
#include <SkImage.h>
#include <SkStream.h>
#include <SkData.h>
#pragma GCC diagnostic pop

namespace frame_animation {

void FramePlayer::start () {
	request_stop = false;
}

void FramePlayer::pause () {
	request_stop = true;
}

void FramePlayer::stop () {
	request_exit = true;
}

void FramePlayer::animation_thread (FramePlayer* const player) {
	shared_ptr<FrameInfo> info = player->frame_info;

	int frame_rate = info->cur_rate();
	int frame_time = 1000 / frame_rate;

	FrameMode mode = info->cur_mode();
	int frame_cnt  = info->cur_max_count();

	player->init_frame(info);

	int idx = 0;
	do {
		const long now = ns2ms(systemTime());
		if (info->cur_idx() >= frame_cnt) {
			if (mode == FRAME_MODE_REPEATE) {
				idx = 0;
				info->reset_frame();
			}
			else
				break;
		}

		if (!player->request_stop)
			player->flush_frame(info->next_frame(), idx++);

		const long sleepTime = ns2ms(systemTime()) - now;
		if (sleepTime > 0)
			sleep(sleepTime);
	} while (!player->request_exit);

	player->unint_frame(info);
}

// -----------------------------------------------------------------------
void SkiaPlayer::init_frame (const shared_ptr<FrameInfo>& info) {
	int frame_cnt = info->cur_max_count();
	for (int i = 0; i < frame_cnt; i++) {
		char *buf = nullptr;

		shared_ptr<istream> is = info->next_frame();
		is->seekg(0, ios_base::end);
		size_t len = is->tellg();
		is->seekg(0, ios_base::beg);

		buf = static_cast<char*>(malloc(sizeof(char)*len));
		if (buf == nullptr) {
			FPLog.E()<<"SkiaPlayer malloc buffer fail"<<endl;
			continue;
		}

		if (is->read(buf, len)) {
			FPLog.E()<<"SkiaPlayer read fail"<<endl;
			free(buf);
			continue;
		}

		SkBitmap bitmap;
		sk_sp<SkData>  data  = SkData::MakeWithoutCopy(buf, len);
		sk_sp<SkImage> image = SkImage::MakeFromEncoded(data);
		image->asLegacyBitmap(&bitmap, SkImage::kRO_LegacyBitmapMode);
		free(buf);

		frames.push_back(bitmap);
	}
}

void SkiaPlayer::flush_frame(shared_ptr<istream> in __unused, int idx __unused) const {

}

void SkiaPlayer::unint_frame (const shared_ptr<FrameInfo>& info __unused) {
	for (list<SkBitmap>::iterator it = frames.begin(); it != frames.end(); it++)
		it->reset();
}

// ----------------------------------------------------------------------
GLPlayer::GLPlayer (shared_ptr<FrameInfo> info):FramePlayer(info) {
	const EGLint display_attrs[] = {
		EGL_RED_SIZE,   8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE,  8,
		EGL_DEPTH_SIZE, 0,
		EGL_NONE,
	};

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(display, 0, 0);

	EGLConfig config;
	EGLint num_config;
	eglChooseConfig(display, display_attrs, &config, 1, &num_config);
	surface = eglCreateWindowSurface(display, config, nullptr, nullptr);
	context = eglCreateContext(display, config, nullptr, nullptr);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
		FPLog.E()<<"eglMakeCurrent fail"<<endl;
}

void GLPlayer::init_frame (const shared_ptr<FrameInfo>& info) {
	char *buf = nullptr;
	int max_frames = info->cur_max_count();

	for (int i = 0; i < max_frames; i++) {
		shared_ptr<istream> is = info->next_frame();
		is->seekg(0, ios_base::end);	
		size_t size = is->tellg();
		is->seekg(0, ios_base::beg);

		buf = static_cast<char*>(malloc(size * sizeof(char)));
		if (buf == nullptr) {
			FPLog.E()<<"GLPlayer init_frame malloc fail"<<endl;
			continue;
		}

		if (is->read(buf, size)) {
			FPLog.E()<<"GLPlayer read fail"<<endl;
			free(buf);
			continue;
		}

		SkBitmap bitmap;
		sk_sp<SkData> data = SkData::MakeWithoutCopy(buf, size);
		sk_sp<SkImage> image = SkImage::MakeFromEncoded(data);
		if (!image.get()) {
			FPLog.E()<<"MakeFromEncoded : "<<i<<" fail"<<endl;
			free(buf);
			continue;
		}

		if (!image->asLegacyBitmap(&bitmap, SkImage::kRO_LegacyBitmapMode)) {
			FPLog.E()<<"asLegacyBitmap : "<<i<<" fail"<<endl;
			free(buf);
			continue;
		}
		free(buf);

		const int w = bitmap.width();
		const int h = bitmap.height();
		const void *p = bitmap.getPixels();

		GLuint texture_id;
		glGenTextures(1, &texture_id);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		switch (bitmap.colorType()) {
			case kAlpha_8_SkColorType:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, p);
				break;
			case kARGB_4444_SkColorType:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_4_4_4_4, p);
				break;
			case kN32_SkColorType:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE ,p);
				break;
			case kRGB_565_SkColorType:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, p);
				break;
			default:
				break;
		}

		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glBindTexture(GL_TEXTURE_2D, 0);
		frame_names.push_back(texture_id);		
	}
}

void GLPlayer::flush_frame(shared_ptr<istream> in __unused, int idx) const {
	glBindTexture(GL_TEXTURE_2D, frame_names[idx]);
	eglSwapBuffers(display, surface);
}

void GLPlayer::unint_frame (const shared_ptr<FrameInfo>& info __unused) {
	for (vector<GLuint>::iterator it = frame_names.begin(); it != frame_names.end(); it++)
		glDeleteTextures(1, &(*it));

	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(display, context);
	eglDestroySurface(display, surface);
	eglTerminate(display);
}

}; //namespace frame_animation
