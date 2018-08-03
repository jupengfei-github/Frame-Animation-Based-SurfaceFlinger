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
		FPLog.E()<<"cur_frame : "<<path<<" failed";
		return shared_ptr<istream>(nullptr);
	}

	return shared_ptr<istream>(new istream(new ZipStreamBuf(zip_file, entry)));
}

// -----------------------------------------------
shared_ptr<istream> ApkFrameInfo::cur_frame () {
	ResTable resTable = assetManager.getResources();
	string name = frames[idx];

	String16 s16_name = String16(name.c_str(), name.length());
	String16 s16_type = String16("drawable");
	String16 s16_pkg  = String16("animation");
	uint32_t id = resTable.identifierForName(s16_name, s16_name.size(),
		s16_type, s16_type.size(), s16_pkg, s16_pkg.size());
	if (id == 0) {
		FPLog.E()<<"identifierForName "<<name<<" fail";
		return shared_ptr<istream>(nullptr);
	}

	Res_value value;
	resTable.getResource(id, &value);

	auto_ptr<constStringPool> strPool(resTable.getTableStringBlock(0));
	size_t len;
	char* str = strPool->string8At(value.data, &len);

	string real_path(str, len);
	shared_ptr<Asset> asset(assetManager->open(real_path, Asset::AssetMode::STREAM));

	return shared_ptr<istream>(new ResStreamBuf(asseet));
}

// ----------------------------------------------
shared_ptr<istream> DIRFrameInfo::cur_frame () {
	string path = frame_path + "/" + frames[idx];
	shared_ptr<istream> ifm(new ifstream(path));
	if (!ifm->good()) {
		FPLog.E()<<"cur_frame : "<<path<<" failed";
		return shared_ptr<istream>(nullptr);
	}

	return ifm;
}

}; //namespace frame_animation
