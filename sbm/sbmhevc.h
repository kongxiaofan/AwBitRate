#ifndef SBMHEVC_H
#define SBMHEVC_H

#include "sbm/sbm.h"
#include <QThread>

class SbmHevc : public Sbm, public QThread
{
public:
    SbmHevc();
    void run();
protected:
     int checkBitStreamTypeWithStartCode(VideoStreamDataInfo *pStream);
     int checkBitStreamTypeWithoutStartCode(VideoStreamDataInfo *pStream);
     int checkBitStreamType();
     int searchStartCode(int* pAfterStartCodeIdx);
     void detectWithStartCode();
     void detectWithoutStartCode();
     void detectOneFramePic();
};

#endif // SBMHEVC_H
