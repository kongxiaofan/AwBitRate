#include "sbm.h"
#include <QDebug>
#include <QMutex>

#define SBM_FRAME_FIFO_SIZE (2048)  //* store 2048 frames of bitstream data at maximum.
#define MAX_FRAME_PIC_NUM (100)
#define DEFAULT_NALU_NUM (32)
#define MAX_INVALID_STREAM_DATA_SIZE (1*1024*1024) //* 1 MB
#define MAX_NALU_NUM_IN_FRAME (1024)

QMutex mutex;

Sbm::Sbm()
{
}

int Sbm::init(int size)
{
    int i;
    if(size <= 0)
    {
        //qDebug("the size is error");
        return -1;
    }

    pStreamBuffer = (char*)malloc(size);
    if(pStreamBuffer == NULL)
    {
        //qDebug("malloc for sbmBuf failed");
        return -1;
    }

    frameFifo.pFrames = (VideoStreamDataInfo *)malloc(SBM_FRAME_FIFO_SIZE
                                                 * sizeof(VideoStreamDataInfo));
    if(frameFifo.pFrames == NULL)
    {
        goto ERROR;
    }
    memset(frameFifo.pFrames, 0,  SBM_FRAME_FIFO_SIZE * sizeof(VideoStreamDataInfo));

    pStreamBufferEnd  = pStreamBuffer + size - 1;
    nStreamBufferSize = size;
    pWriteAddr        = pStreamBuffer;
    nValidDataSize    = 0;


    frameFifo.nMaxFrameNum     = SBM_FRAME_FIFO_SIZE;
    frameFifo.nValidFrameNum   = 0;
    frameFifo.nUnReadFrameNum  = 0;
    frameFifo.nReadPos         = 0;
    frameFifo.nWritePos        = 0;
    frameFifo.nFlushPos        = 0;

    mFramePicFifo.pFramePics = (FramePicInfo*)malloc(MAX_FRAME_PIC_NUM*sizeof(FramePicInfo));
    if(mFramePicFifo.pFramePics == NULL)
    {
        //qDebug("malloc for framePic failed");
        goto ERROR;
    }
    memset(mFramePicFifo.pFramePics, 0, MAX_FRAME_PIC_NUM*sizeof(FramePicInfo));
    mFramePicFifo.nMaxFramePicNum = MAX_FRAME_PIC_NUM;

    for(i = 0; i < MAX_FRAME_PIC_NUM; i++)
    {
        mFramePicFifo.pFramePics[i].pNaluInfoList
            = (NaluInfo*)malloc(DEFAULT_NALU_NUM*sizeof(NaluInfo));
        if(mFramePicFifo.pFramePics[i].pNaluInfoList == NULL)
        {
            //qDebug("malloc for naluInfo failed");
            goto ERROR;
        }
        memset(mFramePicFifo.pFramePics[i].pNaluInfoList, 0,
               DEFAULT_NALU_NUM*sizeof(NaluInfo));
        mFramePicFifo.pFramePics[i].nMaxNaluNum = DEFAULT_NALU_NUM;
    }

    stateCmd = SBM_THREAD_CMD_READ;

    bStreamWithStartCode = -1;
    return 0;

ERROR:
    if(pStreamBuffer)
        free(pStreamBuffer);

    if(frameFifo.pFrames)
        free(frameFifo.pFrames);

    if(mFramePicFifo.pFramePics)
    {
        for(i=0; i < MAX_FRAME_PIC_NUM; i++)
        {
            if(mFramePicFifo.pFramePics[i].pNaluInfoList)
                free(mFramePicFifo.pFramePics[i].pNaluInfoList);
        }
        free(mFramePicFifo.pFramePics);
    }
    return -1;
}

void Sbm::destroy()
{
    qDebug(" sbm destroy");
    stateCmd = SBM_THREAD_CMD_QUIT;

    if(pStreamBuffer != NULL)
    {
        free(pStreamBuffer);
        pStreamBuffer = NULL;
    }

    if(frameFifo.pFrames != NULL)
    {
        free(frameFifo.pFrames);
        frameFifo.pFrames = NULL;
    }

    if(mFramePicFifo.pFramePics)
    {
        int i=0;
        for(i=0; i < MAX_FRAME_PIC_NUM; i++)
        {
            if(mFramePicFifo.pFramePics[i].pNaluInfoList)
                free(mFramePicFifo.pFramePics[i].pNaluInfoList);
        }
        free(mFramePicFifo.pFramePics);
    }

    qDebug(" sbm destroy finish");
}

void Sbm::reset()
{
    qDebug("SbmFrameReset");
    stateCmd = SBM_THREAD_CMD_RESET;

    qDebug("** wait for reset sem");
    qDebug("** wait for reset sem ok");
    qDebug("SbmFrameReset finish");
    return;
}

int Sbm::decideStreamBufferSize(int width, int heigth)
{
    int nBufferSize = 8*1024*1024;

    if(width == 0)
        width = 1920;

    if(width < 480)
        nBufferSize = 2*1024*1024;
    else if (width < 720)
        nBufferSize = 4*1024*1024;
    else if(width < 1080)
        nBufferSize = 6*1024*1024;
    else if(width < 2160)
        nBufferSize = 8*1024*1024;
    else
        nBufferSize = 32*1024*1024;
    if(width >= 3840 && nBufferSize<=32*1024*1024)
    {
        nBufferSize = 32*1024*1024;
    }
    qDebug("init size = %d", nBufferSize);
    return nBufferSize;
}

void *Sbm::getBuffAddr()
{
    return pStreamBuffer;
}

int Sbm::getBuffSize()
{
    return nStreamBufferSize;
}

int Sbm::getStreamFrameNum()
{
    return frameFifo.nValidFrameNum + mFramePicFifo.nValidFramePicNum;
}

int Sbm::getStreamDataSize()
{
    return nValidDataSize;
}

int Sbm::requestStreamBuffer(int nRequireSize, char **ppBuf, int *pBufSize, char **ppRingBuf, int *pRingBufSize)
{
    char*                pStart;
    char*                pStreamBufEnd;
    char*                pMem;
    int                  nFreeSize;

    *ppBuf        = NULL;
    *ppRingBuf    = NULL;
    *pBufSize     = 0;
    *pRingBufSize = 0;

    if(nRequireSize == 0)
        nRequireSize = 4;


    if(sbmRequestBuffer(nRequireSize, &pMem, &nFreeSize) < 0)
    {
        //qDebug("sbmRequestBuffer return -1");
        return -1;
    }


    //* calculate the output buffer pos.
    pStreamBufEnd = (char*)getBuffAddr() + getBuffSize();
    pStart        = pMem;
    if(pStart >= pStreamBufEnd)
        pStart -= getBuffSize();

    if(pStart + nFreeSize <= pStreamBufEnd) //* check if buffer ring back.
    {
        *ppBuf    = pStart;
        *pBufSize = nFreeSize;
    }
    else
    {
        //* the buffer ring back.
        *ppBuf        = pStart;
        *pBufSize     = pStreamBufEnd - pStart;
        *ppRingBuf    = (char*)getBuffAddr();
        *pRingBufSize = nFreeSize - *pBufSize;
    }

    return 0;
}

char* Sbm::getBufferWritePointer()
{
    return pWriteAddr;
}

int Sbm::addStream(VideoStreamDataInfo *pDataInfo)
{
    //qDebug("addStream");
    int nWritePos;
    char *pNewWriteAddr;

    if(pDataInfo == NULL)
    {
        qDebug("input error.");
        return -1;
    }
    mutex.lock();

    if(pDataInfo->pData == 0)
    {
        qDebug("data buffer is NULL.\n");
        mutex.unlock();
        return -1;
    }

    if(frameFifo.nValidFrameNum >= frameFifo.nMaxFrameNum)
    {
        qDebug("nValidFrameNum > nMaxFrameNum.");
        mutex.unlock();
        return -1;
    }

    if(pDataInfo->nLength + nValidDataSize > nStreamBufferSize)
    {
        qDebug("no free buffer.");
        mutex.unlock();
        return -1;
    }


    nWritePos = frameFifo.nWritePos;
    //qDebug("nWritePos = %d, &pFrames[nWritePos] = %p", nWritePos, &frameFifo.pFrames[nWritePos]);
    memcpy(&frameFifo.pFrames[nWritePos], pDataInfo, sizeof(VideoStreamDataInfo));
    nWritePos++;
    if(nWritePos >= frameFifo.nMaxFrameNum)
    {
        nWritePos = 0;
    }

    frameFifo.nWritePos = nWritePos;
    frameFifo.nValidFrameNum++;
    frameFifo.nUnReadFrameNum++;
    nValidDataSize += pDataInfo->nLength;
    pNewWriteAddr = pWriteAddr + pDataInfo->nLength;
    if(pNewWriteAddr > pStreamBufferEnd)
    {
        pNewWriteAddr -= nStreamBufferSize;
    }

    pWriteAddr = pNewWriteAddr;

    mutex.unlock();
    return 0;
}


FramePicInfo* Sbm::requestFramePic()
{
    qDebug("requestFramePic");
    FramePicInfo* pFramePic = NULL;

    mutex.lock();

    if(mFramePicFifo.nUnReadFramePicNum == 0)
    {
        qDebug("nUnReadFramePicNum == 0.");
        mutex.unlock();
        return NULL;
    }

    pFramePic = &mFramePicFifo.pFramePics[mFramePicFifo.nFPReadPos];
    if(pFramePic == NULL)
    {
        qDebug("request framePic failed");
        mutex.unlock();
        return NULL;
    }

    mFramePicFifo.nFPReadPos++;
    mFramePicFifo.nUnReadFramePicNum--;
    qDebug("nFpReadPos = %d. nUnReadFramePicNum = %d", mFramePicFifo.nFPReadPos, mFramePicFifo.nUnReadFramePicNum);
    if(mFramePicFifo.nFPReadPos >= mFramePicFifo.nMaxFramePicNum)
    {
        mFramePicFifo.nFPReadPos = 0;
    }
    mutex.unlock();
    return pFramePic;
}

int Sbm::returnFramePic(FramePicInfo* pFramePic)
{
    int nReadPos;

    if(pFramePic == NULL)
    {
        qDebug("input error.");
        return -1;
    }

    mutex.lock();

    if(mFramePicFifo.nValidFramePicNum == 0)
    {
        qDebug("nValidFrameNum == 0.");
        mutex.unlock();
        return -1;
    }

    nReadPos = mFramePicFifo.nFPReadPos;
    nReadPos--;
    if(nReadPos < 0)
    {
        nReadPos = mFramePicFifo.nMaxFramePicNum - 1;
    }

    if(pFramePic != &mFramePicFifo.pFramePics[nReadPos])
    {
        qDebug("wrong frame pic sequence.");
        abort();
    }

    mFramePicFifo.nFPReadPos = nReadPos;
    mFramePicFifo.nUnReadFramePicNum++;
    mutex.unlock();
    return 0;
}

int Sbm::flushFramePic(FramePicInfo* pFramePic)
{
    qDebug("flushFramePic");
    int nFlushPos;

    if(pFramePic == NULL)
    {
        qDebug("input error");
        return -1;
    }

    mutex.lock();

    if(mFramePicFifo.nValidFramePicNum == 0)
    {
        qDebug("nValidFrameNum == 0.");
        mutex.unlock();
        return -1;
    }
    nFlushPos = mFramePicFifo.nFPFlushPos;
    qDebug("sbm flush stream , pos = %d, pFrame = %p, %p",nFlushPos,
          pFramePic, &mFramePicFifo.pFramePics[nFlushPos]);
    if(pFramePic != &mFramePicFifo.pFramePics[nFlushPos])
    {
        qDebug("not current nFlushPos.");
        abort();
    }

    nFlushPos++;
    if(nFlushPos >= mFramePicFifo.nMaxFramePicNum)
    {
        nFlushPos = 0;
    }

    mFramePicFifo.nValidFramePicNum--;
    mFramePicFifo.nFPFlushPos = nFlushPos;
    qDebug("validFramePicNum = %d",mFramePicFifo.nValidFramePicNum );
    nValidDataSize -= pFramePic->nLength;

    mutex.unlock();
    return 0;
}

void Sbm::setEos(bool bEos)
{
    qDebug("setEos");
    bEosFlag = bEos;
}

bool Sbm::isEos()
{
    return bEosFlag;
}

int Sbm::sbmRequestBuffer(int nRequireSize,char **ppBuf, int *pBufSize)
{
    //qDebug("sbmRequestBuffer");
    if(ppBuf == NULL || pBufSize == NULL)
    {
        qDebug("input error.");
        return -1;
    }

    mutex.lock();

    if(frameFifo.nValidFrameNum >= frameFifo.nMaxFrameNum)
    {
        qDebug("nValidFrameNum >= nMaxFrameNum.");
        mutex.unlock();
        return -1;
    }
    //qDebug("nValidDataSize = %d, nStreamBufferSize = %d", nValidDataSize, nStreamBufferSize);
    if(nValidDataSize < nStreamBufferSize)
    {
        int nFreeSize = nStreamBufferSize - nValidDataSize;
        if((nRequireSize + 64) > nFreeSize)
        {
            mutex.unlock();
            return -1;
        }

        *ppBuf    = pWriteAddr;
        *pBufSize = nRequireSize;

        mutex.unlock();
        return 0;
    }
    else
    {
        qDebug("no free buffer.");
        mutex.unlock();
        return -1;
    }
}

VideoStreamDataInfo* Sbm::requestStream()
{
    //qDebug("requestStream");
    VideoStreamDataInfo *pDataInfo;
    mutex.lock();

    if(frameFifo.nUnReadFrameNum == 0)
    {
       // qDebug("nUnReadFrameNum == 0.");
        mutex.unlock();
        return NULL;
    }

    pDataInfo = &frameFifo.pFrames[frameFifo.nReadPos];

    if(pDataInfo == NULL)
    {
        //qDebug("request failed.");
        mutex.unlock();
        return NULL;
    }

    frameFifo.nReadPos++;
    frameFifo.nUnReadFrameNum--;
    //qDebug("readPos = %d, nunReadFrameNum = %d", frameFifo.nReadPos, frameFifo.nUnReadFrameNum);
    if(frameFifo.nReadPos >= frameFifo.nMaxFrameNum)
    {
        frameFifo.nReadPos = 0;
    }
    mutex.unlock();

    //qDebug("*** reqeust stream, pDataInfo = %p, pos = %d",pDataInfo, frameFifo.nReadPos - 1);
   // qDebug("*** reqeust stream, data: %x %x %x %x %x %x %x %x ",
    //     pDataInfo->pData[0], pDataInfo->pData[1],pDataInfo->pData[2],pDataInfo->pData[3],
    //     pDataInfo->pData[4],pDataInfo->pData[5],pDataInfo->pData[6],pDataInfo->pData[7]);
    return pDataInfo;
}

int Sbm::returnStream(VideoStreamDataInfo *pDataInfo)
{
    int nReadPos;

    if(pDataInfo == NULL)
    {
        qDebug("input error.");
        return -1;
    }

    mutex.lock();

    if(frameFifo.nValidFrameNum == 0)
    {
        qDebug("nValidFrameNum == 0.");
        mutex.unlock();
        return -1;
    }
    nReadPos = frameFifo.nReadPos;
    nReadPos--;
    if(nReadPos < 0)
    {
        nReadPos = frameFifo.nMaxFrameNum - 1;
    }
    frameFifo.nUnReadFrameNum++;
    if(pDataInfo != &frameFifo.pFrames[nReadPos])
    {
        qDebug("wrong frame sequence.");
        abort();
    }

    frameFifo.pFrames[nReadPos] = *pDataInfo;
    frameFifo.nReadPos  = nReadPos;

    mutex.unlock();
    return 0;
}


int Sbm::flushStream(VideoStreamDataInfo *pDataInfo)
{
    //qDebug("flushStream");
    int nFlushPos;

    mutex.lock();

    if(frameFifo.nValidFrameNum == 0)
    {
        qDebug("no valid frame., flush pos = %d, pDataInfo = %p",
             frameFifo.nFlushPos, pDataInfo);
        mutex.unlock();
        return -1;
    }

    nFlushPos = frameFifo.nFlushPos;

    qDebug("flush stream, pDataInfo = %p, pos = %d, %p",
          pDataInfo, nFlushPos,&frameFifo.pFrames[nFlushPos]);
    if(pDataInfo != &frameFifo.pFrames[nFlushPos])
    {
        qDebug("not current nFlushPos.");
        mutex.unlock();
        abort();
        return -1;
    }

    nFlushPos++;
    if(nFlushPos >= frameFifo.nMaxFrameNum)
    {
        nFlushPos = 0;
    }

    frameFifo.nValidFrameNum--;
    //nValidDataSize     -= pDataInfo->nLength;
   // qDebug("validFrameNum = %d", frameFifo.nValidFrameNum);
    frameFifo.nFlushPos = nFlushPos;
    mutex.unlock();
    return 0;
}


FramePicInfo* Sbm::requestEmptyFramePicBuf()
{
    qDebug("requestEmptyFramePicBuf");
    int nWritePos = -1;
    FramePicInfo* pFramePic = NULL;

    mutex.lock();

    if(mFramePicFifo.nValidFramePicNum >= mFramePicFifo.nMaxFramePicNum)
    {
        qDebug("no emptye framePic");
        mutex.unlock();
        return NULL;
    }

    nWritePos = mFramePicFifo.nFPWritePos;
    pFramePic = &mFramePicFifo.pFramePics[nWritePos];

    mutex.unlock();
    qDebug("request empty frame pic, pos = %d, pFramePic = %p",nWritePos, pFramePic);
    return pFramePic;
}


int Sbm::addFramePic(FramePicInfo* pFramePic)
{
    qDebug("addFramePic");
    int nWritePos = -1;

    if(pFramePic == NULL)
    {
        qDebug("error input");
        return -1;
    }

    mutex.lock();

    if(mFramePicFifo.nValidFramePicNum >= mFramePicFifo.nMaxFramePicNum)
    {
        qDebug("nValidFrameNum >= nMaxFrameNum.");
        mutex.unlock();
        return -1;
    }

    nWritePos = mFramePicFifo.nFPWritePos;
    if(pFramePic != &mFramePicFifo.pFramePics[nWritePos])
    {
        qDebug("the frame pic is not match: %p, %p, %d",
              pFramePic, &mFramePicFifo.pFramePics[nWritePos], nWritePos);
        abort();
    }

    nWritePos++;
    if(nWritePos >= mFramePicFifo.nMaxFramePicNum)
    {
        nWritePos = 0;
    }

    mFramePicFifo.nFPWritePos = nWritePos;
    mFramePicFifo.nValidFramePicNum++;
    mFramePicFifo.nUnReadFramePicNum++;
    qDebug("FPwritePos = %d, validFramePicNum = %d, unReadFramePicNum = %d",
           nWritePos, mFramePicFifo.nValidFramePicNum, mFramePicFifo.nUnReadFramePicNum);
    mutex.unlock();
    return 0;
}

inline void Sbm::expandNaluList(FramePicInfo* pFramePic)
{
    qDebug("nalu num for one frame is not enought, expand it: %d, %d",
          pFramePic->nMaxNaluNum, pFramePic->nMaxNaluNum + DEFAULT_NALU_NUM);

    pFramePic->nMaxNaluNum += DEFAULT_NALU_NUM;
    pFramePic->pNaluInfoList = (NaluInfo*)realloc(pFramePic->pNaluInfoList,
                                       pFramePic->nMaxNaluNum*sizeof(NaluInfo));

}

void Sbm::initFramePicInfo(DetectFramePicInfo* pDetectInfo)
{
    qDebug("initFramePicInfo");
    FramePicInfo* pFramePic = mDetectInfo.pCurFramePic;
    pFramePic->nLength = 0;
    pFramePic->pDataStartAddr = NULL;
    pFramePic->nCurNaluIdx = 0;

    if(pFramePic->nMaxNaluNum > DEFAULT_NALU_NUM)
    {
        pFramePic->nMaxNaluNum   = DEFAULT_NALU_NUM;
        pFramePic->pNaluInfoList = (NaluInfo*)realloc(pFramePic->pNaluInfoList,
                                           pFramePic->nMaxNaluNum*sizeof(NaluInfo));
    }

    memset(pFramePic->pNaluInfoList, 0, pFramePic->nMaxNaluNum*sizeof(NaluInfo));

}

int Sbm::supplyStreamData()
{
    qDebug("supplyStreamData");
    char* pEnd   = pStreamBufferEnd;

    if(mDetectInfo.pCurStream)
    {
        flushStream( mDetectInfo.pCurStream);
        mDetectInfo.pCurStream = NULL;
        if(mDetectInfo.pCurFramePic)
        {
           mDetectInfo.pCurFramePic->nLength += mDetectInfo.nCurStreamDataSize;
        }
        mDetectInfo.nCurStreamDataSize = 0;
        mDetectInfo.pCurStreamDataPtr  = NULL;
    }

    VideoStreamDataInfo *pStream = requestStream();
    if(pStream == NULL)
    {
        qDebug("no bit stream");
        return -1;
    }
    mDetectInfo.pCurStream = pStream;
    mDetectInfo.pCurStreamDataPtr  = mDetectInfo.pCurStream->pData;
    mDetectInfo.nCurStreamDataSize = mDetectInfo.pCurStream->nLength;
    mDetectInfo.nCurStreamRebackFlag = 0;
    if((mDetectInfo.pCurStream->pData + mDetectInfo.pCurStream->nLength) > pEnd)
    {
        mDetectInfo.nCurStreamRebackFlag = 1;
    }
    return 0;

}


void Sbm::disposeInvalidStreamData()
{
    int bNeedAddFramePic = 0;
    qDebug("**1 pCurFramePic->nLength = %d, flag = %d",mDetectInfo.pCurFramePic->nLength,
         (mDetectInfo.pCurStreamDataPtr == mDetectInfo.pCurStream->pData));
    if(mDetectInfo.pCurStreamDataPtr == mDetectInfo.pCurStream->pData
       && mDetectInfo.pCurFramePic->nLength == 0)
    {
        mDetectInfo.pCurFramePic->pDataStartAddr = mDetectInfo.pCurStream->pData;
        mDetectInfo.pCurFramePic->nLength = mDetectInfo.pCurStream->nLength;
        bNeedAddFramePic = 1;
    }
    else
    {
        mDetectInfo.pCurFramePic->nLength += mDetectInfo.nCurStreamDataSize;
        qDebug("**2, pCurFramePic->nLength = %d, diff = %d",mDetectInfo.pCurFramePic->nLength,
             mDetectInfo.pCurFramePic->nLength - MAX_INVALID_STREAM_DATA_SIZE);

        if(mDetectInfo.pCurFramePic->nLength > MAX_INVALID_STREAM_DATA_SIZE)
        {
            bNeedAddFramePic = 1;
        }
    }

    qDebug("bNeedAddFramePic = %d",bNeedAddFramePic );
    flushStream(mDetectInfo.pCurStream);
    mDetectInfo.pCurStream = NULL;
    mDetectInfo.pCurStreamDataPtr = NULL;
    mDetectInfo.nCurStreamDataSize = 0;

    if(bNeedAddFramePic)
    {
        addFramePic(mDetectInfo.pCurFramePic);
        mDetectInfo.pCurFramePic = NULL;
    }
}


void Sbm::skipCurStreamDataBytes(int nSkipSize)
{
    qDebug("skipCurStreamDataBytes = %d", nSkipSize);
    char* pStart = pStreamBuffer;
    char* pEnd   = pStreamBufferEnd;
    mDetectInfo.pCurStreamDataPtr += nSkipSize;
    mDetectInfo.nCurStreamDataSize -= nSkipSize;
    if(mDetectInfo.pCurStreamDataPtr > pEnd)
    {
        mDetectInfo.pCurStreamDataPtr = pStart + (mDetectInfo.pCurStreamDataPtr - pEnd - 1);
    }
    mDetectInfo.pCurFramePic->nLength += nSkipSize;

}


void Sbm::storeNaluInfo(int nNaluType, int nNaluSize, char* pNaluBuf)
{
    qDebug("storeNaluInfo, nNaluType = %d, nNaluSize = %d", nNaluType, nNaluSize);
    int nNaluIdx = mDetectInfo.pCurFramePic->nCurNaluIdx;
    NaluInfo* pNaluInfo = &mDetectInfo.pCurFramePic->pNaluInfoList[nNaluIdx];
    pNaluInfo->nType = nNaluType;
    pNaluInfo->pDataBuf = pNaluBuf;
    pNaluInfo->nDataSize = nNaluSize;
    mDetectInfo.pCurFramePic->nCurNaluIdx++;
    if(mDetectInfo.pCurFramePic->nCurNaluIdx >= mDetectInfo.pCurFramePic->nMaxNaluNum)
    {
        expandNaluList(mDetectInfo.pCurFramePic);
    }

}
