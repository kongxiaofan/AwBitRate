#ifndef SBM_H
#define SBM_H


typedef struct VIDEOSTREAMDATAINFO
{
    char*    pData;
    int      nLength;
    int      bIsFirstPart;
    int      bIsLastPart;
}VideoStreamDataInfo;


typedef struct STREAMFRAMEFIFO
{
    VideoStreamDataInfo* pFrames;
    int                  nMaxFrameNum;
    int                  nValidFrameNum;
    int                  nUnReadFrameNum;
    int                  nReadPos;
    int                  nWritePos;
    int                  nFlushPos;
}StreamFrameFifo;

typedef struct STREAMNALUINFO
{
    int   nType;
    char* pDataBuf;
    int   nDataSize;
}NaluInfo;

typedef struct FRAMEPICINFO
{
    NaluInfo* pNaluInfoList;
    int       nMaxNaluNum;
    int       nCurNaluIdx;
    int       nLength;
    char*     pDataStartAddr;
    int       nFrameNaluType;
}FramePicInfo;

typedef struct FRAMEPICFIFO
{
    FramePicInfo* pFramePics;
    int           nMaxFramePicNum;
    int           nValidFramePicNum;
    int           nUnReadFramePicNum;
    int           nFPReadPos;
    int           nFPWritePos;
    int           nFPFlushPos;
}FramePicFifo;

typedef struct DETECTEFRAMEPICINFO
{
    VideoStreamDataInfo* pCurStream;
    char*                pCurStreamDataPtr;
    int                  nCurStreamDataSize;
    bool                 nCurStreamRebackFlag;
    FramePicInfo*        pCurFramePic;
    bool                 bCurFrameStartCodeFound;
}DetectFramePicInfo;

enum SBM_THREAD_CMD
{
    SBM_THREAD_CMD_START = 0,
    SBM_THREAD_CMD_READ  = 1,
    SBM_THREAD_CMD_QUIT  = 2,
    SBM_THREAD_CMD_RESET = 3
};

class Sbm
{
public:
    Sbm();
    int init(int size);
    void destroy();
    void reset();
    int decideStreamBufferSize(int width, int heigth);
    void *getBuffAddr();
    int getBuffSize();
    int getStreamFrameNum();
    int getStreamDataSize();
    int requestStreamBuffer(int nRequireSize, char** ppBuf,
                                 int*          pBufSize,
                                 char**        ppRingBuf,
                                 int*          pRingBufSize);
    int addStream(VideoStreamDataInfo *pDataInfo);
    FramePicInfo* requestFramePic();
    int returnFramePic(FramePicInfo* pFramePic);
    int flushFramePic(FramePicInfo* pFramePic);
    char* getBufferWritePointer();
    //void* getBufferDataInfo();
    void setEos(bool bEos);
    bool isEos();
protected:
    bool            bEosFlag;
    bool            bStreamPacket;
    char*           pStreamBuffer;   //start
    char*           pStreamBufferEnd;
    int             nStreamBufferSize;
    char*           pWriteAddr;
    int             nValidDataSize;
    StreamFrameFifo frameFifo;

    FramePicFifo     mFramePicFifo;
    int              bStreamWithStartCode;
    DetectFramePicInfo mDetectInfo;
    SBM_THREAD_CMD    stateCmd;
protected:
    VideoStreamDataInfo *requestStream();
    int sbmRequestBuffer(int nRequireSize, char** ppBuf, int* pBufSize);
    int returnStream(VideoStreamDataInfo *pDataInfo);
    int flushStream(VideoStreamDataInfo *pDataInfo, bool bFlush);
    FramePicInfo* requestEmptyFramePicBuf();
    int addFramePic(FramePicInfo* pFramePic);
    void expandNaluList(FramePicInfo* pFramePic);
    void initFramePicInfo(DetectFramePicInfo* pDetectInfo);
    int supplyStreamData();
    void disposeInvalidStreamData();
    void skipCurStreamDataBytes(int nSkipSize);
    void storeNaluInfo(int nNaluType, int nNaluSize, char* pNaluBuf);
};

#endif // SBM_H
