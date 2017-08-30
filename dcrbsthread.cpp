#include "dcrbsthread.h"
#include "decoder/mpeg2frame.h"
#include "decoder/avsframe.h"
#include <QFile>
#include <QMutex>
#include <QDebug>
#include <QWaitCondition>

DcrBSThread::DcrBSThread(QFile *fp, Decoder decoder)
{
    frameRate = decoder.nFrameRate;
    file = fp;
    type = decoder.de;
    bStreamType = decoder.isStreamType;
}



void DcrBSThread::run()
{
    switch(type)
    {
    case H265:
        break;
    case H264:
        break;
    case MPEG2:
        decribeMpeg2Info(frameRate);
        break;
    case AVS:
        decribeAvsInfo(frameRate);
        break;
    default:
        break;
    }
}

void DcrBSThread::decribeMpeg2Info(const qint16 nFrameRate)
{
    qDebug("***********************nFrameRate:%d", nFrameRate);
    qint32 nFrameStreamSize = 0;
    qint32 size = 0;
    qint32 leftSize = 0;
    qint32 nSecFrameStreamBitSize = 0;
    qint16 count = 0;
    qint32 second = 0;
    qint32 nReadSize = 0;
    qint32 bitRate;
    unsigned char *pBuf;
    qint64 length = 0;
    qDebug("come to decribe");
    Mpeg2Frame *mpeg2Frame = new Mpeg2Frame();
    while(!file->atEnd() && file->isReadable())
    {
        char inbuf[1024];
       // qDebug("into while");

        length = file->readLine(inbuf, sizeof(inbuf));
        //qDebug("read length = %d", length);
        if(length == -1)
        {
            break;
        }
        nReadSize += length;
        emit sendTotalReadBits(nReadSize);
        pBuf = (unsigned char *)inbuf;
        leftSize = length;
        while(leftSize > 0)
        {
            size = mpeg2Frame->catOneMpeg2StreamFrame(pBuf, leftSize);
            //qDebug("size = %d", size);
            if(size == -1)
            {
                nFrameStreamSize += leftSize;
                size = 0;
                break;
            }
            else
            {
                leftSize -= size;
                pBuf += size;
                nFrameStreamSize += size;
                //qDebug("nFrameSize = %d", nFrameStreamSize);
                nSecFrameStreamBitSize += nFrameStreamSize * 8;
                nFrameStreamSize = 0;
                count++;
                if(count == nFrameRate)
                {
                    count = 0;
                    second++;
                    bitRate = nSecFrameStreamBitSize/1024;
                    qDebug("the second is %d, bitStream is %dKbps", second, bitRate);
                    emit sendInfo(second, bitRate);
                    nSecFrameStreamBitSize = 0;
                }
            }
        }
    }
   // file->close();
    qDebug("thread end");
    this->quit();
    return;
}

void DcrBSThread::decribeAvsInfo(const qint16 nFrameRate)
{
    qDebug("***********************nFrameRate:%d", nFrameRate);
    qint32 nFrameStreamSize = 0;
    qint32 size = 0;
    qint32 leftSize = 0;
    qint32 nSecFrameStreamBitSize = 0;
    qint16 count = 0;
    qint32 second = 0;
    qint32 nReadSize = 0;
    qint32 bitRate;
    unsigned char *pBuf;
    qint64 length = 0;
    qDebug("come to decribe");
    AvsFrame *avsFrame = new AvsFrame();
    while(!file->atEnd() && file->isReadable())
    {
        char inbuf[1024];
       // qDebug("into while");

        length = file->readLine(inbuf, sizeof(inbuf));
        //qDebug("read length = %d", length);
        if(length == -1)
        {
            break;
        }
        nReadSize += length;
        emit sendTotalReadBits(nReadSize);
        pBuf = (unsigned char *)inbuf;
        leftSize = length;
        while(leftSize > 0)
        {
            size = avsFrame->catOneAvsStreamFrame(pBuf, leftSize);
            //qDebug("size = %d", size);
            if(size == -1)
            {
                nFrameStreamSize += leftSize;
                size = 0;
                break;
            }
            else
            {
                leftSize -= size;
                pBuf += size;
                nFrameStreamSize += size;
                //qDebug("nFrameSize = %d", nFrameStreamSize);
                nSecFrameStreamBitSize += nFrameStreamSize * 8;
                nFrameStreamSize = 0;
                count++;
                if(count == nFrameRate)
                {
                    count = 0;
                    second++;
                    bitRate = nSecFrameStreamBitSize/(1024*1024);
                    qDebug("the second is %d, bitStream is %dMbps", second, bitRate);
                    emit sendInfo(second, bitRate);
                    nSecFrameStreamBitSize = 0;
                }
            }
        }
    }
    file->close();
    qDebug("thread end");
    this->quit();
    return;

}
