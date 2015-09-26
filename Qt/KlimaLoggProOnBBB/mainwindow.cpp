#include <stdio.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "readdataworker.h"
#include "bitconverter.h"

#include <QDebug>


#include <errno.h>
#include <string.h>

#define TEMPERATURE_OFFSET 40

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qDebug() << "MainWindow(): " << QThread::currentThreadId();

    ui->setupUi(this);
    m_kldatabase = new KLDatabase(this);

    m_UpdatePlotTimer = new QTimer(this);
    m_UpdatePlotTimer->setInterval(5000);


    FILE *fd = fopen(SENSOR,"wb");
    if(!fd)
    {
        qDebug() << "could not open" << SENSOR;

    }else
    {
        //write last read index to the driver
        int index = m_kldatabase->getLastRetrievedIndex();
        qDebug() << "Writing LastRetrievedIndex from database to Driver: " << index;
        fwrite(&index,sizeof(int),1,fd);
        fclose(fd);

    }

    m_AcquisitionThread = new QThread(this);
    m_reader = new ReadDataWorker(m_kldatabase);

    m_reader->moveToThread(m_AcquisitionThread);


    QObject::connect(m_AcquisitionThread, SIGNAL(started()), m_reader, SLOT(process()) );
    QObject::connect(m_AcquisitionThread, SIGNAL(finished()), m_reader, SLOT(deleteLater()) );
    QObject::connect(m_AcquisitionThread, SIGNAL(finished()), m_AcquisitionThread, SLOT(deleteLater()));

    QObject::connect(m_reader, SIGNAL(newData()), this, SLOT(newData()) );
    QObject::connect(m_reader, SIGNAL(readErrno(int)), this, SLOT(HandleErrNo(int)) );

    QObject::connect(m_UpdatePlotTimer, SIGNAL(timeout()), this, SLOT(OnDrawPlot()));

    connect(ui->pushButton_1, SIGNAL(clicked()), this, SLOT(selectLongTimespan()));
    connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(selectMediumTimespan()));
    connect(ui->pushButton_3, SIGNAL(clicked()), this, SLOT(selectShortTimespan()));
    connect(this,SIGNAL(DrawPlot()),this,SLOT(OnDrawPlot()));

    m_MSGBox = new QMessageBox(this);
    m_MSGBox->setDefaultButton(QMessageBox::NoButton);
    m_MSGBox->setWindowTitle("information");

    //initialize plot
    makePlot();
    OnDrawPlot();


    m_AcquisitionThread->start();
    m_UpdatePlotTimer->start();
}



MainWindow::~MainWindow()
{
    qDebug() << "MainWindow Destructor";

    m_UpdatePlotTimer->stop();
    delete m_UpdatePlotTimer;
    delete m_kldatabase;
    delete ui;
    delete m_MSGBox;
}

void MainWindow::closeEvent(QCloseEvent * bar)
{
    qDebug() << "MainWindow::closeEvent(QCloseEvent * bar)";

    QObject::disconnect(m_reader, SIGNAL(readErrno(int)), this, SLOT(HandleErrNo(int)) );

    m_reader->shutdown();

    m_AcquisitionThread->quit();
    m_AcquisitionThread->wait(2000);

    bar->accept();
}

void MainWindow::HandleErrNo(int error)
{
    if(error == 200)
    {
        if(!m_MSGBox->isVisible())
        {
            m_MSGBox->setText("Please press the USB Button on your KlimaLoggPro");
            m_MSGBox->showNormal();
        }
    }
    else if(error == 0)
    {
        if(m_MSGBox->isVisible())
        {
            m_MSGBox->close();
        }
    }else
    {
        qDebug() << "Error: " << error << strerror(error);
    }
}


//
//  Get new values and update plot
//
void MainWindow::OnDrawPlot()
{
    qDebug() << "MainWindow::OnDrawPlot()" << QThread::currentThreadId();

    QVector<double> x1(140000), y1(140000), y2(140000), y3(140000), y4(140000);

    int count = m_kldatabase->getValues(x1, y1, y2, y3, y4);

    if(count == 0)
        return;

    //update UI
    // get created graphs
    QCPGraph *graph1 = ui->customPlot->graph(0);
    QCPGraph *graph2 = ui->customPlot->graph(1);
    QCPGraph *graph3 = ui->customPlot->graph(2);
    QCPGraph *graph4 = ui->customPlot->graph(3);

    //.. and update the values
    graph1->setData(x1, y1);
    graph2->setData(x1, y2);
    graph3->setData(x1, y3);
    graph4->setData(x1, y4);

    //reconfigure axis
    ui->customPlot->xAxis->setRange(x1[0], x1[count-1]);

    //redraw
    ui->customPlot->replot();
}

void MainWindow::makePlot()
{
    // create and configure plottables:
    QCPGraph *graph1 = ui->customPlot->addGraph();
    QCPGraph *graph2 = ui->customPlot->addGraph(ui->customPlot->xAxis, ui->customPlot->yAxis2);
    QCPGraph *graph3 = ui->customPlot->addGraph();
    QCPGraph *graph4 = ui->customPlot->addGraph(ui->customPlot->xAxis, ui->customPlot->yAxis2);

    graph1->setPen(QPen(QColor(Qt::yellow), 4));
    graph2->setPen(QPen(QColor(0, 80, 80), 4));
    graph3->setPen(QPen(QColor(255, 0, 0), 4));
    graph4->setPen(QPen(QColor(0, 0, 120), 4));

    graph1->setName("innen Temperatur");
    graph2->setName("innen Luftfeuchtigkeit");
    graph3->setName("aussen Temperatur");
    graph4->setName("aussen Luftfeuchtigkeit");

    ui->customPlot->legend->setVisible(true);

    graph1->setLineStyle(QCPGraph::lsLine);
    graph2->setLineStyle(QCPGraph::lsLine);
    graph3->setLineStyle(QCPGraph::lsLine);
    graph4->setLineStyle(QCPGraph::lsLine);

    // make legend align in top left corner
    ui->customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignTop);

    // move bars above graphs and grid below bars:
    ui->customPlot->addLayer("abovemain", ui->customPlot->layer("main"),QCustomPlot::limAbove);
    ui->customPlot->addLayer("belowmain", ui->customPlot->layer("main"), QCustomPlot::limBelow);
    graph1->setLayer("abovemain");
    ui->customPlot->xAxis->grid()->setLayer("belowmain");
    ui->customPlot->yAxis->grid()->setLayer("belowmain");
    ui->customPlot->yAxis2->grid()->setLayer("belowmain");

    // set some pens, brushes and backgrounds:
    ui->customPlot->xAxis->setBasePen(QPen(Qt::white, 1));
    ui->customPlot->yAxis->setBasePen(QPen(Qt::white, 1));
    ui->customPlot->yAxis2->setBasePen(QPen(Qt::white, 1));

    ui->customPlot->xAxis->setTickPen(QPen(Qt::white, 1));
    ui->customPlot->yAxis->setTickPen(QPen(Qt::white, 1));
    ui->customPlot->yAxis2->setTickPen(QPen(Qt::white, 1));

    ui->customPlot->xAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->customPlot->yAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->customPlot->yAxis2->setSubTickPen(QPen(Qt::white, 1));

    ui->customPlot->xAxis->setTickLabelColor(Qt::white);
    ui->customPlot->yAxis->setTickLabelColor(Qt::white);
    ui->customPlot->yAxis2->setTickLabelColor(Qt::white);

    ui->customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);

    ui->customPlot->xAxis->setAutoTicks(false);

    ui->customPlot->xAxis->setAutoTickStep(false);
    ui->customPlot->yAxis->setAutoTickStep(false);
    ui->customPlot->yAxis2->setAutoTickStep(false);

    ui->customPlot->yAxis->setTickStep(5);
    ui->customPlot->yAxis2->setTickStep(5);

    ui->customPlot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->customPlot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    ui->customPlot->yAxis2->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));

    ui->customPlot->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->customPlot->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    ui->customPlot->yAxis2->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));

    ui->customPlot->xAxis->grid()->setSubGridVisible(true);
    ui->customPlot->yAxis->grid()->setSubGridVisible(true);
    ui->customPlot->yAxis2->grid()->setSubGridVisible(true);

    ui->customPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->customPlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->customPlot->yAxis2->grid()->setZeroLinePen(Qt::NoPen);

    ui->customPlot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    ui->customPlot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    ui->customPlot->yAxis2->setUpperEnding(QCPLineEnding::esSpikeArrow);

    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QColor(80, 80, 80));
    plotGradient.setColorAt(1, QColor(80, 80, 80));
    ui->customPlot->setBackground(plotGradient);
    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(30, 30, 30));
    axisRectGradient.setColorAt(1, QColor(30, 30, 30));
    ui->customPlot->axisRect()->setBackground(axisRectGradient);
    ui->customPlot->yAxis->setRange(10, 30);
    ui->customPlot->yAxis2->setRange(35, 70);

    ui->customPlot->xAxis->setLabelColor(Qt::white);
    ui->customPlot->yAxis->setLabelColor(Qt::white);
    ui->customPlot->yAxis2->setLabelColor(Qt::white);

    ui->customPlot->xAxis->setLabel("Zeit");
    ui->customPlot->yAxis->setLabel("Temperatur [°C]");
    ui->customPlot->yAxis2->setLabel("relative Luftfeuchtigkeit [%]");

    QVector<double> xAxisTicks;
    xAxisTicks << 1442613600 << 1442700000 << 1442786400 << 1442872800 << 1442959200 << 1443045600 << 1443132000 << 1443218400 << 1443304800;
    ui->customPlot->xAxis->setTickVector(xAxisTicks);
    ui->customPlot->xAxis->setSubTickCount(3);
    ui->customPlot->xAxis->setDateTimeFormat("dd.MM.yy");

    ui->customPlot->yAxis2->setVisible(true);
}

void MainWindow::selectShortTimespan()
{
    qDebug() << "15 Minuten";
    m_kldatabase->SetTimeIntervall(TimeIntervall::SHORT);
    QVector<double> xAxisTicks;
    xAxisTicks << 1443243600 << 1443243600 +300  << 1443243600 +600 << 1443243600 +900 << 1443243600  +1200<< 1443243600  +1500 << 1443243600  +1800
               << 1443243600 +2100  << 1443243600 +2400 << 1443243600 +2700 << 1443243600 +3000 << 1443243600 +3300 << 1443243600 +3600
               << 1443243600 +3600 << 1443243600 +300 +3600 << 1443243600 +600 +3600<< 1443243600 +900 +3600<< 1443243600  +1200+3600<< 1443243600  +1500 +3600<< 1443243600  +1800+3600
               << 1443243600 +2100 +3600 << 1443243600 +2400 +3600<< 1443243600 +2700 +3600<< 1443243600 +3000 +3600<< 1443243600 +3300 +3600<< 1443243600 +3600+3600
               << 1443243600 +2*3600<< 1443243600 +300 +2*3600 << 1443243600 +600 +2*3600<< 1443243600 +900 +2*3600<< 1443243600  +1200+2*3600<< 1443243600  +1500 +2*3600<< 1443243600  +1800+2*3600
               << 1443243600 +2100+2*3600  << 1443243600 +2400 +2*3600<< 1443243600 +2700+2*3600 << 1443243600 +3000 +2*3600<< 1443243600 +3300 +2*3600<< 1443243600 +3600     +2*3600
                  ;
    ui->customPlot->xAxis->setTickVector(xAxisTicks);
    ui->customPlot->xAxis->setSubTickCount(4);
    ui->customPlot->xAxis->setDateTimeFormat("dd.MM.yy hh:mm");

  //  OnDrawPlot();
    // emit DrawPlot();
        ui->customPlot->replot();
}


void MainWindow::selectMediumTimespan()
{
    qDebug() << "24 Stunden";
    m_kldatabase->SetTimeIntervall(TimeIntervall::MEDIUM);
    QVector<double> xAxisTicks;
    xAxisTicks << 1443132000 << 1443132000 +4*3600 << 1443132000 +8*3600 << 1443132000 +12*3600 << 1443132000  +16*3600<< 1443132000  +20*3600 << 1443132000  +24*3600
               << 1443132000 +28*3600  << 1443132000 +32*3600 << 1443132000 +36*3600 << 1443132000 +40*3600 << 1443132000 +44*3600 << 1443132000 +48*3600 ;
    ui->customPlot->xAxis->setTickVector(xAxisTicks);
    ui->customPlot->xAxis->setSubTickCount(3);
    ui->customPlot->xAxis->setDateTimeFormat("dd.MM.yy hh:mm");

//    OnDrawPlot();
    // emit DrawPlot();
        ui->customPlot->replot();
}

void MainWindow::selectLongTimespan()
{
    qDebug() << "7 Tage";
    m_kldatabase->SetTimeIntervall(TimeIntervall::LONG);

    QVector<double> xAxisTicks;
    xAxisTicks << 1442613600 << 1442700000 << 1442786400 << 1442872800 << 1442959200 << 1443045600 << 1443132000 << 1443218400 << 1443304800;
    ui->customPlot->xAxis->setTickVector(xAxisTicks);
    ui->customPlot->xAxis->setSubTickCount(3);
    ui->customPlot->xAxis->setDateTimeFormat("dd.MM.yy");

 //   OnDrawPlot();
    // emit DrawPlot();
        ui->customPlot->replot();
}

void MainWindow::newData() {
    qDebug() << "MainWindow::newData()" << QThread::currentThreadId();
}
