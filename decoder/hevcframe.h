#ifndef HEVCFRAME_H
#define HEVCFRAME_H

class QFile;
class HevcFrame
{
public:
    HevcFrame(bool type, QFile *file);
    ~HevcFrame();
    int judgeOneFrame();

private:
    int searchNaluSize(char *pData, int nSize);
    bool judgeFirstNalu(char *pData);
    bool judgeFirstNalu(char *pData1, char *pData2, int x);
    int searchStartCode(char *pData);
    int judgeStreamPacketFrame();
    int judgeFramePacketFrame();


private:
    bool bStreameType;
    int offset;
    QFile *fp;
    int length;
    int curStreamData;
    int frameLength;
    char *pBuf ;
    char *pBuf2;
};

#endif // HEVCFRAME_H
