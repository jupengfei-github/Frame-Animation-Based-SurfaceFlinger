#include <regex>
#include <sstream>
#include <iostream>
#include <fstream>
#include <androidfw/ZipFileRO.h>
#include <androidfw/Asset.h>
#include <androidfw/AssetManager.h>
#include <androidfw/ResourceTypes.h>

#include "FrameParser.h"
#include "FrameStream.h"
#include "FrameError.h"

namespace frame_animation {

/* desc frame type */
const string FrameParser::FRAME_TYPE_ZIP_STR = "zip";
const string FrameParser::FRAME_TYPE_APK_STR = "apk";
const string FrameParser::FRAME_TYPE_DIR_STR = "dir";

shared_ptr<FrameInfo> FrameParser::parse (const string& path) {
	AnimResType type = frame_type(path);
	return FrameInfo::create_from_type(type);
}

bool ends_with (const string& str, const string& suffix) {
	return str.rfind(suffix) == (str.length() - suffix.length());
}

FrameParser::AnimResType FrameParser::frame_type (const string& value) const {
	if (value == FRAME_TYPE_APK_STR)
		return FRAME_RES_TYPE_APK;
	else if (value == FRAME_TYPE_ZIP_STR)
		return FRAME_RES_TYPE_ZIP;
	else if (value == FRAME_TYPE_DIR_STR)
		return FRAME_RES_TYPE_DIR;
	else 
		return FRAME_RES_TYPE_NONE;
}

}; //namespace frame_animation
