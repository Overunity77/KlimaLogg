#include <stdio.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

#define TEMPERATURE_OFFSET 40

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{


    ui->setupUi(this);
    m_kldatabase = new KLDatabase(this);

    MainWindow::makePlot();
}

MainWindow::~MainWindow()
{
    delete m_kldatabase;
    delete ui;
}


void MainWindow::makePlot()
{
    // prepare data:
//    double actualTime = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
    QVector<double> x1(14000), y1(14000);


/*
    char data[100];
    int retValue = 0;
    int counter = 0;

    FILE *fd_klimalogg = NULL;

    fd_klimalogg = fopen("/dev/usb_test", "rb");
    if (!fd_klimalogg) {
        qDebug() << "kann /dev/usb_test nicht oeffnen";
    }

    qDebug() << "actualTime" << actualTime;

 //   y1[0] = 10;
    double firstTimePoint;
    firstTimePoint=  QDateTime(  QDate(2015, 9,13),  QTime(17, 20)).toMSecsSinceEpoch()/1000.0;
 //   x1[0] = firstTimePoint;

    qDebug() << "firstTimePoint" << firstTimePoint;


    while (counter < 20) {
        retValue = (int)fread(data, 8, 1, fd_klimalogg);
        //printf("retValue= %d\n", retValue);
        if (retValue) {
            qDebug() << "Record Nr :" << counter;
            x1[counter] = QDateTime(  QDate((int)data[2] + 2000, (int)data[1],(int)data[0]),  QTime((int)data[3], (int)data[4])).toMSecsSinceEpoch()/1000.0;
            qDebug() << x1[counter];
            y1[counter] = (int)data[6] - (int)TEMPERATURE_OFFSET + ((double)data[7])/10;
            qDebug() << y1[counter];
            counter = counter +1;

        }
    }
    fclose(fd_klimalogg);
*/



    //read DB
    bool ok = m_kldatabase->getValues(x1, y1);

    if (ok)
    {
        //update UI

    }
    else
    {
        //qDebug();
    }





    // create and configure plottables:
    QCPGraph *graph1 = ui->customPlot->addGraph();
    graph1->setData(x1, y1);
    graph1->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(Qt::white), 9));
    graph1->setPen(QPen(QColor(120, 120, 120), 2));


    // move bars above graphs and grid below bars:
    ui->customPlot->addLayer("abovemain", ui->customPlot->layer("main"),QCustomPlot::limAbove);
    ui->customPlot->addLayer("belowmain", ui->customPlot->layer("main"), QCustomPlot::limBelow);
    graph1->setLayer("abovemain");
    ui->customPlot->xAxis->grid()->setLayer("belowmain");
    ui->customPlot->yAxis->grid()->setLayer("belowmain");

    // set some pens, brushes and backgrounds:
    ui->customPlot->xAxis->setBasePen(QPen(Qt::white, 1));
    ui->customPlot->yAxis->setBasePen(QPen(Qt::white, 1));
    ui->customPlot->xAxis->setTickPen(QPen(Qt::white, 1));
    ui->customPlot->yAxis->setTickPen(QPen(Qt::white, 1));
    ui->customPlot->xAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->customPlot->yAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->customPlot->xAxis->setTickLabelColor(Qt::white);
    ui->customPlot->yAxis->setTickLabelColor(Qt::white);

    ui->customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->customPlot->xAxis->setDateTimeFormat("dd.MM.yyyy hh:mm");
    ui->customPlot->xAxis->setAutoTickStep(false);
    ui->customPlot->xAxis->setTickStep(86400);

    ui->customPlot->yAxis->setAutoTickStep(false);
    ui->customPlot->yAxis->setTickStep(1);

    ui->customPlot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->customPlot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->customPlot->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->customPlot->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->customPlot->xAxis->grid()->setSubGridVisible(true);
    ui->customPlot->yAxis->grid()->setSubGridVisible(true);
    ui->customPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->customPlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->customPlot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    ui->customPlot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QColor(80, 80, 80));
    plotGradient.setColorAt(1, QColor(50, 50, 50));
    ui->customPlot->setBackground(plotGradient);
    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(80, 80, 80));
    axisRectGradient.setColorAt(1, QColor(30, 30, 30));
    ui->customPlot->axisRect()->setBackground(axisRectGradient);

    ui->customPlot->xAxis->setRange(x1[0], x1[13000]);
    ui->customPlot->yAxis->setRange(20, 32);


}
