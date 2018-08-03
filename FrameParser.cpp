#include <androidfw/ZipFileRO.h>
#include <regex>
#include <sstream>
#include <iostream>
#include <exception>

#include "FrameParser.h"
#include "FrameStream.h"

namespace frame_animation {

const string FrameParser::ENTRY_DESC = "desc.txt";

/* desc file key/value */
const string FrameParser::DESC_KEY_MODE       = "mode";
const string FrameParser::DESC_KEY_RESOLUTION = "resolution";
const string FrameParser::DESC_KEY_RATE       = "rate";
const string FrameParser::DESC_KEY_FRAMES     = "frames";
const string FrameParser::DESC_KEY_FRAME_PATH = "frame_path";
const string FrameParser::DESC_KEY_FRAME_TYPE = "frame_type";

/* desc frame type */
const string FrameParser::FRAME_TYPE_ZIP_STR = "zip";
const string FrameParser::FRAME_TYPE_APK_STR = "apk";
const string FrameParser::FRAME_TYPE_DIR_STR = "dir";

template<typename T> T lexical_cast(string s) {
	istringstream iss(s);
	T rt;
	iss>>rt;
	return rt;
}

bool FrameParser::ends_with (string str, string suffix) {
	return str.rfind(suffix) == (str.length() - suffix.length());
}

shared_ptr<FrameInfo> FrameParser::parse_frame (const string path) {
	if (ends_with(path, ".zip"))
		return parse_zip_frame(path);
	else if (ends_with(path, ".apk"))
		return parse_apk_frame(path);	
	else
		return parse_dir_frame(path);
}

shared_ptr<ZipFrameInfo> FrameParser::parse_zip_frame (const string path) {
	shared_ptr<ZipFileRO> zip_file(ZipFileRO::open(path.c_str()));
	if (!zip_file.get()) {
		FPLog.E()<<"open "<<path<<" fail"<<endl;
		throw exception();
	}

	ZipEntryRO desc = zip_file->findEntryByName(ENTRY_DESC.c_str());
	if (desc == nullptr) {
		FPLog.E()<<"find entry "<<ENTRY_DESC<<" fail"<<endl;
		throw exception();
	}

	FileMap *file_map = zip_file->createEntryFileMap(desc);
	string desc_str(static_cast<char*>(map->getDataPtr()), map->getDataLength());
	parse_desc_file(desc_str);
	delete file_map;
	zip_file->releaseEntry(desc);

	return shared_ptr<ZipFrameInfo>(new ZipFrameInfo(dsinf, zip_file));
}

shared_ptr<ApkFrameInfo> FrameParser::parse_apk_frame (const string path) {
	uint32_t cookie;
	AssetManager assetManager;

	assetManager.addAssetPath(String8(path), &cookie);
	ResTable resTable = assetManager.getResources();

	String16 name(path.c_str());
	String16 type("raw");
	String16 package("android");
	uint32_t id = resTable.identifierForName(name, name.size(),
		type, type.size(), package, package.size());
}

shared_ptr<DIRFrameInfo> FrameParser::parse_dir_frame (const string path) {

}

void FrameParser::parse_desc_file (const string &desc_str) {
	regex item_pattern("(\\w+)\\s*:\\s*\\[(.+)\\]");
	smatch item_match;

	while (regex_search(desc_str, item_match, item_pattern)) {
		string key_item(item_match[1].first, item_match[1].second);
		string value_item(item_match[2].first, item_match[2].second);
		parse_desc_item(key_item, value_item);

		desc_str = item_match.suffix().str();
	}
}

void FrameParser::parse_desc_item (string key, string value) {
	if (key == DESC_KEY_RESOLUTION) {
		regex resolution_regex("(\\d+)x(\\d+)");
		smatch result;
		if (regex_search(value, result, resolution_regex)) {
			frame_desc.resolution.width  = lexical_cast<int>(string(result[1].first, result[1].second));
			frame_desc.resolution.height = lexical_cast<int>(string(result[2].first, result[2].second));
		}
		else
			;
			//FPLog.E()<<"invalide resoution : "<<value<<endl;
	}
	else if (key == DESC_KEY_RATE)
		frame_desc.frame_rate = lexical_cast<int>(value);
	else if (key == DESC_KEY_FRAMES) {
		regex frame_regex("\\w+");
		smatch frame_match;
		while (regex_search(value, frame_match, frame_regex)) {
			frame_desc.frames.push_back(string(frame_match[0].first, frame_match[0].second));
		}
	}
	else if (key == DESC_KEY_FRAME_PATH)
		frame_desc.frame_path = value;
	else if (key == DESC_KEY_FRAME_TYPE)
		type = frame_type(value);
	else if (key == DESC_KEY_MODE)
		frame_desc.frame_mode = frame_mode(value);
	else 
		;//FPLog.E()<<"illegal desc item "<<value<<endl;
}

FrameParser::FrameResType FrameParser::frame_type (string value) {
	if (value == FRAME_TYPE_APK_STR)
		return FRAME_RES_TYPE_APK;
	else if (value == FRAME_TYPE_ZIP_STR)
		return FRAME_RES_TYPE_ZIP;
	else if (value == FRAME_TYPE_DIR_STR)
		return FRAME_RES_TYPE_DIR;
	else 
		return FRAME_RES_TYPE_NONE;
}

FrameMode FrameParser::frame_mode (string value) {
	if (value == FRAME_MODE_NORMAL_STR)
		return FRAME_MODE_NORMAL;
	else if (value == FRAME_MODE_REPEATE_STR)
		return FRAME_MODE_REPEATE;
	else if (value == FRAME_MODE_REVERSE_STR)
		return FRAME_MODE_REVERSE;
	else
		return FRAME_MODE_NORMAL;
}

}; //namespace frame_animation
