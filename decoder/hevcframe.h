#ifndef HEVCFRAME_H
#define HEVCFRAME_H
#include <QThread>
#include "include/infor.h"
#include "sbm/sbmhevc.h"

class QFile;


class H265Submit : public QThread
{
public:
    H265Submit(QFile *fp, Decoder dec, SbmHevc *tSbm);
    void run();
private:
    SbmHevc *pSbm;
    QFile *file;
    Decoder decoder;
};

class H265Cosum : public QThread
{
    Q_OBJECT
public:
     H265Cosum(SbmHevc *tSbm, Decoder dec);
     void run();

signals:
    void sendInfo(int second, int bitRate);
private:
     SbmHevc *pSbm;
     Decoder  decoder;
};


class HevcFrame
{
public:
    HevcFrame(QFile *fp, Decoder dec);
    ~HevcFrame();
    H265Submit *sb;
    H265Cosum  *cs;
    void caculate();

private:
    SbmHevc *pSbm;
};

#endif // HEVCFRAME_H
