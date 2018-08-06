#include <SkImage.h>
#include <SkStream.h>
#include <SkData.h>
#include <SkPaint.h>
#include <SkImageInfo.h>

#include <ui/PixelFormat.h>
#include <ui/DisplayInfo.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#include <android/graphics/GraphicsJNI.h>

#include "FramePlayer.h"

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

bool FramePlayer::init_display_surface () {
	DisplayInfo dinfo;

	FPLog.I()<<"init_display_surface 1"<<endl;
	sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain));

	FPLog.I()<<"init_display_surface 2"<<endl;
	status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
	if (status) {
		FPLog.E()<<"animation_thread getDisplayInfo fail"<<endl;
		return false;
	}

	FPLog.I()<<"init_display_surface 3"<<endl;
	unique_ptr<SurfaceComposerClient> session(new SurfaceComposerClient());
	if (!session.get()) {
		FPLog.E()<<"create SurfaceComposerClient fail"<<endl;
		return false;
	}

	FPLog.I()<<"init_display_surface 4"<<endl;
	control = session->createSurface(String8("frameAnimation"), dinfo.w, dinfo.h, PIXEL_FORMAT_RGBX_8888);

	FPLog.I()<<"init_display_surface 5"<<endl;
	surface = control->getSurface();
	FPLog.E()<<"create Surface width : "<<dinfo.w<<" height : "<<dinfo.h<<endl;

	return true;
}

void FramePlayer::unint_display_surface () {
	surface.clear();
	control.clear();
}

void FramePlayer::animation_thread (FramePlayer* const player) {
	shared_ptr<FrameInfo> info = player->frame_info;
	int frame_time = 1000 / info->cur_rate();
	FrameMode mode = info->cur_mode();
	int frame_cnt  = info->cur_max_count();

	FPLog.I()<<"animation_thread started"<<endl;
	if (!player->init_display_surface()) {
		FPLog.E()<<"init_display_surface fail"<<endl;
		return;
	}

	if (!player->init_frame(info)) {
		player->unint_display_surface();
		FPLog.E()<<"init_frame fail"<<endl;
		return;
	}

	int idx = 0;
	bool exit;
	while (!player->request_exit) {
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
			exit = player->flush_frame(info->next_frame(), idx++);

		if (exit)
			break;

		const long sleepTime = ns2ms(systemTime()) - now;
		if (sleepTime > 0)
			sleep(sleepTime);
	}

	FPLog.I()<<"animation_thread stoped"<<endl;
	player->unint_frame(info);
	player->unint_display_surface();
}

// -----------------------------------------------------------------------
bool SkiaPlayer::init_frame (const shared_ptr<FrameInfo>& frame_info) {
	ANativeWindow_Buffer buffer;
	status_t err = surface->lock(&buffer, nullptr);
	if (err < 0) {
		FPLog.E()<<"Surface lock buffer fail"<<endl;
		return false;
	}

	SkImageInfo info = SkImageInfo::Make(buffer.width, buffer.height,
		convertPixelFormat(buffer.format),
		buffer.format == PIXEL_FORMAT_RGBX_8888? kOpaque_SkAlphaType : kPremul_SkAlphaType,
		GraphicsJNI::defaultColorSpace());
	SkBitmap bitmap;
	ssize_t bpr = buffer.stride * bytesPerPixel(buffer.format);
	bitmap.setInfo(info, bpr);
	if (buffer.width > 0 && buffer.height > 0)
		bitmap.setPixels(buffer.bits);
	else
		bitmap.setPixels(nullptr);
	canvas = auto_ptr<SkCanvas>(new SkCanvas(bitmap));

	int frame_cnt = frame_info->cur_max_count();
	for (int i = 0; i < frame_cnt; i++) {
		char *buf = nullptr;

		shared_ptr<istream> is = frame_info->next_frame();
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

	return true;
}

bool SkiaPlayer::flush_frame(shared_ptr<istream> in __unused, int idx) const {
	SkPaint paint;
	const SkBitmap bitmap = frames[idx];

	canvas->drawBitmap(bitmap, 0, 0, &paint);
	surface->unlockAndPost();
	return true;
}

void SkiaPlayer::unint_frame (const shared_ptr<FrameInfo>& info __unused) {
	for (vector<SkBitmap>::iterator it = frames.begin(); it != frames.end(); it++)
		it->reset();
}

inline SkColorType SkiaPlayer::convertPixelFormat (PixelFormat format) {
	switch (format) {
		case PIXEL_FORMAT_RGBX_8888: return kN32_SkColorType;
		case PIXEL_FORMAT_RGBA_8888: return kN32_SkColorType;
		case PIXEL_FORMAT_RGBA_FP16: return kRGBA_F16_SkColorType;
		case PIXEL_FORMAT_RGB_565:   return kRGB_565_SkColorType;
		default:                     return kUnknown_SkColorType;
	}
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

	EGLint num_config;
	eglChooseConfig(display, display_attrs, &config, 1, &num_config);
}

bool GLPlayer::init_frame (const shared_ptr<FrameInfo>& info) {
	char *buf = nullptr;
	int max_frames = info->cur_max_count();

	egl_surface = eglCreateWindowSurface(display, config, surface.get(), nullptr);
	context = eglCreateContext(display, config, nullptr, nullptr);
	if (eglMakeCurrent(display, egl_surface, egl_surface, context) == EGL_FALSE) {
		FPLog.E()<<"eglMakeCurrent fail"<<endl;
		return false;
	}

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

	return true;
}

bool GLPlayer::flush_frame(shared_ptr<istream> in __unused, int idx) const {
	glBindTexture(GL_TEXTURE_2D, frame_names[idx]);
	eglSwapBuffers(display, egl_surface);
	return true;
}

void GLPlayer::unint_frame (const shared_ptr<FrameInfo>& info __unused) {
	for (vector<GLuint>::iterator it = frame_names.begin(); it != frame_names.end(); it++)
		glDeleteTextures(1, &(*it));

	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(display, context);
	eglDestroySurface(display, egl_surface);
	eglTerminate(display);
}

}; //namespace frame_animation
