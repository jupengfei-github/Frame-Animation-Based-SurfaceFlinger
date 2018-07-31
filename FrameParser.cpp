/*
 * Copyright (C) 2018-2024 The Surface Frame-Animation Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
