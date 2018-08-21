#ifndef _FRAME_PARSER_H_
#define _FRAME_PARSER_H_

#include <string>
#include <vector>
#include <list>
#include <utils/FileMap.h>

#include <GLES/gl.h>
#include <EGL/egl.h>

#include "FrameInfo.h"

using namespace std;
using namespace android;

namespace frame_animation {

class FrameParser {
	static const string FRAME_TYPE_ZIP_STR;
	static const string FRAME_TYPE_APK_STR;
	static const string FRAME_TYPE_DIR_STR;

	static AnimResType frame_type (const string&);
public:
	static shared_ptr<FrameInfo> parse (const string&);
};

}; //namespace frame_animation

#endif
