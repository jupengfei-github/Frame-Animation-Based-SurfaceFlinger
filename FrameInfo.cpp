#include "FrameInfo.h"

namespace frame_animation {

/* desc frame mode */
const string FRAME_MODE_REPEATE_STR = "repeate";
const string FRAME_MODE_REVERSE_STR = "reverse";
const string FRAME_MODE_NORMAL_STR  = "normal";

int FrameInfo::cur_max_count() {
	return frames.size();
}

int FrameInfo::cur_idx() {
	return idx;
}

FrameMode FrameInfo::cur_mode() {
	return frame_mode;
}

int FrameInfo::cur_rate() {
	return frame_rate;
}

void FrameInfo::next_frame() {
	idx++;
}

Resolution FrameInfo::cur_resolution() {
	return resolution;
}

// ------------------------------------------------
shared_ptr<istream> ZipFrameInfo::cur_frame () {
	string path = frame_path + "/" + frames[idx];
	ZipEntryRO entry = zip_file->findEntryByName(path.c_str());

	if (entry == nullptr) {
		FPLog.E()<<"cur_frame : "<<path<<" failed"<<endl;
		return shared_ptr<istream>(nullptr);
	}

	return shared_ptr<istream>(new istream(new ZipStreamBuf(zip_file, entry)));
}

// -----------------------------------------------
shared_ptr<istream> ApkFrameInfo::cur_frame () {
	return shared_ptr<istream>(nullptr);
}

// ----------------------------------------------
shared_ptr<istream> DIRFrameInfo::cur_frame () {
	return shared_ptr<istream>(nullptr);
}

}; //namespace frame_animation
