#ifndef _FRAME_PARSER_H_
#define _FRAME_PARSER_H_

#include <string>
#include <vector>
#include <utils/FileMap.h>

#include "FrameInfo.h"

using namespace std;
using namespace android;

namespace frame_animation {

class FrameParser {
	static const string ENTRY_DESC;

	/* desc key/value */
	static const string DESC_KEY_RESOLUTION;
	static const string DESC_KEY_MODE;
	static const string DESC_KEY_RATE;
	static const string DESC_KEY_FRAMES;
	static const string DESC_KEY_FRAME_PATH;
	static const string DESC_KEY_FRAME_TYPE;

	/* frame type string */
	static const string FRAME_TYPE_DIR_STR;
	static const string FRAME_TYPE_ZIP_STR;
	static const string FRAME_TYPE_APK_STR;
	enum FrameResType {
		FRAME_RES_TYPE_DIR,
		FRAME_RES_TYPE_ZIP,
		FRAME_RES_TYPE_APK,
		FRAME_RES_TYPE_NONE,
	};

	void parse_desc_file(FileMap*);
	void parse_desc_item(string, string);
	shared_ptr<FrameInfo> createFrameInfo(shared_ptr<ZipFileRO> zip_file);

	FrameResType frame_type (string);
	FrameMode frame_mode (string);

public:
	/* frame parse */
	shared_ptr<FrameInfo> parse_frame (const string);

private:
	string file_path;
	DescriptionInfo frame_desc;
	FrameResType type;
};
	
}; //namespace frame_animation

#endif
