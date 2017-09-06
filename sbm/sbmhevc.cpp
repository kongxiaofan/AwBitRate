#include "sbmhevc.h"
#include <QMutex>

extern QMutex mutex;

typedef enum SbmHevcNaluType
{
    SBM_HEVC_NAL_TRAIL_N    = 0,
    SBM_HEVC_NAL_TRAIL_R    = 1,
    SBM_HEVC_NAL_TSA_N      = 2,
    SBM_HEVC_NAL_TSA_R      = 3,
    SBM_HEVC_NAL_STSA_N     = 4,
    SBM_HEVC_NAL_STSA_R     = 5,
    SBM_HEVC_NAL_RADL_N     = 6,
    SBM_HEVC_NAL_RADL_R     = 7,
    SBM_HEVC_NAL_RASL_N     = 8,
    SBM_HEVC_NAL_RASL_R     = 9,
    SBM_HEVC_NAL_BLA_W_LP   = 16,
    SBM_HEVC_NAL_BLA_W_RADL = 17,
    SBM_HEVC_NAL_BLA_N_LP   = 18,
    SBM_HEVC_NAL_IDR_W_RADL = 19,
    SBM_HEVC_NAL_IDR_N_LP   = 20,
    SBM_HEVC_NAL_CRA_NUT    = 21,
    SBM_HEVC_NAL_VPS        = 32,
    SBM_HEVC_NAL_SPS        = 33,
    SBM_HEVC_NAL_PPS        = 34,
    SBM_HEVC_NAL_AUD        = 35,
    SBM_HEVC_NAL_EOS_NUT    = 36,
    SBM_HEVC_NAL_EOB_NUT    = 37,
    SBM_HEVC_NAL_FD_NUT     = 38,
    SBM_HEVC_NAL_SEI_PREFIX = 39,
    SBM_HEVC_NAL_SEI_SUFFIX = 40,
    SBM_HEVC_UNSPEC63          = 63
}SbmHevcNaluType;

#define IsFrameNalu(eNaluType) (eNaluType <= SBM_HEVC_NAL_CRA_NUT)

#define SBM_FRAME_FIFO_SIZE (2048)  //* store 2048 frames of bitstream data at maximum.
#define MAX_FRAME_PIC_NUM (100)
#define DEFAULT_NALU_NUM (32)
#define MAX_INVALID_STREAM_DATA_SIZE (1*1024*1024) //* 1 MB
#define MAX_NALU_NUM_IN_FRAME (1024)

static inline char readByteIdx(char *p, char *pStart, char *pEnd, int i)
{
    char c = 0x0;
    if((p+i) <= pEnd)
        c = p[i];
    else
    {
        int d = (int)(pEnd - p) + 1;
        c = pStart[i - d];
    }
    return c;
}


static inline void ptrPlusOne(char **p, char *pStart, char *pEnd)
{
    if((*p) == pEnd)
        (*p) = pStart;
    else
        (*p) += 1;
}


SbmHevc::SbmHevc()
{
}

void SbmHevc::run()
{
    while(1)
    {
        if(stateCmd == SBM_THREAD_CMD_QUIT)
        {
            qDebug(" exit sbm thread ");
            return;
        }
        else if(stateCmd == SBM_THREAD_CMD_READ)
        {
            if(bStreamWithStartCode == -1)
            {
                checkBitStreamType();

            }
            else
            {
                if((bStreamWithStartCode == 0 && bStreamPacket) ||
                   (bStreamWithStartCode == 1 && !bStreamPacket))
                   {
                   }
                detectOneFramePic();
            }

        }
        else if(stateCmd == SBM_THREAD_CMD_RESET)
        {
            if(mDetectInfo.pCurStream)
            {
                flushStream(mDetectInfo.pCurStream);
                mDetectInfo.pCurStream = NULL;
            }

            mutex.lock();
            stateCmd = SBM_THREAD_CMD_READ;
            mDetectInfo.bCurFrameStartCodeFound = 0;
            mDetectInfo.nCurStreamDataSize = 0;
            mDetectInfo.nCurStreamRebackFlag = 0;
            mDetectInfo.pCurStreamDataPtr = NULL;

            if(mDetectInfo.pCurFramePic)
            {
               mDetectInfo.pCurFramePic = NULL;
            }

            pWriteAddr                 = pStreamBuffer;
            nValidDataSize             = 0;

            frameFifo.nReadPos         = 0;
            frameFifo.nWritePos        = 0;
            frameFifo.nFlushPos        = 0;
            frameFifo.nValidFrameNum   = 0;
            frameFifo.nUnReadFrameNum  = 0;

            mFramePicFifo.nFPFlushPos = 0;
            mFramePicFifo.nFPReadPos  = 0;
            mFramePicFifo.nFPWritePos = 0;
            mFramePicFifo.nUnReadFramePicNum = 0;
            mFramePicFifo.nValidFramePicNum = 0;

            mutex.unlock();
        }
    }
}

int SbmHevc::checkBitStreamTypeWithStartCode(VideoStreamDataInfo *pStream)
{
    char *pBuf = NULL;
    char tmpBuf[6] = {0};
    const int nTsStreamType       = 0x000001;
    const int nForbiddenBitValue  = 0;
    const int nTemporalIdMinValue = 1;
    char* pStart = pStreamBuffer;
    char* pEnd   = pStreamBufferEnd;
    int nHadCheckBytesLen = 0;
    int nCheck4BitsValue = -1;
    int nTemporalId      = -1;
    int nForbiddenBit    = -1;

    //*1. process sbm-cycle-buffer case
    pBuf = pStream->pData;

    while((nHadCheckBytesLen + 6) < pStream->nLength)
    {

        tmpBuf[0] = readByteIdx(pBuf, pStart, pEnd, nHadCheckBytesLen + 0);
        tmpBuf[1] = readByteIdx(pBuf, pStart, pEnd, nHadCheckBytesLen + 1);
        tmpBuf[2] = readByteIdx(pBuf, pStart, pEnd, nHadCheckBytesLen + 2);
        tmpBuf[3] = readByteIdx(pBuf, pStart, pEnd, nHadCheckBytesLen + 3);
        tmpBuf[4] = readByteIdx(pBuf, pStart, pEnd, nHadCheckBytesLen + 4);
        tmpBuf[5] = readByteIdx(pBuf, pStart, pEnd, nHadCheckBytesLen + 5);

        nCheck4BitsValue = (tmpBuf[0] << 24) | (tmpBuf[1] << 16) | (tmpBuf[2] << 8) | tmpBuf[3];
        if(nCheck4BitsValue == 0) //*compatible for the case: 00 00 00 00 00 00 00 01
        {
            nHadCheckBytesLen++;
            continue;
        }

        if(nCheck4BitsValue == nTsStreamType)
        {
            nForbiddenBit = tmpBuf[4] >> 7; //* read 1 bits
            nTemporalId   = tmpBuf[5] & 0x7;//* read 3 bits
            if(nTemporalId >= nTemporalIdMinValue && nForbiddenBit == nForbiddenBitValue)
            {
                bStreamWithStartCode = 1;
                return 0;
            }
            else
            {
                nHadCheckBytesLen += 4;
                continue;
            }
        }
        else if((nCheck4BitsValue >> 8) == nTsStreamType)
        {
            nForbiddenBit = tmpBuf[3] >> 7; //* read 1 bits
            nTemporalId   = tmpBuf[4] & 0x7;//* read 3 bits
            if(nTemporalId >= nTemporalIdMinValue && nForbiddenBit == nForbiddenBitValue)
            {
                bStreamWithStartCode = 1;
                return 0;
            }
            else
            {
                nHadCheckBytesLen += 3;
                continue;
            }

        }
        else
        {
            nHadCheckBytesLen += 4;
            continue;
        }
    }

    return -1;
}

int SbmHevc::checkBitStreamTypeWithoutStartCode(VideoStreamDataInfo *pStream)
{
    const int nForbiddenBitValue  = 0;
    const int nTemporalIdMinValue = 1;
    char *pBuf = NULL;
    char tmpBuf[6] = {0};
    int nTemporalId      = -1;
    int nForbiddenBit    = -1;
    int nDataSize   = -1;
    int nRemainSize = -1;
    int nRet = -1;
    char* pStart = pStreamBuffer;
    char* pEnd   = pStreamBufferEnd;

    int nHadProcessLen = 0;
    pBuf = pStream->pData;
    while(nHadProcessLen < pStream->nLength)
    {
        nRemainSize = pStream->nLength-nHadProcessLen;
        tmpBuf[0] = readByteIdx(pBuf, pStart, pEnd, 0);
        tmpBuf[1] = readByteIdx(pBuf, pStart, pEnd, 1);
        tmpBuf[2] = readByteIdx(pBuf, pStart, pEnd, 2);
        tmpBuf[3] = readByteIdx(pBuf, pStart, pEnd, 3);
        tmpBuf[4] = readByteIdx(pBuf, pStart, pEnd, 4);
        tmpBuf[5] = readByteIdx(pBuf, pStart, pEnd, 5);
        nDataSize = (tmpBuf[0] << 24) | (tmpBuf[1] << 16) | (tmpBuf[2] << 8) | tmpBuf[3];
        nForbiddenBit = tmpBuf[4] >> 7; //* read 1 bits
        nTemporalId   = tmpBuf[5] & 0x7;//* read 3 bits
        if(nDataSize > (nRemainSize - 4)
           || nDataSize < 0
           || nTemporalId < nTemporalIdMinValue
           || nForbiddenBit != nForbiddenBitValue)
        {
            qDebug("check stream type fail: nDataSize[%d], streamSize[%d], nTempId[%d], fobBit[%d]",
                 nDataSize, (pStream->nLength-nHadProcessLen),nTemporalId,nForbiddenBit);
            nRet = -1;
            break;
        }
        qDebug("*** nDataSize = %d, nRemainSize = %d, proceLen = %d, totalLen = %d",
            nDataSize, nRemainSize,
            nHadProcessLen,pStream->nLength);

        if(nDataSize == (nRemainSize - 4) && nDataSize != 0)
        {
            nRet = 0;
            break;
        }

        nHadProcessLen += nDataSize + 4;
        pBuf = pStream->pData + nHadProcessLen;
        if(pBuf - pStreamBufferEnd > 0)
        {
            pBuf = pStreamBuffer + (pBuf - pStreamBufferEnd);
        }
    }

    return nRet;
}

int SbmHevc::checkBitStreamType()
{
    const int nUpLimitCount       = 50;
    int nReqeustCounter  = 0;
    int nRet = -1;
    int bStartCode_with = 0;
    int bStartCode_without = 0;

    while(nReqeustCounter < nUpLimitCount)
    {
        VideoStreamDataInfo *pStream = NULL;
        nReqeustCounter++;
        pStream = requestStream();
        if(pStream == NULL)
        {
            nRet = -1;
            break;
        }
        if(pStream->nLength == 0 || pStream->pData == NULL)
        {
            flushStream( pStream);
            pStream = NULL;
            continue;
        }

        if(checkBitStreamTypeWithStartCode( pStream) == 0)
        {
            bStartCode_with = 1;
        }
        else
        {
            bStartCode_with = 0;
        }

        if(checkBitStreamTypeWithoutStartCode( pStream) == 0)
        {
            bStartCode_without = 1;
        }
        else
        {
            bStartCode_without = 0;
        }

        if(bStartCode_with == 1 && bStartCode_without == 1)
        {
            bStreamWithStartCode = 0;
        }
        else if(bStartCode_with == 1 && bStartCode_without == 0)
        {
            bStreamWithStartCode = 1;
        }
        else if(bStartCode_with == 0 && bStartCode_without == 1)
        {
            bStreamWithStartCode = 0;
        }
        else
        {
           bStreamWithStartCode = -1;
        }

        qDebug("result: bStreamWithStartCode[%d], with[%d], whitout[%d]",bStreamWithStartCode,
              bStartCode_with, bStartCode_without);

        //*continue reqeust stream from sbm when if judge the stream type
        if(bStreamWithStartCode == -1)
        {
            flushStream( pStream);
            continue;
        }
        else
        {
            //* judge stream type successfully, return.
            returnStream( pStream);
            nRet = 0;
            break;
        }
    }

    return nRet;
}

int SbmHevc::searchStartCode(int *pAfterStartCodeIdx)
{
    char* pStart = pStreamBuffer;
    char* pEnd   = pStreamBufferEnd;

    DetectFramePicInfo* pDetectInfo = &mDetectInfo;


    char* pBuf = pDetectInfo->pCurStreamDataPtr;
    int nSize = pDetectInfo->nCurStreamDataSize - 3;

    if(pDetectInfo->nCurStreamRebackFlag)
    {
        qDebug("bHasTwoDataTrunk Buf: %p, BufEnd: %p, curr: %p, diff: %d ",
                pStart, pEnd, pBuf, (unsigned)(pEnd - pBuf));
        char tmpBuf[3];
        while(nSize > 0)
        {
            tmpBuf[0] = readByteIdx(pBuf , pStart, pEnd, 0);
            tmpBuf[1] = readByteIdx(pBuf , pStart, pEnd, 1);
            tmpBuf[2] = readByteIdx(pBuf , pStart, pEnd, 2);
            if(tmpBuf[0] == 0 && tmpBuf[1] == 0 && tmpBuf[2] == 1)
            {
                (*pAfterStartCodeIdx) += 3; //so that buf[0] is the actual data, not start code
                return 0;
            }
            ptrPlusOne(&pBuf, pStart, pEnd);
            ++(*pAfterStartCodeIdx);
            --nSize;
        }
    }
    else
    {
        while(nSize > 0)
        {
            if(pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 1)
            {
                (*pAfterStartCodeIdx) += 3; //so that buf[0] is the actual data, not start code
                return 0;
            }
            ++pBuf;
            ++(*pAfterStartCodeIdx);
            --nSize;
        }
    }
    return -1;
}

void SbmHevc::detectWithStartCode()
{
    char tmpBuf[6] = {0};
    char* pStart = pStreamBuffer;
    char* pEnd   = pStreamBufferEnd;
    int   bFirstSliceSegment = 0;

    if(mDetectInfo.pCurFramePic == NULL)
    {
       mDetectInfo.pCurFramePic = requestEmptyFramePicBuf();
       if(mDetectInfo.pCurFramePic == NULL)
       {
            return ;
       }
       initFramePicInfo(&mDetectInfo);
    }

    while(1)
    {

        //*1. request bit stream
        if(mDetectInfo.nCurStreamDataSize < 5 || mDetectInfo.pCurStreamDataPtr == NULL)
        {
            if(supplyStreamData() != 0)
            {
                if(mDetectInfo.bCurFrameStartCodeFound == 1 && bEosFlag == true)
                {
                    mDetectInfo.bCurFrameStartCodeFound = 0;
                    addFramePic(mDetectInfo.pCurFramePic);
                    mDetectInfo.pCurFramePic = NULL;
                }
                return ;
            }
        }

        if(mDetectInfo.pCurFramePic->pDataStartAddr == NULL)
        {
           mDetectInfo.pCurFramePic->pDataStartAddr = mDetectInfo.pCurStreamDataPtr;
        }

        //*2. find startCode
        int nAfterStartCodeIdx = 0;
        int nRet = searchStartCode(&nAfterStartCodeIdx);
        if(nRet != 0 //*  can not find startCode
           || mDetectInfo.pCurFramePic->nCurNaluIdx > MAX_NALU_NUM_IN_FRAME)
        {
            qDebug("can not find startCode, curNaluIdx = %d, max = %d",
                  mDetectInfo.pCurFramePic->nCurNaluIdx, MAX_NALU_NUM_IN_FRAME);
            disposeInvalidStreamData();
            return ;
        }

        //* now had find the startCode
        //*3.  read the naluType and bFirstSliceSegment
        char* pAfterStartCodeBuf = mDetectInfo.pCurStreamDataPtr + nAfterStartCodeIdx;
        tmpBuf[0] = readByteIdx(pAfterStartCodeBuf ,pStart, pEnd, 0);
        int nNaluType = (tmpBuf[0] & 0x7e) >> 1;

        qDebug("*** nNaluType = %d",nNaluType);
        if((nNaluType >= SBM_HEVC_NAL_VPS && nNaluType <= SBM_HEVC_NAL_AUD) ||
            nNaluType == SBM_HEVC_NAL_SEI_PREFIX)
        {
            /* Begining of access unit, needn't bFirstSliceSegment */
            if(mDetectInfo.bCurFrameStartCodeFound == 1)
            {
                mDetectInfo.bCurFrameStartCodeFound = 0;
                addFramePic(mDetectInfo.pCurFramePic);
                mDetectInfo.pCurFramePic = NULL;
                return ;
            }
        }

        if(IsFrameNalu(nNaluType))
        {
            tmpBuf[2] = readByteIdx(pAfterStartCodeBuf ,pStart, pEnd, 2);
            bFirstSliceSegment = (tmpBuf[2] >> 7);
            qDebug("***bFirstSliceSegment = %d", bFirstSliceSegment);
            if(bFirstSliceSegment == 1)
            {
                if(mDetectInfo.bCurFrameStartCodeFound == 0)
                {
                    qDebug("pCurFramePic = %p, pCurStream = %p",
                         mDetectInfo.pCurFramePic, mDetectInfo.pCurStream);
                    mDetectInfo.bCurFrameStartCodeFound = 1;
                    mDetectInfo.pCurFramePic->nFrameNaluType = nNaluType;
                }
                else
                {
                    qDebug("**** have found one frame pic ****");
                    mDetectInfo.bCurFrameStartCodeFound = 0;
                    addFramePic(mDetectInfo.pCurFramePic);
                    mDetectInfo.pCurFramePic = NULL;
                    return ;
                }
            }
        }

        //*if code run here, it means that this is the normal nalu of new frame, we should store it
        //*4. skip nAfterStartCodeIdx
        skipCurStreamDataBytes(nAfterStartCodeIdx);

        //*5. find the next startCode to determine size of cur nalu
        int nNaluSize = 0;
        nAfterStartCodeIdx = 0;
        nRet = searchStartCode(&nAfterStartCodeIdx);
        if(nRet != 0)//* can not find next startCode
        {
            nNaluSize = mDetectInfo.nCurStreamDataSize;
        }
        else
        {
            nNaluSize = nAfterStartCodeIdx - 3; //* 3 is the length of startCode
        }

        //*6. store  nalu info
        storeNaluInfo(nNaluType, nNaluSize, mDetectInfo.pCurStreamDataPtr);

        //*7. skip naluSize bytes
        skipCurStreamDataBytes(nNaluSize);
    }

    return ;
}

void SbmHevc::detectWithoutStartCode()
{
    char tmpBuf[6] = {0};
    char* pStart = pStreamBuffer;
    char* pEnd   = pStreamBufferEnd;
    unsigned int bFirstSliceSegment=0;
    const unsigned int nPrefixBytes = 4;
    //int i = 0;

    if(mDetectInfo.pCurFramePic == NULL)
    {
       mDetectInfo.pCurFramePic = requestEmptyFramePicBuf();
       if(mDetectInfo.pCurFramePic == NULL)
       {
            return ;
       }
       initFramePicInfo(&mDetectInfo);
    }

    while(1)
    {
        //*1. request bit stream
        if(mDetectInfo.nCurStreamDataSize < 5 || mDetectInfo.pCurStreamDataPtr == NULL)
        {
            if(supplyStreamData() != 0)
            {
                if(mDetectInfo.bCurFrameStartCodeFound == 1 && bEosFlag == true)
                {
                    mDetectInfo.bCurFrameStartCodeFound = 0;
                    addFramePic(mDetectInfo.pCurFramePic);
                    mDetectInfo.pCurFramePic = NULL;
                }
                return ;
            }
        }

        if(mDetectInfo.pCurFramePic->pDataStartAddr == NULL)
        {
           mDetectInfo.pCurFramePic->pDataStartAddr = mDetectInfo.pCurStreamDataPtr;
        }

        //*2. read nalu size
        tmpBuf[0] = readByteIdx(mDetectInfo.pCurStreamDataPtr ,pStart,pEnd, 0);
        tmpBuf[1] = readByteIdx(mDetectInfo.pCurStreamDataPtr ,pStart,pEnd, 1);
        tmpBuf[2] = readByteIdx(mDetectInfo.pCurStreamDataPtr ,pStart,pEnd, 2);
        tmpBuf[3] = readByteIdx(mDetectInfo.pCurStreamDataPtr ,pStart,pEnd, 3);
        unsigned int nNaluSize = 0;
        nNaluSize = (tmpBuf[0] << 24) | (tmpBuf[1] << 16) | (tmpBuf[2] << 8) | tmpBuf[3];
        qDebug("*** read nalu size = %u, ",nNaluSize);
        if(nNaluSize > (mDetectInfo.nCurStreamDataSize - nPrefixBytes)
           || nNaluSize == 0
           || mDetectInfo.pCurFramePic->nCurNaluIdx > MAX_NALU_NUM_IN_FRAME)
        {
            qDebug(" error: nNaluSize[%u] > nCurStreamDataSize[%d], curNaluIdx = %d, max = %d",
                   nNaluSize, mDetectInfo.nCurStreamDataSize,
                   mDetectInfo.pCurFramePic->nCurNaluIdx, MAX_NALU_NUM_IN_FRAME);
            disposeInvalidStreamData();
            return ;
        }

        //*3. read the naluType and bFirstSliceSegment
        char* pAfterStartCodePtr = NULL;
        unsigned int nNaluType=0;
        pAfterStartCodePtr = mDetectInfo.pCurStreamDataPtr + nPrefixBytes;
        tmpBuf[0] = readByteIdx(pAfterStartCodePtr ,pStart, pEnd, 0);
        nNaluType = (tmpBuf[0] & 0x7e) >> 1;
        qDebug("*** nNaluType = %d",nNaluType);
        if((nNaluType >= SBM_HEVC_NAL_VPS && nNaluType <= SBM_HEVC_NAL_AUD) ||
            nNaluType == SBM_HEVC_NAL_SEI_PREFIX)
        {
            /* Begining of access unit, needn't bFirstSliceSegment */
            if(mDetectInfo.bCurFrameStartCodeFound == 1)
            {
                mDetectInfo.bCurFrameStartCodeFound = 0;
                addFramePic( mDetectInfo.pCurFramePic);
                mDetectInfo.pCurFramePic = NULL;
                return ;
            }
        }

        if(IsFrameNalu(nNaluType))
        {
            tmpBuf[2] = readByteIdx(pAfterStartCodePtr ,pStart, pEnd, 2);
            bFirstSliceSegment = (tmpBuf[2] >> 7);
            qDebug("***bFirstSliceSegment = %d", bFirstSliceSegment);

            if(bFirstSliceSegment == 1)
            {
                if(mDetectInfo.bCurFrameStartCodeFound == 0)
                {
                    mDetectInfo.bCurFrameStartCodeFound = 1;
                    mDetectInfo.pCurFramePic->nFrameNaluType = nNaluType;
                }
                else
                {
                    qDebug("**** have found one frame pic ****");
                    mDetectInfo.bCurFrameStartCodeFound = 0;
                    addFramePic( mDetectInfo.pCurFramePic);
                    mDetectInfo.pCurFramePic = NULL;
                    return ;
                }
            }
        }

        //*4. skip 4 bytes
        skipCurStreamDataBytes(nPrefixBytes);

        //*5. store  nalu info
        storeNaluInfo(nNaluType, nNaluSize, mDetectInfo.pCurStreamDataPtr);

        //*6. skip naluSize bytes
        skipCurStreamDataBytes(nNaluSize);
    }
    return ;
}

void SbmHevc::detectOneFramePic()
{
    qDebug("bStreamWithStartCode = %d",bStreamWithStartCode);
    if(bStreamWithStartCode == 1)
    {
        detectWithStartCode();
    }
    else
    {
        detectWithoutStartCode();
    }
}


