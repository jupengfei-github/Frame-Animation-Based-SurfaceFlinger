#include <iostream>
#include <string>

#include "FrameParser.h"

using namespace std;
using namespace frame_animation;

static void usage () {
	cout<<"frame_animation file_path"<<endl;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		usage();
		exit(-1);
	}

	FrameParser frame_parser;
	frame_parser.parse_frame(string(argv[1]));
	return 0;
}
