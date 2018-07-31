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

#include <androidfw/ResourceTypes.h>

#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#include "FrameDisplay.h"
#include "FrameError.h"

namespace frame_animation {

DisplayMetrics::DisplayMetrics () {
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain));
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dInfo);
    if (status)
        throw structor_exception("getDisplayInfo fail");
}

int DisplayMetrics::density () const {
    switch (width()) {
        case 540: case 480:
            return ResTable_config::DENSITY_HIGH;   //hdpi
        case 720:
            return ResTable_config::DENSITY_XHIGH;  //xhdpi
        case 1080:
            return ResTable_config::DENSITY_XXHIGH; //xxhdpi
        case 1920:
            return ResTable_config::DENSITY_XXHIGH; //xxxhdpi
        default:
            return ResTable_config::DENSITY_DEFAULT;  //lhdpi
    }
}

}; //namespace frame_animation
