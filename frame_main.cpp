#include <iostream>
#include <string>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include "FrameParser.h"
#include "FramePlayer.h"
#include "FrameInfo.h"

using namespace std;
using namespace frame_animation;

int main(void) {
	sp<ProcessState> proc(ProcessState::self());
	proc->startThreadPool();

	FrameParser frame_parser;
	shared_ptr<FrameInfo> frame_info = frame_parser.parse_frame(string("/sdcard/animation"));
	auto_ptr<FramePlayer> frame_player = auto_ptr<FramePlayer>(new SkiaPlayer(frame_info));
	frame_player->start();

	IPCThreadState::self()->joinThreadPool();
	return 0;
}
