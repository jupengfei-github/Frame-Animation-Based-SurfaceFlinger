#include <SkImage.h>
#include <SkStream.h>
#include <SkData.h>
#include <SkPaint.h>
#include <SkImageInfo.h>
#include <SkStream.h>
#include <SkColor.h>

#include <ui/PixelFormat.h>
#include <ui/DisplayInfo.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#include <android/graphics/GraphicsJNI.h>

#include "FramePlayer.h"
#include "FrameError.h"
#include "FrameDisplay.h"

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
	DisplayMetrics dm;

	sp<SurfaceComposerClient> session = new SurfaceComposerClient();
	if (!session.get()) {
		FPLog.E()<<"create SurfaceComposerClient fail"<<endl;
		return false;
	}

	control = session->createSurface(String8("frameAnimation"), dm.width(), dm.height(), PIXEL_FORMAT_RGBA_8888);
	if (!control.get()) {
		FPLog.E()<<"createSurface fail"<<endl;
		return false;
	}

	surface = control->getSurface();
	FPLog.E()<<"create Surface width : "<<dm.width()<<" height : "<<dm.height()<<endl;

	Size s = info->size();
	xoff = max(0, static_cast<int>(dm.width()  - s.width)) / 2;
	yoff = max(0, static_cast<int>(dm.height() - s.height)) / 2;

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

	int max_frames = info->count();
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

		if (idx >= max_frames) {
			if (mode == FRAME_MODE_REPEATE)
				idx = 0;
			else if (mode == FRAME_MODE_REVERSE)
				step = step == 1? -1 : 1;
			else
				break;
		}

		if (!player->request_stop) {
			exit = player->flush_frame(idx);
			idx += step;
		}

		if (!exit)
			break;

		const long sleepTime = ns2ms(systemTime()) - now;
		if (sleepTime > 0)
			usleep(sleepTime);
	}

	FPLog.I()<<"animation_thread stoped"<<endl;
	player->unint_frame();
	player->unint_display_surface();
}

// -----------------------------------------------------------------------
bool SkiaPlayer::init_frame () {
	int max_frames = info->count();

	for (int i = 0; i < max_frames; i++) {
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
	int max_frames = info->count();
	if (idx >= max_frames)
		return false;

	ANativeWindow_Buffer buffer;
	status_t err = surface->lock(&buffer, nullptr);
	if (err < 0) {
		FPLog.E()<<"flush_frame Surface lockBuffer fail"<<endl;
		return false;
	}

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
	auto_ptr<SkCanvas> canvas = auto_ptr<SkCanvas>(new SkCanvas(bitmap));

	SkPaint paint;
	const SkBitmap map = frames[idx];

	Size s = info->size();
	int x = xoff + max(0, (s.width - map.width())) / 2;
	int y = yoff + max(0, (s.height - map.height())) / 2;

	canvas->drawColor(SK_ColorBLACK);
	canvas->drawBitmap(map, x, y, &paint);
	surface->unlockAndPost();
	return true;
}

void SkiaPlayer::unint_frame () {
	for (vector<SkBitmap>::iterator it = frames.begin(); it != frames.end(); it++)
		it->reset();
}

inline SkColorType SkiaPlayer::convertPixelFormat (PixelFormat format) {
	switch (format) {
		case PIXEL_FORMAT_RGBX_8888: case PIXEL_FORMAT_RGBA_8888: return kN32_SkColorType;
		case PIXEL_FORMAT_RGBA_FP16: return kRGBA_F16_SkColorType;
		case PIXEL_FORMAT_RGB_565:   return kRGB_565_SkColorType;
		default:                     return kUnknown_SkColorType;
	}
}

// ----------------------------------------------------------------------
const GLchar* GLPlayer::VERTEX_STR =
	"attribute vec4 vPosition; \n"
	"attribute vec2 vCooridnate; \n"
	"uniform mat4 vMatrix; \n"
    "varying vec2 aCoordinate; \n"
	" void main () { \n"
	" 	gl_Position = vPosition; \n"
	"	aCoordinate = vCooridnate; \n"
	" } \n";

const GLchar* GLPlayer::FRAGMENT_STR =
	"precision mediump float; \n"
    "                        \n"
	" uniform sampler2D vTexture; \n"
	" varying vec2 aCoordinate;   \n"
	"                         \n"
	" void main () { \n"
	"	gl_FragColor = texture2D(vTexture, aCoordinate); \n"
	" } \n";

const GLfloat GLPlayer::vertex_position[] = {
	-1.0f,  1.0f, 0,
	-1.0f, -1.0f, 0,
	1.0f,  -1.0f, 0,
	1.0f,   1.0f, 0,
};

const GLfloat GLPlayer::texture_position[] = {
	0,  0,
	0,  1.0f,
	1.0f, 1.0f,
	1.0f, 0,
};

GLuint GLPlayer::loadShader (GLenum type, const GLchar* shader_str) {
	GLuint shader;

	if (!(shader = glCreateShader(type))) {
		FPLog.E()<<"glCreateShader Type="<<type<<" Fail"<<endl;
		return 0;
	}

	glShaderSource(shader, 1, &shader_str, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		FPLog.E()<<"glCompileShader "<<shader<<":"<<type<<" fail"<<endl;

		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		if (length > 0) {
			char * buf = static_cast<char*>(malloc(length));
			if (buf != nullptr) {
				memset(buf, 0, length);
				glGetShaderInfoLog(shader, length, NULL, buf);	
				FPLog.E()<<"glCompileShader Error="<<buf<<endl;
				free(buf);
			}
		}

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

bool GLPlayer::init_frame () {
	int max_frames = info->count();
	const EGLint display_attrs[] = {
		EGL_RED_SIZE,   8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE,  8,
		EGL_DEPTH_SIZE, 16,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE,
	};

	egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(egl_display, 0, 0);

	EGLint num_config;
	eglChooseConfig(egl_display, display_attrs, &egl_config, 1, &num_config);
	egl_surface = eglCreateWindowSurface(egl_display, egl_config, surface.get(), nullptr);

	EGLint context_attrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE,
	};
	egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attrs);
	if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) == EGL_FALSE) {
		FPLog.E()<<"eglMakeCurrent fail : "<<eglGetError()<<endl;
		return false;
	}

	vShader = loadShader(GL_VERTEX_SHADER, VERTEX_STR);
	if (glGetError() != GL_NO_ERROR) {
		FPLog.E()<<"load VertexShader Error="<<glGetError()<<endl;
		return false;
	}

	fShader = loadShader(GL_FRAGMENT_SHADER, FRAGMENT_STR);
	if (glGetError() != GL_NO_ERROR) {
		glDeleteShader(vShader);
		FPLog.E()<<"load FragmentShader Error="<<glGetError()<<endl;
		return false;
	}

	if (!(program = glCreateProgram())) {
		FPLog.E()<<"glCreateProgram Fail="<<endl;
		glDeleteShader(vShader);
		glDeleteShader(fShader);
		return false;
	}

	glAttachShader(program, vShader);
	glAttachShader(program, fShader);
	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		FPLog.E()<<"glLinkProgram "<<program<<" Fail"<<endl;

		GLint length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		if (length > 0) {
			char *buf = static_cast<char*>(malloc(length));
			if (buf != nullptr) {
				memset(buf, 0, length);
				glGetProgramInfoLog(program, length, nullptr, buf);
				FPLog.E()<<"glLinkProgram Error="<<buf<<endl;
				free(buf);
			}
		}

		glDeleteProgram(program);
		glDeleteShader(vShader);
		glDeleteShader(fShader);
		return false;
	}

	GLfloat maxtrix[] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
	GLint uMaxtrix = glGetUniformLocation(program, "vMatrix");
	glUniformMatrix4fv(uMaxtrix, sizeof(maxtrix), false, maxtrix);

	for (int i = 0; i < max_frames; i++) {
		shared_ptr<istream> is = info->frame(i);
		if (!is.get() || !is->good())
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
			continue;
		}

		if (!image->asLegacyBitmap(&bitmap, SkImage::kRO_LegacyBitmapMode)) {
			FPLog.E()<<"asLegacyBitmap : "<<i<<" fail"<<endl;
			continue;
		}

		const int w = bitmap.width();
		const int h = bitmap.height();
		const void *p = bitmap.getPixels();

		GLuint texture_id;
		glGenTextures(1, &texture_id);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		switch (bitmap.colorType()) {
			case kAlpha_8_SkColorType:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, p);
				break;
			case kARGB_4444_SkColorType:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, p);
				break;
			case kN32_SkColorType:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE ,p);
				break;
			case kRGB_565_SkColorType:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, p);
				break;
			default:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, p);
				break;
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		frame_textures.push_back(texture_id);
		frame_size.push_back(pair<int, int>(w, h));
	}

	return true;
}

bool GLPlayer::flush_frame(int idx __unused) const {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);

	Size s = info->size();
	pair<int, int> size = frame_size[idx];
	int x = xoff + max((s.width - size.first), 0)/2;
	int y = yoff + max((s.height - size.second), 0)/2;
	glViewport(x, y, size.first, size.second);

	GLint vertex_attr_pos = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vertex_attr_pos);
	glVertexAttribPointer(vertex_attr_pos, 3, GL_FLOAT, GL_FALSE, 0, vertex_position);

	GLint uTexture = glGetUniformLocation(program, "vTexture");
	glUniform1i(uTexture, GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame_textures[idx]);

	GLint frag_attr_pos = glGetAttribLocation(program, "vCooridnate");
	glEnableVertexAttribArray(frag_attr_pos);
	glVertexAttribPointer(frag_attr_pos, 2, GL_FLOAT, GL_FALSE, 0, texture_position);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	if (eglSwapBuffers(egl_display, egl_surface) == EGL_FALSE)
		return false;
	else
		return true;

	glDisableVertexAttribArray(vertex_attr_pos);
	glDisableVertexAttribArray(frag_attr_pos);
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
