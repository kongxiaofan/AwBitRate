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


#if 0
#define logd(x) (qWarning() << __TIME__ << "D " << __FILE__ << ":" \
    << "<" << __FUNCTION__ << ":" << __LINE__ << ">:"<< (x))

#define logw(x) (qWarning() << __TIME__ << "W" << __FILE__ << ":" \
    << "<" << __FUNCTION__ << ":" << __LINE__ << ">:"<< (x))

#define loge(x) (qCritical() << __TIME__ << "E" << __FILE__ << ":" \
    << "<" << __FUNCTION__ << ":" << __LINE__ << ">:"<< (x))
#endif

#ifndef LOG_TAG
#define LOG_TAG "AwbitRate"
#endif

#define LOG_LEVEL_ERROR     "error  "
#define LOG_LEVEL_WARNING   "warning"
#define LOG_LEVEL_INFO      "info   "
#define LOG_LEVEL_VERBOSE   "verbose"
#define LOG_LEVEL_DEBUG     "debug  "

#define AWLOG(level, fmt, arg...)  \
    qDebug("%s %s: %s <%s:%u>: "fmt, __TIME__, level, __FILE__, __FUNCTION__, __LINE__, ##arg)

#define logd(fmt, arg...) AWLOG(LOG_LEVEL_DEBUG, fmt, ##arg)

#define loge(fmt, arg...) AWLOG(LOG_LEVEL_ERROR, fmt, ##arg)
#define logw(fmt, arg...) AWLOG(LOG_LEVEL_WARNING, fmt, ##arg)
#define logi(fmt, arg...) AWLOG(LOG_LEVEL_INFO, fmt, ##arg)
#define logv(fmt, arg...) AWLOG(LOG_LEVEL_VERBOSE, fmt, ##arg)

#endif // INFOR_H
