#include "FrameParser.h"

namespace frame_animation {

/* desc frame type */
const string FrameParser::FRAME_TYPE_ZIP_STR = "zip";
const string FrameParser::FRAME_TYPE_APK_STR = "apk";
const string FrameParser::FRAME_TYPE_DIR_STR = "dir";

bool ends_with (const string& str, const string& suffix) {
    return str.rfind(suffix) == (str.length() - suffix.length());
}

shared_ptr<FrameInfo> FrameParser::parse (const string& path) {
    string type_str;
    if (ends_with(path, FRAME_TYPE_ZIP_STR))
        type_str = FRAME_TYPE_ZIP_STR;
    else if (ends_with(path, FRAME_TYPE_APK_STR))
        type_str = FRAME_TYPE_APK_STR;
    else
        type_str = FRAME_TYPE_DIR_STR;

    return FrameInfo::create_from_type(path, frame_type(type_str));
}

AnimResType FrameParser::frame_type (const string& value) {
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
