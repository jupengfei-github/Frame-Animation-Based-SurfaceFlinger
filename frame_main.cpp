#include <iostream>
#include <string>

#include "FrameParser.h"

using namespace std;
using namespace frame_animation;

int main(void) {
	FrameParser frame_parser;
	frame_parser.parse_frame(string("/sdcard/animation.zip"));
	return 0;
}
