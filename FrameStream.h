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

#ifndef _FRAME_STREAM_H_
#define _FRAME_STREAM_H_

#include <ostream>
#include <streambuf>
#include <string>
#include <vector>
#include <androidfw/ZipFileRO.h>
#include <androidfw/Asset.h>
#include <android/log.h>

using namespace std;
using namespace android;

namespace frame_animation {

class fpstream : public ostream {
    fpstream& log_priority (android_LogPriority pri);
public:
    fpstream ();
    fpstream& I() { return log_priority(ANDROID_LOG_INFO);  }
    fpstream& E() { return log_priority(ANDROID_LOG_ERROR); }
    fpstream& D() { return log_priority(ANDROID_LOG_DEBUG); }
    fpstream& W() { return log_priority(ANDROID_LOG_WARN);  }
};

extern fpstream FPLog;

// ------------------------------------------------------
class ResStreamBuf : public streambuf {
    shared_ptr<Asset> asset;
    char *buf;
    int length;
public:
    ResStreamBuf (shared_ptr<Asset> asset);
    virtual int underflow();
protected:
    virtual pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode which) override;
    virtual pos_type seekpos(pos_type pos, ios_base::openmode mode = ios_base::in | ios_base::out) override;
};

class ZipStreamBuf : public streambuf {
    shared_ptr<ZipFileRO> zip_file;
    string file_name;
    unique_ptr<FileMap> file_map;
    int length;
public:
    ZipStreamBuf (shared_ptr<ZipFileRO>, string);
    virtual int underflow() override;
protected:
    virtual pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode which) override;
    virtual pos_type seekpos(pos_type pos, ios_base::openmode mode = ios_base::in | ios_base::out) override;
};

}; //namespace frame_animation

#endif
