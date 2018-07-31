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

#ifndef _FRAME_DISPLAY_H_
#define _FRAME_DISPLAY_H_

#include <ui/DisplayInfo.h>

using namespace std;
using namespace android;

namespace frame_animation {

class DisplayMetrics {
    DisplayInfo dInfo;
public:
    DisplayMetrics ();

    int density () const;
    int width () const {
        return dInfo.w;
    }

    int height () const {
        return dInfo.h;
    }
};

}; //namespace android

#endif
