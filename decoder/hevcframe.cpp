#include "hevcframe.h"
#include <QFile>

HevcFrame::HevcFrame(QFile *fp, Decoder dec)
{
    pSbm = new SbmHevc();
    int size = pSbm->decideStreamBufferSize(dec.wigth, dec.heigth);
    pSbm->init(size);
    pSbm->setEos(false);
    sb = new H265Submit(fp, dec, pSbm);
    cs = new H265Cosum(pSbm, dec);
}

HevcFrame::~HevcFrame()
{

}

void HevcFrame::caculate()
{
    sb->start();
    pSbm->start();
    cs->start();
    sb->wait();
    cs->wait();
    pSbm->destroy();
}



H265Submit::H265Submit(QFile *fp, Decoder dec, SbmHevc *tSbm)
{
    pSbm = tSbm;
    file = fp;
    decoder = dec;
}

void H265Submit::run()
{
    char*               pBuf0;
    char*               pBuf1;
    int                 nBufSize0;
    int                 nBufSize1;
    char pData[1024];
    int                 nLength;
    VideoStreamDataInfo pDataInfo;
    qDebug("submint thread run, file = %p", file);

    while(!file->atEnd() && file->isReadable())
    {
        nLength = (int)file->readLine(pData, 1024);//how big??
        //qDebug("read length = %d", nLength);
        //request  buffer
        while(1)
        {
            int nRet = pSbm->requestStreamBuffer(nLength, &pBuf0, &nBufSize0, &pBuf1, &nBufSize1);
            //qDebug("nRet = %d, nBufSize0 = %d, nBufSize1 = %d, nLength = %d", nRet, nBufSize0, nBufSize1, nLength);
            if(nRet < 0)
            {
                sleep(10);
                qDebug("wait for buffer");
                //do something
            }
            else
                break;
        }
        //submit data
        if(nLength > nBufSize0)
        {
            memcpy(pBuf0, pData, nBufSize0);
            memcpy(pBuf1, pData + nBufSize0, nLength-nBufSize0);
        }
        else
            memcpy(pBuf0, pData, nLength);

        pDataInfo.pData        = pBuf0;
        pDataInfo.nLength      = nLength;
        pSbm->addStream(&pDataInfo);
    }
    qDebug("end of thread");
    file->seek(0);
    pSbm->setEos(true);
    return;
}


H265Cosum::H265Cosum(SbmHevc *tSbm, Decoder dec)
{
    pSbm = tSbm;
    decoder = dec;
}

void H265Cosum::run()
{
    int second = 0;
    int bitRate = 0;
    int nCount = 0;

    while(1)
    {
        FramePicInfo*        pFramePic = NULL;
        //request one pic
        pFramePic = pSbm->requestFramePic();
        qDebug("pFramePic = %p", pFramePic);
        if(pFramePic == NULL)
        {
            if(!pSbm->isEos())
            {
                sleep(10);
                continue;
            }
            else
                break;
        }
        //do other thing
        bitRate += pFramePic->nLength;
        nCount++;
        second++;
        if(nCount == decoder.nFrameRate)
        {
            nCount = 0;
            emit sendInfo(second, bitRate);
        }

        //flush the pic
        pSbm->flushFramePic(pFramePic);
    }
    qDebug("exit the comsum thread");
    return;
}
