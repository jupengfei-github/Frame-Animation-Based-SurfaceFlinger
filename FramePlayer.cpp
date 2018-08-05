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

void FramePlayer::animation_thread (FramePlayer const *player) {
	shared_ptr<FrameInfo> info = player->frame_info;

	int frame_rate = info->cur_rate();
	int frame_time = 1000 / frame_rate;

	FrameMode mode = info->cur_mode();
	int frame_cnt  = info->cur_max_count();

	player->init_frame(info);

	int idx = 0;
	do {
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
			player->flush_frame(info->next_frame(), idx++);

		const long sleepTime = ns2ms(systemTime()) - now;
		if (sleepTime > 0)
			sleep(sleepTime);
	} while (!player->request_exit);

	player->unint_frame(info);
}

// -----------------------------------------------------------------------
void SkiaPlayer::init_frame (const shared_ptr<FrameInfo>& info __unused) const {

}

void SkiaPlayer::flush_frame(shared_ptr<istream> in __unused, int idx __unused) const {

}

void SkiaPlayer::unint_frame (const shared_ptr<FrameInfo>& info __unused) const {

}

// ----------------------------------------------------------------------
void GLPlayer::init_frame (const shared_ptr<FrameInfo>& info __unused) const {

}

void GLPlayer::flush_frame(shared_ptr<istream> in __unused, int idx __unused) const {

}

void GLPlayer::unint_frame (const shared_ptr<FrameInfo>& info __unused) const {

}

}; //namespace frame_animation
