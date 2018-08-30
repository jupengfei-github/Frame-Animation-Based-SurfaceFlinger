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
