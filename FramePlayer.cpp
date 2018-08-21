#include <SkImage.h>
#include <SkStream.h>
#include <SkData.h>
#include <SkPaint.h>
#include <SkImageInfo.h>
#include <SkStream.h>

#include <ui/PixelFormat.h>
#include <ui/DisplayInfo.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#include <android/graphics/GraphicsJNI.h>

#include "FramePlayer.h"
#include "FrameError.h"

namespace frame_animation {

class SkStreamAdapter : public SkStream {
	shared_ptr<istream> instream;
public:
	SkStreamAdapter (shared_ptr<istream> in):instream(in) {}

	virtual size_t read (void *buf, size_t len) {
		instream->read(static_cast<char*>(buf), len);			
		return instream->gcount();
	}

	virtual bool isAtEnd () const {
		return instream->eof();
	}

	static std::unique_ptr<SkStreamAsset> MakeFromFile (const char path[] __unused) {
		throw io_exception("Unsupported Operation");
	}
};

// --------------------------------------------------
void FramePlayer::start () {
	request_stop = false;
}

void FramePlayer::pause () {
	request_stop = true;
}

void FramePlayer::stop () {
	request_exit = true;
}

/* apply Surface from SurfaceFlinger */
bool FramePlayer::init_display_surface () {
	DisplayInfo dinfo;

	sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain));
	status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
	if (status) {
		FPLog.E()<<"getDisplayInfo fail"<<endl;
		return false;
	}

	sp<SurfaceComposerClient> session = new SurfaceComposerClient();
	if (!session.get()) {
		FPLog.E()<<"create SurfaceComposerClient fail"<<endl;
		return false;
	}

	control = session->createSurface(String8("frameAnimation"), dinfo.w, dinfo.h, PIXEL_FORMAT_RGB_565);
	if (!control.get()) {
		FPLog.E()<<"createSurface fail"<<endl;
		return false;
	}

	surface = control->getSurface();
	width  = dinfo.w;
	height = dinfo.h;
	FPLog.E()<<"create Surface width : "<<dinfo.w<<" height : "<<dinfo.h<<endl;

	SurfaceComposerClient::openGlobalTransaction(); 
	control->setLayer(0x40000000);
	SurfaceComposerClient::closeGlobalTransaction();

	return true;
}

void FramePlayer::unint_display_surface () {
	surface.clear();
	control.clear();
}

void FramePlayer::animation_thread (FramePlayer* const player) {
	shared_ptr<FrameInfo> info = player->info;
	int idx = 0, step = 1;
	bool exit;

	int frame_cnt  = info->count();
	int frame_time = 1000 / info->rate();
	AnimMode mode  = info->mode();

	FPLog.I()<<"animation_thread started"<<endl;
	if (!player->init_display_surface())
		return;

	if (!player->init_frame()) {
		player->unint_display_surface();
		return;
	}

	while (!player->request_exit) {
		const long now = ns2ms(systemTime());

		if (idx >= frame_cnt) {
			if (mode == FRAME_MODE_REPEATE)
				idx = 0;
			else if (mode == FRAME_MODE_REVERSE)
				step = step == 1? -1 : 1;
			else
				break;
		}

		if (!player->request_stop) {
			idx += step;
			exit = player->flush_frame(idx);
		}

		if (!exit)
			break;

		const long sleepTime = ns2ms(systemTime()) - now;
		if (sleepTime > 0)
			sleep(sleepTime);
	}

	FPLog.I()<<"animation_thread stoped"<<endl;
	player->unint_frame();
	player->unint_display_surface();
}

// -----------------------------------------------------------------------
bool SkiaPlayer::init_frame () {
	ANativeWindow_Buffer buffer;

	status_t err = surface->lock(&buffer, nullptr);
	if (err < 0) {
		FPLog.E()<<"Surface lock buffer fail"<<endl;
		return false;
	}

	Size s = info->size();
	xoff = max(0, (width  - s.width)) / 2;
	yoff = max(0, (height - s.height)) / 2;

	SkImageInfo image_info = SkImageInfo::Make(buffer.width, buffer.height,
		convertPixelFormat(buffer.format),
		buffer.format == PIXEL_FORMAT_RGBX_8888? kOpaque_SkAlphaType : kPremul_SkAlphaType,
		GraphicsJNI::defaultColorSpace());

	SkBitmap bitmap;
	ssize_t bpr = buffer.stride * bytesPerPixel(buffer.format);
	bitmap.setInfo(image_info, bpr);
	if (buffer.width > 0 && buffer.height > 0)
		bitmap.setPixels(buffer.bits);
	else
		bitmap.setPixels(nullptr);
	canvas = auto_ptr<SkCanvas>(new SkCanvas(bitmap));

	int frame_cnt = info->count();
	for (int i = 0; i < frame_cnt; i++) {
		shared_ptr<istream> is = info->frame(i);
		if (!is.get() || !is->good())
			continue;

		is->seekg(0, ios_base::end);
		size_t len = is->tellg();
		is->seekg(0, ios_base::beg);

		SkBitmap bitmap;
		SkStreamAdapter adapter(is);
		sk_sp<SkData> data = SkData::MakeFromStream(&adapter, len);

		sk_sp<SkImage> image = SkImage::MakeFromEncoded(data);
		image->asLegacyBitmap(&bitmap, SkImage::kRO_LegacyBitmapMode);

		frames.push_back(bitmap);
	}

	return true;
}

bool SkiaPlayer::flush_frame(int idx) const {
	SkPaint paint;
	const SkBitmap bitmap = frames[idx];

	Size s = info->size();
	int x = xoff + max(0, (s.width - bitmap.width())) / 2;
	int y = yoff + max(0, (s.height - bitmap.height())) / 2;

	canvas->drawBitmap(bitmap, x, y, &paint);
	surface->unlockAndPost();
	return true;
}

void SkiaPlayer::unint_frame () {
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

	egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(egl_display, 0, 0);

	EGLint num_config;
	eglChooseConfig(egl_display, display_attrs, &egl_config, 1, &num_config);
}

bool GLPlayer::init_frame () {
	char *buf = nullptr;
	int max_frames = info->count();

	egl_surface = eglCreateWindowSurface(egl_display, egl_config, nullptr, nullptr);
	egl_context = eglCreateContext(egl_display, egl_config, nullptr, nullptr);
	if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) == EGL_FALSE) {
		FPLog.E()<<"eglMakeCurrent fail"<<endl;
		return false;
	}

	for (int i = 0; i < max_frames; i++) {
		shared_ptr<istream> is = info->frame(i);
		if (!is.get())
			continue;

		is->seekg(0, ios_base::end);	
		size_t size = is->tellg();
		is->seekg(0, ios_base::beg);

		SkBitmap bitmap;
		SkStreamAdapter adapter(is);
		sk_sp<SkData> data = SkData::MakeFromStream(&adapter, size);
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
		frame_textures.push_back(texture_id);
	}

	return true;
}

bool GLPlayer::flush_frame(int idx) const {
	glBindTexture(GL_TEXTURE_2D, frame_textures[idx]);
	eglSwapBuffers(egl_display, egl_surface);
	return true;
}

void GLPlayer::unint_frame () {
	for (vector<GLuint>::iterator it = frame_textures.begin(); it != frame_textures.end(); it++)
		glDeleteTextures(1, &(*it));

	eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(egl_display, egl_context);
	eglDestroySurface(egl_display, egl_surface);
	eglTerminate(egl_display);
}

}; //namespace frame_animation
