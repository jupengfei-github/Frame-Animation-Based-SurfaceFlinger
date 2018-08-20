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

/* Animation Resource Type */
enum AnimResType {
	FRAME_RES_TYPE_DIR,
	FRAME_RES_TYPE_ZIP,
	FRAME_RES_TYPE_APK,
	FRAME_RES_TYPE_NONE,
};

/* Display Area of Animation */
struct Size {
	int width;
	int height;
};

/* Animation Excute Mode */
enum AnimMode {
	FRAME_MODE_REVERSE,
	FRAME_MODE_REPEATE,
	FRAME_MODE_NORMAL,
};

/* Animation Information */
struct AnimInfo {
	string parent_path;  /* Root Directory */
	string frame_path;   /* Images Directory Relative Root */
	vector<string> frames; /* Frame Image Name Relative Images */
	AnimMode mode;  /* reapte/reverse/normal */
	int rate;       /* frames per seconds */
	Size size;      /* Anim Display Area  */
};

class FrameInfo {
	/* Frme Desc key/value */
	static const string DESC_KEY_RESOLUTION;
	static const string DESC_KEY_MODE;
	static const string DESC_KEY_RATE;
	static const string DESC_KEY_FRAMES;
	static const string DESC_KEY_FRAME_PATH;
	static const string DESC_KEY_FRAME_TYPE;

	/* Frame Mode String */
	static const string FRAME_MODE_REVERSE_STR;
	static const string FRAME_MODE_REPEATE_STR;
	static const string FRAME_MODE_NORMAL_STR;

	void parse_desc_item(const string&, const string&);
	AnimMode frame_mode (const string& value) const;
	void dump () const;
protected:
	int cur_frame; /* current frame */
	int max_frame; /* max frames */
	AnimInfo info;
 
public:
	virtual ~FrameInfo () {}

	int count();
	int idx();
	AnimMode mode();
	int  rate();
	void reset();
	Size size();

	void parse_anim_info (const string&);
	virtual shared_ptr<istream> next_frame() = 0;
	virtual string parse_anim_file(const string&) = 0;
};

// --------------------------------------------------------
class ZipFrameInfo : public FrameInfo {
	shared_ptr<ZipFileRO> zip_file;
public:
	ZipFrameInfo (AnimInfo info, shared_ptr<ZipFileRO> zip):FrameInfo(info) {
		zip_file = zip;
	}
	virtual ~ZipFrameInfo () {}

	virtual shared_ptr<istream> next_frame() override;
	virtual string parse_anim_file (const string&) override;
};

// --------------------------------------------------------
class ApkFrameInfo : public FrameInfo {
	static const string APK_PACKAGE   = "animation";
	static const string APK_NAME      = "desc";
	static const string APK_DESC_TYPE = "raw";
	static const string APK_ANIM_TYPE = "drawable";

	shared_ptr<AssetManager> assetManager;
public:
	ApkFrameInfo (AnimInfo info, shared_ptr<AssetManager> assetManager):FrameInfo(info),assetManager(assetManager) {}

	virtual shared_ptr<istream> next_frame() override;
	virtual string parse_anim_file (const string&) override;
	virtual ~ApkFrameInfo() {}
};

class DIRFrameInfo : public FrameInfo {
	string base_path;
public:
	DIRFrameInfo (AnimInfo info):FrameInfo(info) {}

	virtual shared_ptr<istream> next_frame() override;
	virtual string parse_anim_file (const string&) override;
	virtual ~DIRFrameInfo() {}
};

}; //namespace frame_animation

#endif
