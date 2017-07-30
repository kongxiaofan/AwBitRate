#ifndef AVSFRAME_H
#define AVSFRAME_H

class AvsFrame
{
public:
    AvsFrame();
    int isStartCode(int mStartCodeType);
    int searchAvsStartCode(unsigned char *pBuf, int length);
    int catOneAvsStreamFrame(unsigned char *pBuf, int length);

private:
    bool bCurFrameStartCodeFound;
    bool bHasSlice;
    int  offset;
    int  ncount;

private:
    void resetValue();
};

#endif // AVSFRAME_H
