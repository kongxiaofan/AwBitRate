#ifndef AWBITRATE_H
#define AWBITRATE_H

#include <QDialog>
class QFile;
class QFileInfo;
class QTimer;
class DcrBSThread;
class QCPBars;

namespace Ui {
class AwBitRate;
}

class AwBitRate : public QDialog
{
    Q_OBJECT
    
public:
    explicit AwBitRate(QWidget *parent = 0);
    ~AwBitRate();
    
private slots:
    void on_openBtn_clicked();

    void on_startBtn_clicked();

    void udateProgress(qint32 totalSize);

    void showBitRat(qint32 second, qint32 bitRate);

    void updatePor(qint32 second, qint32 bitRate);

    void on_exportBtn_clicked();

private:
    Ui::AwBitRate *ui;
    QFile *file;
    QFileInfo *info;
    QTimer *timer;
    qint32 nTotalSize;
    DcrBSThread *thrd;
    qint32 nOrBitRate;
    QVector<QString> proLabels;
    QVector<double> proValues;
    QCPBars* bars;

private:
    void initPor();
    void updatePor();

};

#endif // AWBITRATE_H
