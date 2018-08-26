#include <regex>
#include <sstream>
#include <fstream>
#include <androidfw/ZipFileRO.h>
#include <androidfw/Asset.h>
#include <androidfw/AssetManager.h>
#include <androidfw/ResourceTypes.h>

#include "FrameInfo.h"

using namespace std;
using namespace android;

namespace frame_animation {

/* String -> T */
template<typename T> T lexical_cast(const string& s) {
	T rt;
	istringstream iss(s);
	iss>>rt;
	return rt;
}

/* desc file key/value */
const string FrameInfo::DESC_KEY_MODE       = "mode";
const string FrameInfo::DESC_KEY_RESOLUTION = "resolution";
const string FrameInfo::DESC_KEY_RATE       = "rate";
const string FrameInfo::DESC_KEY_FRAMES     = "frames";
const string FrameInfo::DESC_KEY_FRAME_PATH = "frame_path";

const string FrameInfo::FRAME_MODE_REVERSE_STR = "reverse";
const string FrameInfo::FRAME_MODE_REPEATE_STR = "repeat";
const string FrameInfo::FRAME_MODE_NORMAL_STR  = "normal";


/* desc file name */
const string FrameInfo::ENTRY_DESC = "desc.txt";

int FrameInfo::count() {
	return info.frames.size();
}

AnimMode FrameInfo::mode() {
	return info.mode;
}

int FrameInfo::rate() {
	return info.rate;
}

Size FrameInfo::size() {
	return info.size;
}

AnimMode FrameInfo::frame_mode (const string& value) const {
	if (value == FRAME_MODE_NORMAL_STR)
		return FRAME_MODE_NORMAL;
	else if (value == FRAME_MODE_REPEATE_STR)
		return FRAME_MODE_REPEATE;
	else if (value == FRAME_MODE_REVERSE_STR)
		return FRAME_MODE_REVERSE;
	else
		return FRAME_MODE_NORMAL;
}

void FrameInfo::dump () const {
	FPLog.I()<<"FrameInfo : "<<endl;
	FPLog.I()<<"Path  : "<<info.frame_path<<endl;
	FPLog.I()<<"Mode  : "<<info.mode<<endl;
	FPLog.I()<<"Rate  : "<<info.rate<<endl;
	FPLog.I()<<"Size  : ["<<info.size.width<<"x"<<info.size.height<<"]"<<endl;
}

void FrameInfo::parse_desc_item(const string& key, const string& value) {
	if (key == DESC_KEY_RESOLUTION) {
		regex resolution_regex("(\\d+)x(\\d+)");
		smatch result;
		if (regex_search(value, result, resolution_regex)) {
			info.size.width  = lexical_cast<int>(string(result[1].first, result[1].second));
			info.size.height = lexical_cast<int>(string(result[2].first, result[2].second));
		}
		else
			FPLog.E()<<"invalide resoution : "<<value<<endl;
	}
	else if (key == DESC_KEY_RATE)
		info.rate = lexical_cast<int>(value);
	else if (key == DESC_KEY_FRAMES) {
		regex frame_regex("[\\w\\.]+");
		smatch frame_match;
		string ite_value = value;
		while (regex_search(ite_value, frame_match, frame_regex)) {
			info.frames.push_back(string(frame_match[0].first, frame_match[0].second));
			ite_value = frame_match.suffix().str();
		}
	}
	else if (key == DESC_KEY_FRAME_PATH)
		info.frame_path = value;
	else if (key == DESC_KEY_MODE)
		info.mode = frame_mode(value);
	else 
		FPLog.E()<<"illegal desc item : "<<value<<endl;
}

void FrameInfo::parse_anim_info () {
	string anim_str = parse_anim_file();

	regex item_pattern("\\s*(\\w+)\\s*:\\s*\\[([^\\[\\]]+)\\]");
	smatch item_match;

	string ite_str = anim_str;
	while (regex_search(ite_str, item_match, item_pattern)) {
		string key_item(item_match[1].first, item_match[1].second);
		string value_item(item_match[2].first, item_match[2].second);
		parse_desc_item(key_item, value_item);

		ite_str = item_match.suffix().str();
	}

	max_frame = info.frames.size();
	dump();
}

shared_ptr<FrameInfo> FrameInfo::create_from_type (const string& path, AnimResType type) {
	shared_ptr<FrameInfo> frame_info;

	if (FRAME_RES_TYPE_APK == type) {
		shared_ptr<AssetManager> assetManager(new AssetManager());
		String8 s8_path(path.c_str(), path.length());

		if (!assetManager->addAssetPath(s8_path, nullptr))
			throw new parse_exception("parse_apk_frame addAssetPath fail");

		frame_info = shared_ptr<FrameInfo>(new ApkFrameInfo(assetManager));
	}
	else if (FRAME_RES_TYPE_ZIP == type) {
		shared_ptr<ZipFileRO> zip_file(ZipFileRO::open(path.c_str()));
		if (!zip_file.get())
			throw parse_exception("open " + path + " fail");

		frame_info =  shared_ptr<FrameInfo>(new ZipFrameInfo(zip_file));
	}
	else if (FRAME_RES_TYPE_DIR == type) {
		frame_info =  shared_ptr<FrameInfo>(new DIRFrameInfo(path));
	}
	else
		throw new parse_exception("unknown anim_res_type " + path);

	frame_info->parse_anim_info();
	return frame_info;
}

// ------------------------------------------------
shared_ptr<istream> ZipFrameInfo::frame (int idx) {
	if (idx >= max_frame) {
		FPLog.E()<<"frame ignore overflow frame index cur_frame="<<idx<<" max_frame="<<max_frame<<endl;
		idx = max_frame - 1;
	}

	string frame_name = info.frames[idx];
	return shared_ptr<istream>(new istream(new ZipStreamBuf(zip_file, info.frame_path + "/" + frame_name)));
}

string ZipFrameInfo::parse_anim_file () {
	ZipEntryRO desc = zip_file->findEntryByName(ENTRY_DESC.c_str());
	if (desc == nullptr)
		throw parse_exception("find entry " + ENTRY_DESC + " fail");

	auto_ptr<FileMap> file_map(zip_file->createEntryFileMap(desc));
	string desc_str(static_cast<char*>(file_map->getDataPtr()), file_map->getDataLength());
	delete file_map.release();
	zip_file->releaseEntry(desc);

	return desc_str;
}

// -----------------------------------------------
const string ApkFrameInfo::APK_PACKAGE   = "animation";
const string ApkFrameInfo::APK_NAME      = "desc";
const string ApkFrameInfo::APK_DESC_TYPE = "raw";
const string ApkFrameInfo::APK_ANIM_TYPE = "drawable";

shared_ptr<istream> ApkFrameInfo::frame (int idx) {
	const ResTable& resTable = assetManager->getResources();

	if (idx >= max_frame) {
		FPLog.E()<<"frame ignore overflow frame index cur_frame="<<idx<<" max_frame="<<max_frame<<endl;
		idx = max_frame -1;
	}
	string name = info.frames[idx];

	String16 s16_name = String16(name.c_str(), name.length());
	String16 s16_type = String16(APK_ANIM_TYPE.c_str());
	String16 s16_pkg  = String16(APK_PACKAGE.c_str());
	uint32_t id = resTable.identifierForName(s16_name, s16_name.size(),
		s16_type, s16_type.size(), s16_pkg, s16_pkg.size());
	if (id == 0) {
		FPLog.E()<<"identifierForName "<<name<<" fail";
		throw io_exception("Can't find Resource Entry " + name);
	}

	FPLog.I()<<"resource Id="<<id<<endl;
	Res_value value;
	ssize_t index = resTable.getResource(id, &value);

	FPLog.I()<<"id="<<id<<" index="<<index<<endl;
	auto_ptr<const ResStringPool> strPool(resTable.getTableStringBlock(index));
	if (!strPool.get()) {
		FPLog.E()<<"obtain ResStringPool fail"<<endl;
		throw io_exception("Can't find StringPool " + name);
	}

	FPLog.I()<<"string8At "<<value.data<<endl;
	size_t len;
	const char* str = strPool->string8At(value.data, &len);

	string real_path(str, len);
	FPLog.I()<<"find real_path="<<real_path<<endl;
	shared_ptr<Asset> asset(assetManager->openNonAsset(real_path.c_str(), Asset::ACCESS_STREAMING));

	return shared_ptr<istream>(new istream(new ResStreamBuf(asset)));
}

string ApkFrameInfo::parse_anim_file () {
	const ResTable& resTable = assetManager->getResources();
	String16 s16_name(APK_NAME.c_str());
	String16 s16_type(APK_DESC_TYPE.c_str());
	String16 s16_pkg(APK_PACKAGE.c_str());

	uint32_t id = resTable.identifierForName(s16_name, s16_name.size(),
		s16_type, s16_type.size(), s16_pkg, s16_pkg.size());
	if (id == 0) {
		FPLog.E()<<"obtain identifier "<<APK_NAME<<" fail";
		return string();
	}

	Res_value value;
	ssize_t index = resTable.getResource(id, &value);

	FPLog.I()<<"string index="<<index<<endl;
	const ResStringPool *stringPool = resTable.getTableStringBlock(index);

	size_t len;
	const char *str = stringPool->string8At(value.data, &len);
	string real_path(str, len);

	auto_ptr<Asset> asset(assetManager->openNonAsset(real_path.c_str(), Asset::ACCESS_STREAMING));
	string desc_str(static_cast<const char*>(asset->getBuffer(true)), asset->getLength());
	asset->close();

	return desc_str;
}

// ----------------------------------------------
shared_ptr<istream> DIRFrameInfo::frame (int idx) {
	if (idx >= max_frame) {
		FPLog.E()<<"next_frame ignore overflow frame index cur_frame="<<idx<<" max_frame="<<max_frame<<endl;
		idx = max_frame -1;
	}

	string frame_name = info.frames[idx];
	string path = parent_path + "/" + info.frame_path + "/" + frame_name;

	shared_ptr<istream> ifm(new ifstream(path));
	if (!ifm->good()) {
		FPLog.E()<<"frame open "<<path<<" failed"<<endl;
		return shared_ptr<istream>(nullptr);
	}

	return ifm;
}

string DIRFrameInfo::parse_anim_file () {
	string path = parent_path + "/" + ENTRY_DESC;

	ifstream ifm(path);
	if (!ifm.good()) {
		FPLog.E()<<"parse_dir_frame open "<<path<<" fail"<<endl;
		ifm.close();
		return string();
	}

	string desc_str;
	string tmp;
	while (ifm>>tmp) {
		desc_str += tmp;
	}
	ifm.close();

	return desc_str;
}

}; //namespace frame_animation
