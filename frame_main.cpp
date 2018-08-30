#include <iostream>
#include <string>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include "FrameParser.h"
#include "FramePlayer.h"
#include "FrameInfo.h"

#define ANIM_PATH_ZIP  "/sdcard/animation.zip"
#define ANIM_PATH_DIR  "/sdcard/animation"
#define ANIM_PATH_RES  "/sdcard/animation.apk"

#define ANIM_PATH      ANIM_PATH_RES

using namespace std;
using namespace frame_animation;

int main(void) {
    sp<ProcessState> proc(ProcessState::self());
    proc->startThreadPool();

    FrameParser frame_parser;
    shared_ptr<FrameInfo> frame_info = frame_parser.parse(string(ANIM_PATH));
    auto_ptr<FramePlayer> frame_player = auto_ptr<FramePlayer>(new GLPlayer(frame_info));
    frame_player->start();

    IPCThreadState::self()->joinThreadPool();
    return 0;
}
