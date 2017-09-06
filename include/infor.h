#ifndef INFOR_H
#define INFOR_H

#include <QDebug>

typedef enum{H265 = 1, H264, MPEG2, AVS}DecoderType;
typedef struct
{
    DecoderType type;
    int nFrameRate;
    bool isStreamType;
    int wigth;
    int heigth;
}Decoder;

#define logd(x) (qWarning() << __TIME__ << "D " << __FILE__ << ":" \
    << "<" << __FUNCTION__ << ":" << __LINE__ << ">:"<< (x))

#define logw(x) (qWarning() << __TIME__ << "W" << __FILE__ << ":" \
    << "<" << __FUNCTION__ << ":" << __LINE__ << ">:"<< (x))

#define loge(x) (qCritical() << __TIME__ << "E" << __FILE__ << ":" \
    << "<" << __FUNCTION__ << ":" << __LINE__ << ">:"<< (x))
#endif // INFOR_H
