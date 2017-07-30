#ifndef MPEG2FRAME_H
#define MPEG2FRAME_H

class Mpeg2Frame
{
public:
    Mpeg2Frame();
    int isStartCode(int mStartCodeType);
    int searchMpeg2StartCode(unsigned char *pBuf, int length);
    int catOneMpeg2StreamFrame(unsigned char *pBuf, int length);

private:
    bool bCurFrameStartCodeFound;
    bool bHasSlice;
    int  offset;
    int  ncount;

private:
    void resetValue();

};

#endif // MPEG2FRAME_H
