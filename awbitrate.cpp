#include "awbitrate.h"
#include "ui_awbitrate.h"
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include "dcrbsthread.h"
#include "include/infor.h"
#include "qcustomplot/qcustomplot.h"
#include <QCoreApplication>
#include <QVariant>
#include <QAxBase>
#include <QAxObject>
#include <QDesktopServices>

#define  BUFF_SIZE 1024*1024
#define SIZE 15

AwBitRate::AwBitRate(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AwBitRate)
{
    ui->setupUi(this);
    ui->noEditLB->setText("");
    timer = new QTimer;
    timer->setInterval(100);
    nOrBitRate = 1000;//设置默认阈值码率，单位bps
    initPor();
    connect(ui->startBtn, SIGNAL(clicked()), timer, SLOT(start()));

}

AwBitRate::~AwBitRate()
{
    delete ui;
}

void AwBitRate::on_openBtn_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "请选择一个裸码流文件", "./", "file (*.dat *.yuv *.265 *h265 *264 *h264);;所有文件 (*.*)");
    if(!fileName.isEmpty())
    {
        ui->filePathLE->setText(fileName);
        file = new QFile(fileName);
        if(file->open(QIODevice::ReadOnly))
        {
            info = new QFileInfo(fileName);
            if(info->size() == 0)
            {
                QMessageBox::warning(this, "Warning", "Maybe this is an empty file!\n Please choose another file");
                return;
            }
            else
            {
                QString size = QString::number(info->size());
                ui->fileSizeLB->setText(size+" Bytes");
                ui->progressBar->setMaximum(info->size());
                ui->progressBar->setValue(0);
            }
        }
        else
        {
            QMessageBox::warning(this, "warning", "不能打开此文件");
            return;
        }
    }
}

void AwBitRate::on_startBtn_clicked()
{
    int decoder;
    qint16 nFrameRate;
    bool isStreamType;
    if(!ui->frameRateLE->text().isEmpty())
    {
       nFrameRate = ui->frameRateLE->text().toInt();
    }
    else
    {
        ui->noEditLB->setText("!");
        return;
    }
    if(!ui->decoderCB->currentText().isEmpty())
    {
        decoder = ui->decoderCB->currentIndex();
        qDebug("decoder = %d", decoder);
    }
    else
        return;
    if(ui->streamBtn->isChecked())
        isStreamType = 1;
    else
        isStreamType = 0;

    ui->startBtn->setEnabled(false);
    ui->exportBtn->setEnabled(true);
    switch(decoder)
    {
    case H265:
        qDebug("this is h265");
        break;
    case H264:
        qDebug("this is h264");
        break;
    case MPEG2:
        qDebug("this is mpeg2");
        if(!isStreamType)
        {
            QMessageBox::warning(this, "warning", "MPEG2不支持帧封装格式！");
            return;
        }
        break;
    case AVS:
        qDebug("this is avs");
        if(!isStreamType)
        {
            QMessageBox::warning(this, "warning", "AVS不支持帧封装格式！");
            return;
        }
        break;
    default:
        QMessageBox::warning(this, "warning", "请选择一种解码器！");
        return;
    }

    thrd  = new DcrBSThread(file, nFrameRate, isStreamType, (DECODER)decoder);
    connect(thrd, SIGNAL(sendTotalReadBits(qint32)), SLOT(udateProgress(qint32)));
    connect(thrd, SIGNAL(sendInfo(qint32,qint32)), SLOT(showBitRat(qint32,qint32)));
    connect(thrd, SIGNAL(sendInfo(qint32,qint32)), SLOT(updatePor(qint32,qint32)));
    thrd->start();
}


void AwBitRate::udateProgress(qint32 totalSize)
{
    ui->progressBar->setValue(totalSize);
}

void AwBitRate::showBitRat(qint32 second, qint32 bitRate)
{
    ui->tableWidget->insertRow(second-1);
    QTableWidgetItem *sec = new QTableWidgetItem(QString::number(second));
    ui->tableWidget->setItem(second-1, 0, sec);

    QTableWidgetItem *br = new QTableWidgetItem(QString::number(bitRate));
    ui->tableWidget->setItem(second-1, 1, br);

    QTableWidgetItem *diff = new QTableWidgetItem(QString::number(bitRate - nOrBitRate));
    ui->tableWidget->setItem(second-1, 2, diff);
}

void AwBitRate::updatePor(qint32 second, qint32 bitRate)
{
    int i;
    for(i=0; i<SIZE-1; i++)
    {
        proLabels.replace(i,proLabels.at(i+1));
        proValues.replace(i, proValues.at(i+1));
    }
    proLabels.insert(i, QString("%1").arg(second));
    proValues.insert(i, bitRate);
    QVector<double> index(SIZE);

    for(int i=0; i<SIZE; ++i)
        index[i] = i;
    bars->setData(index,proValues);
    ui->proWidget->xAxis->setAutoTicks(false);
    ui->proWidget->xAxis->setAutoTickLabels(false);
    ui->proWidget->xAxis->setAutoTickStep(false);

    ui->proWidget->addPlottable(bars);
    ui->proWidget->rescaleAxes();
    double wid = ui->proWidget->xAxis->range().upper - ui->proWidget->xAxis->range().lower;
    double cl  = bars->width() + (1.0*wid - bars->width()*SIZE) / (SIZE - 1);

    QVector<double> coor;
    for(int i=0; i<SIZE; ++i)
        coor.append(ui->proWidget->xAxis->range().lower + i*cl+bars->width()/2);

    ui->proWidget->xAxis->setTickVector(coor);

    ui->proWidget->xAxis->setTickVectorLabels(proLabels);

    ui->proWidget->replot();
}


void AwBitRate::on_exportBtn_clicked()
{
    QString filepath = QFileDialog::getSaveFileName(this, tr("Save as..."),
                       QString(), tr("EXCEL files (*.xls *.xlsx)"));
    if(!filepath.isEmpty()){
        QAxObject *excel = new QAxObject(this);
        excel->setControl("Excel.Application");
        excel->dynamicCall("SetVisible(bool Visible)", false);
        excel->setProperty("DisplayAlerts", false);
        QAxObject *workbooks = excel->querySubObject("WorkBooks");
        workbooks->dynamicCall("Add");
        QAxObject *workbook = excel->querySubObject("ActiveWorkBook");
        QAxObject *worksheets = workbook->querySubObject("Sheets");
        QAxObject *worksheet = worksheets->querySubObject("Item(int)", 1);
        int rowCount = ui->tableWidget->rowCount();
        int columnCount = ui->tableWidget->columnCount();
        for(int i = 1; i < rowCount + 1; ++i){
            for(int j = 1; j < columnCount + 1; ++j){
                QAxObject *Range = worksheet->querySubObject("Cells(int,int)", i, j);
                Range->dynamicCall("SetValue(const QString &)", ui->tableWidget->item(i-1, j-1)->text());
            }
        }
        workbook->dynamicCall("SaveAs(const QString &)", QDir::toNativeSeparators(filepath));
        if(excel != NULL){
            excel->dynamicCall("Quit()");
            delete excel;
            excel = NULL;
        }
        QMessageBox::information(this,"提示", "Exporting data successful");
    }
}

void AwBitRate::initPor()
{
    proLabels.resize(SIZE);
    proValues.resize(SIZE);
    bars=new QCPBars(ui->proWidget->xAxis,ui->proWidget->yAxis);
    ui->proWidget->xAxis->setAutoTicks(false);
    ui->proWidget->xAxis->setAutoTickLabels(false);
    ui->proWidget->xAxis->setAutoTickStep(false);
    ui->proWidget->replot();
}

void AwBitRate::updatePor()
{
    QVector<QString> labels(SIZE);
    QVector<double> values(SIZE);
    for(int i=0; i<SIZE; ++i)
    {
        labels[i] = QString("%1s").arg(i);
        values[i] = 50;
    }

    QCPBars* bars=new QCPBars(ui->proWidget->xAxis,ui->proWidget->yAxis);
    QVector<double> index(SIZE);

    for(int i=0; i<SIZE; ++i)
        index[i] = i;
    bars->setData(index,values);
    ui->proWidget->xAxis->setAutoTicks(false);
    ui->proWidget->xAxis->setAutoTickLabels(false);
    ui->proWidget->xAxis->setAutoTickStep(false);

    ui->proWidget->addPlottable(bars);
    ui->proWidget->rescaleAxes();

    double wid = ui->proWidget->xAxis->range().upper - ui->proWidget->xAxis->range().lower;
    double cl  = bars->width() + (1.0*wid - bars->width()*SIZE) / (SIZE - 1);

    QVector<double> coor;
    for(int i=0; i<SIZE; ++i)
        coor.append(ui->proWidget->xAxis->range().lower + i*cl+bars->width()/2);

    ui->proWidget->xAxis->setTickVector(coor);

    ui->proWidget->xAxis->setTickVectorLabels(labels);

    ui->proWidget->replot();
}
