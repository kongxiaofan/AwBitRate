#include "dcrbsthread.h"
#include "decoder/mpeg2frame.h"
#include "decoder/avsframe.h"
#include <QFile>
#include <QMutex>
#include <QDebug>
#include <QWaitCondition>
#include "decoder/hevcframe.h"

DcrBSThread::DcrBSThread(QFile *fp, Decoder dec)
{
    file = fp;
    decoder = dec;
}



void DcrBSThread::run()
{
    switch(decoder.type)
    {
    case H265:
        decribeHevcInfo(file, decoder);
        break;
    case H264:
        break;
    case MPEG2:
        decribeMpeg2Info(decoder.nFrameRate);
        break;
    case AVS:
        //decribeAvsInfo(frameRate);
        break;
    default:
        break;
    }
}

void DcrBSThread::recvInfo(qint32 second, qint32 bitRate)
{
    emit sendInfo(second, bitRate);
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

void DcrBSThread::decribeHevcInfo(QFile *fp, Decoder decoder)
{
    HevcFrame *hevc = new HevcFrame(fp, decoder);
    hevc->caculate();
    connect(hevc->cs, SIGNAL(sendInfo(int,int)), SLOT(recvInfo(qint32,qint32)));
}

