#ifndef DCRBSTHREAD_H
#define DCRBSTHREAD_H

#include <QThread>
#include <QMutex>
#include "include/infor.h"
class QFile;

class DcrBSThread : public QThread
{
     Q_OBJECT
public:
    DcrBSThread(QFile *fp, Decoder decoder);
protected:
    void run();

signals:
    void sendTotalReadBits(qint32 bits);
    void sendInfo(qint32 second, qint32 bitRate);
private:
    qint16 frameRate;
    QFile *file;
    DecoderType type;
    bool bStreamType;
    void decribeMpeg2Info(const qint16 nFrameRate);
    void decribeAvsInfo(const qint16 nFrameRate);

};

#endif // DCRBSTHREAD_H
