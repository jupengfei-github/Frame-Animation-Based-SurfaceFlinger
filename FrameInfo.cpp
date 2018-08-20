#include <fstream>
#include <androidfw/ResourceTypes.h>
#include <androidfw/AssetManager.h>
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
const string FrameInfo::DESC_KEY_FRAME_TYPE = "frame_type";

int FrameInfo::count() {
	return max_frame;
}

int FrameInfo::idx() {
	return cur_frame;
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

void FrameInfo::reset () {
	cur_frame = 0;
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
	FPLog.I()<<"frame_path : "<<frame_desc.frame_path<<endl;
	FPLog.I()<<"frame_type : "<<type<<endl;
	FPLog.I()<<"frame_mode : "<<frame_desc.frame_mode<<endl;
	FPLog.I()<<"frame_rate : "<<frame_desc.frame_rate<<endl;
	FPLog.I()<<"frame_resolution : ["<<frame_desc.resolution.width<<"x"<<frame_desc.resolution.height<<"]"<<endl;
}

void FrameInfo::parse_desc_item(const string&, const string&) {
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

void FrameInfo::parse_anim_info (string& path) {
	string anim_str = parse_anim_file(path);

	regex item_pattern("\\s*(\\w+)\\s*:\\s*\\[([^\\[\\]]+)\\]");
	smatch item_match;

	string ite_str = anim_str;
	while (regex_search(ite_str, item_match, item_pattern)) {
		string key_item(item_match[1].first, item_match[1].second);
		string value_item(item_match[2].first, item_match[2].second);
		parse_desc_item(key_item, value_item);

		ite_str = item_match.suffix().str();
	}
}

// ------------------------------------------------
shared_ptr<istream> ZipFrameInfo::next_frame () {
	if (cur_frame >= max_frame)
		FPLog.E()<<"next_frame ignore overflow frame index cur_frame="<<cur_frame<<" max_frame="<<max_frame<<endl;
	else
		cur_frame++;

	string frame_name = info.frames[cur_frame];
	return shared_ptr<istream>(new istream(new ZipStreamBuf(zip_file, info.frame_path + "/" + frame_name)));
}

string ZipFrameInfo::parse_anim_file (const string& path) {
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

	return desc_str;
}

// -----------------------------------------------
shared_ptr<istream> ApkFrameInfo::next_frame () {
	const ResTable& resTable = assetManager->getResources();

	if (cur_frame >= max_frame)
		FPLog.E()<<"next_frame ignore overflow frame index cur_frame="<<cur_frame<<" max_frame="<<max_frame<<endl;
	else
		cur_frame++;
	string name = info.frames[cur_frame];

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

string ApkFrameInfo::parse_anim_file (const string& path) {
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

	return desc_str;
}

// ----------------------------------------------
shared_ptr<istream> DIRFrameInfo::next_frame () {
	if (cur_frame >= max_frame)
		FPLog.E()<<"next_frame ignore overflow frame index cur_frame="<<cur_frame<<" max_frame="<<max_frame<<endl;
	else
		cur_frame++;

	string frame_name = info.frames[cur_frame];
	shared_ptr<istream> ifm(new ifstream(path + "/" + frame_name));
	if (!ifm->good()) {
		FPLog.E()<<"next_frame open "<<path<<" failed"<<endl;
		return shared_ptr<istream>(nullptr);
	}

	return ifm;
}

string DIRFrameInfo::parse_anim_file (const string& path) {
	ifstream ifm(path + "/" + ENTRY_DESC);
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
