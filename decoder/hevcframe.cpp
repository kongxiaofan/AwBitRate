#include "hevcframe.h"
#include <QFile>

HevcFrame::HevcFrame(bool type, QFile *file)
{
    bStreameType = type;
    offset = 0;
    fp = file;
    curStreamData = 0;
    pBuf = (char *)malloc(1024);
    pBuf2 = (char *)malloc(1024);
}

HevcFrame::~HevcFrame()
{
    free(pBuf);
    free(pBuf2);
}

int HevcFrame::judgeOneFrame()
{
    if(bStreameType)
    {
        return judgeStreamPacketFrame();
    }
    else
    {
        return judgeFramePacketFrame();
    }

}

int HevcFrame::searchNaluSize(char *pData, int nSize)
{
    int i, nTemp, nNaluLen;
    int mask = 0xffffff;
    nTemp = -1;
    nNaluLen = -1;
    for(i = 0; i < nSize; i++)
    {
        nTemp = (nTemp << 8) | pData[i];
        if((nTemp & mask) == 0x000001)
        {
            nNaluLen = i - 3;
            break;
        }
    }
    return nNaluLen;
}

bool HevcFrame::judgeFirstNalu(char *pData)
{
    int nNaluType = (pData[0] & 0x7e) >> 1;
    if(((nNaluType >= 32) && (nNaluType <= 35)) || (nNaluType == 39))
    {
        return true;
    }
    if(nNaluType <= 21)
    {
        int bFirstSliceSegment = pData[2] >> 7;
        if(bFirstSliceSegment == 1)
        {
            return true;
        }
        else
            return false;
    }
    return false;
}

bool HevcFrame::judgeFirstNalu(char *pData1, char *pData2, int x)
{
    int nNaluType = (pData1[0] & 0x7e) >> 1;
    if(((nNaluType >= 32) && (nNaluType <= 35)) || (nNaluType == 39))
    {
        return true;
    }
    if(nNaluType <= 21)
    {
        int bFirstSliceSegment = pData2[x] >> 7;
        if(bFirstSliceSegment == 1)
        {
            return true;
        }
        else
            return false;
    }
    return false;
}

int HevcFrame::searchStartCode(char *pData)
{
    int i = 0;
    int nSize = curStreamData - 3;
    while(nSize > 0)
    {
        if(pData[i] == 0x00 && pData[i+1] == 0x00 && pData[i+2] == 0x01)
        {
            offset += 3;
            return 0;
        }
        offset++;
        i++;
        nSize--;
    }
    return -1;
}

int HevcFrame::judgeStreamPacketFrame()
{
    int nRet = 0;
    bool bCurFrameStartCodeFound = false;
    bool bFirstNalu =false;
    offset = 0;
    while(1)
    {
        if(curStreamData < 5)
        {
            length = fp->read(pBuf, 1024);
            if(length == -1)
            {
                if(fp->atEnd())
                {
                    bCurFrameStartCodeFound = false;
                    return frameLength;
                }
                else
                    continue;
            }
            frameLength += curStreamData;
            curStreamData = length;
        }
        pBuf = pBuf + offset;
        nRet = searchStartCode(pBuf);//searchFirstStartCode
        if(nRet != 0)//???????
        {
            frameLength += curStreamData;
            curStreamData = 0;
            continue;
        }
        else
        {
            curStreamData -= offset;
            frameLength += (offset - 3);
            //if(offset + 3 < curStreamData)
            if(curStreamData >= 3)
            {
                pBuf += offset;
                bFirstNalu = judgeFirstNalu(pBuf);
            }
            else
            {
                //length = fread(pBuf2, 1, 1024, rfp);
                length = fp->read(pBuf2, 1024);
                if(length == -1)
                {
                    if(fp->atEnd())
                    {
                        bCurFrameStartCodeFound = false;
                        return frameLength;
                    }
                    else
                        continue;
                }

                if(curStreamData == 2)
                {
                    bFirstNalu = judgeFirstNalu(pBuf, pBuf2, 0);
                    offset = 1;
                }
                else if(curStreamData == 1)
                {
                    bFirstNalu = judgeFirstNalu(pBuf, pBuf2, 1);
                    offset = 2;

                }
                else if(curStreamData < 1)
                {
                    bFirstNalu = judgeFirstNalu(pBuf2);
                    offset = 3;
                }
                memset(pBuf, 0,  1024);
                memcpy(pBuf, pBuf2, length);
                curStreamData = length - offset;
            }

            if(bFirstNalu)
            {
                if(bCurFrameStartCodeFound)
                    return frameLength;
                else
                    bCurFrameStartCodeFound = true;
            }
        }
    }

}

int HevcFrame::judgeFramePacketFrame()
{
    return false;
}
