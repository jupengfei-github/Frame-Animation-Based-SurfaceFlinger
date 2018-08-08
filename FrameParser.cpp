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

template<typename T> T lexical_cast(const string& s) {
	T rt;
	istringstream iss(s);
	iss>>rt;
	return rt;
}

bool ends_with (const string& str, const string& suffix) {
	return str.rfind(suffix) == (str.length() - suffix.length());
}

shared_ptr<FrameInfo> FrameParser::parse_frame (const string& path) {
	FPLog.I()<<"parse_frame : "<<path<<endl;

	shared_ptr<FrameInfo> result;
	if (ends_with(path, ".zip"))
		result = parse_zip_frame(path);
	else if (ends_with(path, ".apk"))
		result = parse_apk_frame(path);	
	else
		result = parse_dir_frame(path);

	dump();
	return result;
}

void FrameParser::dump () const {
	FPLog.I()<<"frame_path : "<<frame_desc.frame_path<<endl;
	FPLog.I()<<"frame_type : "<<type<<endl;
	FPLog.I()<<"frame_mode : "<<frame_desc.frame_mode<<endl;
	FPLog.I()<<"frame_rate : "<<frame_desc.frame_rate<<endl;
	FPLog.I()<<"frame_resolution : ["<<frame_desc.resolution.width<<"x"<<frame_desc.resolution.height<<"]"<<endl;
}

void FrameParser::parse_desc_file (const string &desc_str) {
	regex item_pattern("\\s*(\\w+)\\s*:\\s*\\[([^\\[\\]]+)\\]");
	smatch item_match;

	string ite_str = desc_str;
	while (regex_search(ite_str, item_match, item_pattern)) {
		string key_item(item_match[1].first, item_match[1].second);
		string value_item(item_match[2].first, item_match[2].second);
		parse_desc_item(key_item, value_item);

		ite_str = item_match.suffix().str();
	}
}

void FrameParser::parse_desc_item (const string& key, const string& value) {
	if (key == DESC_KEY_RESOLUTION) {
		regex resolution_regex("(\\d+)x(\\d+)");
		smatch result;
		if (regex_search(value, result, resolution_regex)) {
			frame_desc.resolution.width  = lexical_cast<int>(string(result[1].first, result[1].second));
			frame_desc.resolution.height = lexical_cast<int>(string(result[2].first, result[2].second));
		}
		else
			FPLog.E()<<"invalide resoution : "<<value<<endl;
	}
	else if (key == DESC_KEY_RATE)
		frame_desc.frame_rate = lexical_cast<int>(value);
	else if (key == DESC_KEY_FRAMES) {
		regex frame_regex("[\\w\\.]+");
		smatch frame_match;
		string ite_value = value;
		while (regex_search(ite_value, frame_match, frame_regex)) {
			frame_desc.frames.push_back(string(frame_match[0].first, frame_match[0].second));
			ite_value = frame_match.suffix().str();
		}
	}
	else if (key == DESC_KEY_FRAME_PATH)
		frame_desc.frame_path = value;
	else if (key == DESC_KEY_FRAME_TYPE)
		type = frame_type(value);
	else if (key == DESC_KEY_MODE)
		frame_desc.frame_mode = frame_mode(value);
	else 
		FPLog.E()<<"illegal desc item "<<value<<endl;
}

FrameParser::FrameResType FrameParser::frame_type (const string& value) const {
	if (value == FRAME_TYPE_APK_STR)
		return FRAME_RES_TYPE_APK;
	else if (value == FRAME_TYPE_ZIP_STR)
		return FRAME_RES_TYPE_ZIP;
	else if (value == FRAME_TYPE_DIR_STR)
		return FRAME_RES_TYPE_DIR;
	else 
		return FRAME_RES_TYPE_NONE;
}

FrameMode FrameParser::frame_mode (const string& value) const {
	if (value == FRAME_MODE_NORMAL_STR)
		return FRAME_MODE_NORMAL;
	else if (value == FRAME_MODE_REPEATE_STR)
		return FRAME_MODE_REPEATE;
	else if (value == FRAME_MODE_REVERSE_STR)
		return FRAME_MODE_REVERSE;
	else
		return FRAME_MODE_NORMAL;
}

/* zip animation */
shared_ptr<FrameInfo> FrameParser::parse_zip_frame (const string& path) {
	shared_ptr<ZipFileRO> zip_file(ZipFileRO::open(path.c_str()));
	if (!zip_file.get())
		throw parse_exception("open " + path + " fail");

	ZipEntryRO desc = zip_file->findEntryByName(ENTRY_DESC.c_str());
	if (desc == nullptr)
		throw parse_exception("find entry " + ENTRY_DESC + " fail");

	auto_ptr<FileMap> file_map(zip_file->createEntryFileMap(desc));
	string desc_str(static_cast<char*>(file_map->getDataPtr()), file_map->getDataLength());
	delete file_map.release();
	zip_file->releaseEntry(desc);

	parse_desc_file(desc_str);
	return shared_ptr<ZipFrameInfo>(new ZipFrameInfo(frame_desc, zip_file));
}

/* apk animation */
shared_ptr<FrameInfo> FrameParser::parse_apk_frame (const string& path) {
	shared_ptr<AssetManager> assetManager(new AssetManager());
	shared_ptr<FrameInfo> null_rlt(nullptr);

	String8 s8_path(path.c_str(), path.length());
	if (!assetManager->addAssetPath(s8_path, nullptr)) {
		FPLog.E()<<"parse_apk_frame addAssetPath fail"<<endl;
		return null_rlt;
	}

	const ResTable& resTable = assetManager->getResources();
	String16 s16_name(APK_NAME.c_str());
	String16 s16_type(APK_DESC_TYPE.c_str());
	String16 s16_pkg(APK_PACKAGE.c_str());

	uint32_t id = resTable.identifierForName(s16_name, s16_name.size(),
		s16_type, s16_type.size(), s16_pkg, s16_pkg.size());
	if (id == 0) {
		FPLog.E()<<"obtain identifier "<<path<<" fail";
		return null_rlt;
	}

	Res_value value;
	ssize_t index = resTable.getResource(id, &value);
	const ResStringPool *stringPool = resTable.getTableStringBlock(index);

	size_t len;
	const char *str = stringPool->string8At(value.data, &len);
	string real_path(str, len);

	auto_ptr<Asset> asset(assetManager->openNonAsset(real_path.c_str(), Asset::ACCESS_STREAMING));
	string desc_str(static_cast<const char*>(asset->getBuffer(true)), asset->getLength());
	asset->close();

	parse_desc_file(desc_str);
	return shared_ptr<ApkFrameInfo>(new ApkFrameInfo(frame_desc, assetManager));
}

/* dir animation */
shared_ptr<FrameInfo> FrameParser::parse_dir_frame (const string& path) {
	ifstream ifm(path + "/" + ENTRY_DESC);
	if (!ifm.good()) {
		FPLog.E()<<"open "<<path<<" fail"<<endl;
		ifm.close();
		return shared_ptr<FrameInfo>(nullptr);
	}

	string desc_str;
	string tmp;
	while (ifm>>tmp) {
		desc_str += tmp;
	}
	ifm.close();
	cout<<desc_str<<endl;

	parse_desc_file(desc_str);
	return shared_ptr<DIRFrameInfo>(new DIRFrameInfo(frame_desc));
}

}; //namespace frame_animation
