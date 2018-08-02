#ifndef _FRAME_INFO_H_
#define _FRAME_INFO_H_

#include <istream>

using namespace std;

namespace frame_animation {

struct Resolution {
	int width;
	int height;
};

class FrameInfo {
	int idx;
	Resolution resolution;
	auto_ptr<istream> data;
};

}; //namespace frame_animation

#endif
