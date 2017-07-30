#include "mpeg2frame.h"
#include <QDebug>

#define MP2VDEC_SEQ_START_CODE   0x000001B3
#define MP2VDEC_GOP_START_CODE   0x000001B8
#define MP2VDEC_PIC_START_CODE   0x00000100
#define MP2VDEC_USER_START_CODE  0x000001B2
#define MP2VDEC_EXT_START_CODE   0x000001B5
#define MP2VDEC_SLICE_START_CODE 0x00000101

#define IS_FRAME_START(mStartCodeType)        \
(mStartCodeType == MP2VDEC_SEQ_START_CODE || \
 mStartCodeType == MP2VDEC_GOP_START_CODE ||  \
 mStartCodeType == MP2VDEC_PIC_START_CODE)

Mpeg2Frame::Mpeg2Frame()
{
    offset = 0;
    resetValue();
}

int Mpeg2Frame::isStartCode(int mStartCodeType)
{
    int flag1 = (mStartCodeType==MP2VDEC_SEQ_START_CODE)||
                (mStartCodeType==MP2VDEC_GOP_START_CODE)||
                (mStartCodeType==MP2VDEC_PIC_START_CODE);

    int flag2 = (mStartCodeType==MP2VDEC_USER_START_CODE)||
                (mStartCodeType==MP2VDEC_EXT_START_CODE)||
                (mStartCodeType==MP2VDEC_SLICE_START_CODE);
    return ((flag1 || flag2)?1:0);
}

int Mpeg2Frame::searchMpeg2StartCode(unsigned char *pBuf, int length)
{
    int mStartCodeType;
    int nSize = length - 4;
    while(nSize > 0)
    {
        if(pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 1)
        {
            mStartCodeType = 0x00000100 + pBuf[3];
            if(isStartCode(mStartCodeType))
            {
                offset += 4;
                //qDebug("start code is %x, offset = %d", mStartCodeType, offset);
                if(mStartCodeType == MP2VDEC_SLICE_START_CODE)
                    bHasSlice = 1;
                return mStartCodeType;
            }
        }
        ++pBuf;
        ++offset;
        --nSize;
    }
    return -1;
}

int Mpeg2Frame::catOneMpeg2StreamFrame(unsigned char *pBuf, int length)
{
    int mStartCodeType;
    int retSize = 0;
    //qDebug("here while, offset = %d, length = %d", offset, length);
   // qDebug("pBuf[0]:%x, pBuf[1]:%x, pBuf[2]:%x, pBuf[3]:%x, pBuf[4]:%x", pBuf[0],pBuf[1],pBuf[2],pBuf[3],pBuf[4]);

    if(!bCurFrameStartCodeFound)
    {
        mStartCodeType = searchMpeg2StartCode(pBuf, length);
       // qDebug("111return mstartCode： %x", mStartCodeType);
        if(IS_FRAME_START(mStartCodeType))
        {
           // qDebug("find the first start code: %x", mStartCodeType);
            //qDebug("1111 offset = %d", offset);
            bCurFrameStartCodeFound = 1;
        }
        else
        {
           // qDebug("can not find the first start code");
            offset = 0;
           // qDebug("return -1  11111");
            return -1;
        }
        while(offset <= length)
        {
            mStartCodeType = searchMpeg2StartCode(pBuf+offset, length-offset);
           // qDebug("222return mstartCode： %x", mStartCodeType);

            if(IS_FRAME_START(mStartCodeType))
            {
                if(bHasSlice)
                {
                    //qDebug("2222 offset = %d", offset);
                    retSize = offset - 4;
                    resetValue();
                    return retSize;
                }

            }
            else if(mStartCodeType == -1)//* can not find next startCode
            {
                offset = 0;
               // qDebug("return -1  22222");

                return -1;
            }
        }
    }
    else
    {
        while(offset <= length)
        {
            mStartCodeType = searchMpeg2StartCode(pBuf + offset, length - offset);
           // qDebug("333return mstartCode： %x", mStartCodeType);

            if(IS_FRAME_START(mStartCodeType))
            {
                if(bHasSlice)
                {
                   // qDebug("3333 offset = %d", offset);

                    retSize = offset - 4;
                    resetValue();
                    return retSize;
                }
            }
            else if(mStartCodeType == -1)//* can not find next startCode
            {
                offset = 0;
               // qDebug("return -1  333333");
#if 0
                ncount++;
                if(ncount == 10)
                {
                    qDebug("ncount = 10");
                    exit(1);
                }
#endif
                return -1;
            }
        }
    }
    qDebug("return -1  444444");

    return -1;
}



void Mpeg2Frame::resetValue()
{
    bCurFrameStartCodeFound = 0;
    bHasSlice = 0;
    offset = 0;
    ncount = 0;
}
