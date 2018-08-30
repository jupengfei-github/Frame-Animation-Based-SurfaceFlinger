#ifndef _FRAME_INFO_H_
#define _FRAME_INFO_H_

#include <istream>
#include <vector>
#include <androidfw/ZipFileRO.h>
#include <androidfw/AssetManager.h>

#include "FrameStream.h"
#include "FrameError.h"

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

	static const string PROP_SDK_VERSION;

    static ResTable_config default_resource_config();
	void parse_desc_item(const string&, const string&);
	AnimMode frame_mode (const string&) const;
	void dump () const;
protected:
	/* Frame Info FileName */
	static const string ENTRY_DESC;

	int max_frame; /* max frames */
	AnimInfo info;
 
	virtual string parse_anim_file() = 0;
public:
	virtual ~FrameInfo () {}

	int count();
	AnimMode mode();
	int  rate();
	Size size();

	void parse_anim_info ();
	virtual shared_ptr<istream> frame(int) = 0;

	static shared_ptr<FrameInfo> create_from_type (const string&, AnimResType type);
};

// --------------------------------------------------------
class ZipFrameInfo : public FrameInfo {
	shared_ptr<ZipFileRO> zip_file;
public:
	ZipFrameInfo (shared_ptr<ZipFileRO> zip) {
		zip_file = zip;
	}
	virtual ~ZipFrameInfo () {}

	virtual shared_ptr<istream> frame(int) override;
protected:
	virtual string parse_anim_file () override;
};

// --------------------------------------------------------
class ApkFrameInfo : public FrameInfo {
	static const string APK_PACKAGE;
	static const string APK_NAME;
	static const string APK_DESC_TYPE;
	static const string APK_ANIM_TYPE;

	shared_ptr<AssetManager> assetManager;
public:
	ApkFrameInfo (shared_ptr<AssetManager> assetManager):assetManager(assetManager) {}

	virtual shared_ptr<istream> frame(int) override;
	virtual ~ApkFrameInfo() {}
protected:
	virtual string parse_anim_file () override;
};

class DIRFrameInfo : public FrameInfo {
	string parent_path; /* Root Path */
public:
	DIRFrameInfo (const string& path):parent_path(path) {}
	virtual shared_ptr<istream> frame(int) override;
	virtual ~DIRFrameInfo() {}
protected:
	virtual string parse_anim_file () override;
};

}; //namespace frame_animation

#endif
