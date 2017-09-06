#include "avsframe.h"


#define AVS_VDEC_SEQ_START_CODE 		  0x000001B0
#define AVS_VDEC_SEQ_END_CODE			  0x000001B1
#define AVS_VDEC_USER_START_CODE		   0x000001B2
#define AVS_VDEC_IPIC_START_CODE		   0x000001B3
#define AVS_VDEC_PBPIC_START_CODE		   0x000001B6
#define AVS_VDEC_EXTENSION_START_CODE	 0x000001B5
#define AVS_VDEC_SLICE_START_CODE		 0x00000100
#define AVS_VDEC_EDIT_CODE				  0x000001B7

#define IS_FRAME_START(mStartCodeType)        \
(mStartCodeType == AVS_VDEC_SEQ_START_CODE || \
 mStartCodeType == AVS_VDEC_EDIT_CODE ||  \
 mStartCodeType == AVS_VDEC_IPIC_START_CODE || \
 mStartCodeType == AVS_VDEC_PBPIC_START_CODE || \
 mStartCodeType == AVS_VDEC_SEQ_END_CODE)

AvsFrame::AvsFrame()
{
    offset = 0;
    resetValue();

}

int AvsFrame::isStartCode(int mStartCodeType)
{
    int flag1 = (mStartCodeType == AVS_VDEC_SEQ_START_CODE      ||
                 mStartCodeType == AVS_VDEC_EDIT_CODE           ||
                 mStartCodeType == AVS_VDEC_IPIC_START_CODE     ||
                 mStartCodeType == AVS_VDEC_PBPIC_START_CODE    ||
                 mStartCodeType == AVS_VDEC_SEQ_END_CODE);

    int flag2 = (mStartCodeType == AVS_VDEC_EXTENSION_START_CODE ||
                 mStartCodeType == AVS_VDEC_SLICE_START_CODE     ||
                 mStartCodeType == AVS_VDEC_USER_START_CODE);

    return ((flag1 || flag2)?1:0);
}

int AvsFrame::searchAvsStartCode(unsigned char *pBuf, int length)
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
                return mStartCodeType;
            }
        }
        ++pBuf;
        ++offset;
        --nSize;
    }
    return -1;
}

int AvsFrame::catOneAvsStreamFrame(unsigned char *pBuf, int length)
{
    int mStartCodeType;
    int retSize = 0;
    while(retSize < length)
    {
        mStartCodeType = searchAvsStartCode(pBuf+offset, length-offset);
        if(mStartCodeType == -1)
        {
            return -1;
        }
        if(IS_FRAME_START(mStartCodeType))
        {
            if(bCurFrameStartCodeFound)
            {
                retSize = offset - 4;
                offset = 0;
                return retSize;
            }
        }
        if(bCurFrameStartCodeFound == false)
            bCurFrameStartCodeFound = true;
        retSize = offset;
    }
    return 0;
}

void AvsFrame::resetValue()
{
    bCurFrameStartCodeFound = 0;
    bHasSlice = 0;
    offset = 0;
    ncount = 0;

}
