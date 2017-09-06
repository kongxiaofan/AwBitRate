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
    DcrBSThread(QFile *fp, Decoder dec);
protected:
    void run();

signals:
    void sendTotalReadBits(qint32 bits);
    void sendInfo(qint32 second, qint32 bitRate);

private slots:
    void recvInfo(qint32 second, qint32 bitRate);
private:
    Decoder decoder;
    QFile *file;

    bool bStreamType;
    void decribeMpeg2Info(const qint16 nFrameRate);
    void decribeHevcInfo(QFile *fp, Decoder decoder);

};

#endif // DCRBSTHREAD_H
