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
	static shared_ptr<FrameInfo> parse (const string&);
	static AnimResType frame_type (const string&);
	static bool ends_with (const string&, const string&);
};

}; //namespace frame_animation

#endif
