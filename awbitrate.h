#ifndef AWBITRATE_H
#define AWBITRATE_H

#include <QDialog>
class QFile;
class QFileInfo;
class QTimer;
class DcrBSThread;

namespace Ui {
class AwBitRate;
}

class AwBitRate : public QDialog
{
    Q_OBJECT
    
public:
    explicit AwBitRate(QWidget *parent = 0);
    ~AwBitRate();
    //typedef enum{H265 = 1, H264, MPEG2, AVS}DECODER;
    
private slots:
    void on_openBtn_clicked();

    void on_startBtn_clicked();

    void udateProgress(qint32 totalSize);

    void showBitRat(qint32 second, qint32 bitRate);

    void on_exportBtn_clicked();

private:
    Ui::AwBitRate *ui;
    QFile *file;
    QFileInfo *info;
    QTimer *timer;
    qint32 nTotalSize;
    DcrBSThread *thrd;

private:


};

#endif // AWBITRATE_H
