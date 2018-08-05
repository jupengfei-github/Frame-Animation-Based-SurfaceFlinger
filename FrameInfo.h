#ifndef _FRAME_INFO_H_
#define _FRAME_INFO_H_

#include <istream>
#include <vector>
#include <androidfw/ZipFileRO.h>
#include <androidfw/AssetManager.h>
#include "FrameStream.h"

using namespace std;
using namespace android;

namespace frame_animation {

struct Resolution {
	int width;
	int height;
};

const string ENTRY_DESC  = "desc.txt";
const string APK_PACKAGE = "animation";
const string APK_NAME    = "desc";
const string APK_DESC_TYPE = "raw";
const string APK_ANIM_TYPE = "drawable";

/* frame mode str */
const string FRAME_MODE_REVERSE_STR = "reverse";
const string FRAME_MODE_REPEATE_STR = "repeate";
const string FRAME_MODE_NORMAL_STR  = "normal";
enum FrameMode {
	FRAME_MODE_REVERSE,
	FRAME_MODE_REPEATE,
	FRAME_MODE_NORMAL,
};

struct DescriptionInfo {
	vector<string> frames;
	string frame_path;
	FrameMode frame_mode;
	int frame_rate;
	Resolution resolution;
};

class FrameInfo {
protected:
	int idx;
	DescriptionInfo info;
public:
	FrameInfo (DescriptionInfo ifo) {
		info = ifo;
		idx = 0;
	}
	virtual ~FrameInfo () {}

	int cur_max_count();
	int cur_idx();
	FrameMode cur_mode();
	int cur_rate();
	void reset_frame();
	Resolution cur_resolution();

	virtual shared_ptr<istream> next_frame() = 0;
};

// --------------------------------------------------------
class ZipFrameInfo : public FrameInfo {
	shared_ptr<ZipFileRO> zip_file;
public:
	ZipFrameInfo (DescriptionInfo info, shared_ptr<ZipFileRO> zip):FrameInfo(info) {
		zip_file = zip;
	}
	virtual ~ZipFrameInfo () {}
	virtual shared_ptr<istream> next_frame();
};

// --------------------------------------------------------
struct ApkFrameInfo : public FrameInfo {
	ApkFrameInfo (DescriptionInfo info, shared_ptr<AssetManager> assetManager):FrameInfo(info),assetManager(assetManager) {}
	virtual shared_ptr<istream> next_frame();
	virtual ~ApkFrameInfo() {}
private:
	shared_ptr<AssetManager> assetManager;
};

struct DIRFrameInfo : public FrameInfo {
	DIRFrameInfo (DescriptionInfo info):FrameInfo(info) {}
	virtual shared_ptr<istream> next_frame();
	virtual ~DIRFrameInfo() {}
};

}; //namespace frame_animation

#endif
