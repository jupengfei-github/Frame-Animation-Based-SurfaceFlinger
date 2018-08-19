#include <fstream>
#include <androidfw/ResourceTypes.h>
#include <androidfw/AssetManager.h>
#include "FrameInfo.h"

using namespace std;
using namespace android;

namespace frame_animation {

int FrameInfo::cur_max_count() {
	return info.frames.size();
}

int FrameInfo::cur_idx() {
	return idx;
}

FrameMode FrameInfo::cur_mode() {
	return info.frame_mode;
}

int FrameInfo::cur_rate() {
	return info.frame_rate;
}

Resolution FrameInfo::cur_resolution() {
	return info.resolution;
}

void FrameInfo::reset_frame () {
	idx = 0;
}

// ------------------------------------------------
shared_ptr<istream> ZipFrameInfo::next_frame () {
	string path = info.frame_path + "/" + info.frames[idx++];
	return shared_ptr<istream>(new istream(new ZipStreamBuf(zip_file, path)));
}

// -----------------------------------------------
shared_ptr<istream> ApkFrameInfo::next_frame () {
	const ResTable& resTable = assetManager->getResources();
	string name = info.frames[idx];

	String16 s16_name = String16(name.c_str(), name.length());
	String16 s16_type = String16(APK_ANIM_TYPE.c_str());
	String16 s16_pkg  = String16(APK_PACKAGE.c_str());
	uint32_t id = resTable.identifierForName(s16_name, s16_name.size(),
		s16_type, s16_type.size(), s16_pkg, s16_pkg.size());
	if (id == 0) {
		FPLog.E()<<"identifierForName "<<name<<" fail";
		return shared_ptr<istream>(nullptr);
	}

	Res_value value;
	ssize_t index = resTable.getResource(id, &value);

	auto_ptr<const ResStringPool> strPool(resTable.getTableStringBlock(index));
	size_t len;
	const char* str = strPool->string8At(value.data, &len);

	string real_path(str, len);
	shared_ptr<Asset> asset(assetManager->openNonAsset(real_path.c_str(), Asset::ACCESS_STREAMING));

	return shared_ptr<istream>(new istream(new ResStreamBuf(asset)));
}

// ----------------------------------------------
shared_ptr<istream> DIRFrameInfo::next_frame () {
	string path = base_path + "/" + info.frame_path + "/" + info.frames[idx++];

	shared_ptr<istream> ifm(new ifstream(path));
	if (!ifm->good()) {
		FPLog.E()<<"DIRFrameInfo next_frame "<<path<<" failed";
		return shared_ptr<istream>(nullptr);
	}

	return ifm;
}

}; //namespace frame_animation
