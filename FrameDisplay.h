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
