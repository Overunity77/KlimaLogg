#include <stdio.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "bitconverter.h"

#include <QDebug>
#include <errno.h>
#include <string.h>

#define TEMPERATURE_OFFSET 40

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton_1, SIGNAL(clicked()), this, SLOT(selectLongTimespan()));
    connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(selectMediumTimespan()));
    connect(ui->pushButton_3, SIGNAL(clicked()), this, SLOT(selectShortTimespan()));
    connect(this,SIGNAL(DrawPlot()),SLOT(OnDrawPlot()));

    m_kldatabase = new KLDatabase(this);
    m_AcquisitionThread = new QThread(this);
    m_AcquisitionTimer = new QTimer(0);
    // setup signal and slot
    connect(m_AcquisitionTimer, SIGNAL(timeout()),
            m_AcquisitionThread, SLOT(TimerEvent()));
    m_AcquisitionTimer->setInterval(200);
    m_AcquisitionTimer->moveToThread(m_AcquisitionThread);
    m_AcquisitionThread->start();

    m_AcquisitionTimer->start();
    //m_AcquisitionTimer->start(60000);
    //m_AcquisitionTimer->singleShot(0,m_AcquisitionTimer,this,SLOT(TimerEvent()));
    m_MSGBox = new QMessageBox(this);
    m_MSGBox->setDefaultButton(QMessageBox::NoButton);
    m_MSGBox->setWindowTitle("information");
    m_MSGBox->setText("Please press hte USB Button on your KlimaLoggPro");
    //initialize plot

    makePlot();
    emit DrawPlot();
    QTimer::singleShot(0,this,SLOT(TimerEvent()));
}



MainWindow::~MainWindow()
{
    delete m_kldatabase;
    delete ui;

}

void MainWindow::TimerEvent()
{
    ReadUSBFrame();
    emit DrawPlot();
}


void MainWindow::ReadUSBFrame()
{

    char *usbframe = new char[238];
    int retValue = 0;
    ResponseType response = ResponseType::INVALID;

    FILE *fd = NULL;
    fd = fopen("/dev/kl0", "rb");
    if(!fd)
    {
        qDebug() << "could not open /dev/kl1";
        return;
    }
    do
    {
        errno = 0;
        retValue = fread(usbframe,238,1,fd);
        qDebug() << "(int)usbframe[6]: "<< (int)usbframe[6];
        if(retValue <= 0)
        {
            if(errno == 121)
            {
                if(!m_MSGBox->isVisible())
                {
                    m_MSGBox->showNormal();
                }
            }
            else
            {
                qDebug() << "Error: " << errno << strerror(errno);
            }
            break;
        }
        if(m_MSGBox->isVisible())
        {
            m_MSGBox->close();
        }
        response = BitConverter::GetResponseType(usbframe,238);
        if(response == RESPONSE_GET_CURRENT)
        {
            qDebug() << "RESPONSE_GET_CURRENT";
            Record rec = BitConverter::GetSensorValuesFromCurrentData(usbframe);
            m_kldatabase->StoreRecord(rec);
        }
        else if(response == RESPONSE_GET_HISTORY)
        {
            qDebug() << "RESPONSE_GET_HISTORY";

            long  latestIndex = BitConverter::GetLatestIndex(usbframe);
            long thisIndex = BitConverter::GetThisIndex(usbframe);
            qDebug() << "latestIndex = "<<latestIndex;
            qDebug() << "thisIndex = "<<thisIndex;

            for(int i = 0; i < 6;i++)
            {
                Record rec = BitConverter::GetSensorValuesFromHistoryData(usbframe,i);
                m_kldatabase->StoreRecord(rec);
            }

            m_kldatabase->updateLastRetrievedIndex(thisIndex); // TODO sollte mal thisIndex mit
            // latestIndex vergleichen
        }
        else
        {
            qDebug() << "Response Type " << response << " unknown";
        }
    }while(response == RESPONSE_GET_HISTORY);
    fclose(fd);
}


//
//  Get new values and update plot
//
void MainWindow::OnDrawPlot()
{
    QVector<double> x1(140000), y1(140000), y2(140000), y3(140000), y4(140000);

    int count = m_kldatabase->getValues(x1, y1, y2, y3, y4);

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

    ui->customPlot->xAxis->setRange(x1[0], x1[count-1]);
    int tickStep = (int)m_kldatabase->GetTimeIntervall() / 4;
    ui->customPlot->xAxis->setTickStep(tickStep);
    ui->customPlot->replot();
    //ui->customPlot->update();
}

void MainWindow::makePlot()
{
    // prepare data:
    //    double actualTime = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
    // QVector<double> x1(14000), y1(14000);
    double actualTime = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
    qDebug() << "actualTime" << actualTime ;


    /*QVector<double> x1(14000), y1(14000), y2(14000), y3(14000), y4(14000);

    //read DB
    int recordCount = m_kldatabase->getValues(x1, y1, y2, y3, y4);
    qDebug() << "recordCount read to display " << recordCount;
*/
    // create and configure plottables:
    QCPGraph *graph1 = ui->customPlot->addGraph();
    QCPGraph *graph2 = ui->customPlot->addGraph();
    QCPGraph *graph3 = ui->customPlot->addGraph();
    QCPGraph *graph4 = ui->customPlot->addGraph();

    /*
    graph1->setData(x1, y1);
    graph2->setData(x1, y2);
    graph3->setData(x1, y3);
    graph4->setData(x1, y4);
    */
    //   graph1->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(Qt::white), 9));

    graph1->setPen(QPen(Qt::red));                 //(QPen(QColor(120, 120, 120), 2));
    graph2->setPen(QPen(Qt::blue));
    graph3->setPen(QPen(Qt::green));
    graph4->setPen(QPen(Qt::yellow));

    graph1->setLineStyle(QCPGraph::lsLine);
    graph2->setLineStyle(QCPGraph::lsLine);
    graph3->setLineStyle(QCPGraph::lsLine);
    graph4->setLineStyle(QCPGraph::lsLine);

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
    ui->customPlot->xAxis->setTickStep(86400/4); //(2*86400);

    ui->customPlot->yAxis->setAutoTickStep(false);
    ui->customPlot->yAxis->setTickStep(5);

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

    //ui->customPlot->xAxis->setRange(x1[0], x1[recordCount-1]);
    ui->customPlot->yAxis->setRange(10, 70);

    ui->customPlot->xAxis->setLabel("Zeit");
    ui->customPlot->yAxis->setLabel("Temperatur");


}

void MainWindow::selectShortTimespan()
{
    qDebug() << "15 Minuten";
    m_kldatabase->SetTimeIntervall(TimeIntervall::SHORT);
    emit DrawPlot();
}


void MainWindow::selectMediumTimespan()
{
    qDebug() << "24 Stunden";
    m_kldatabase->SetTimeIntervall(TimeIntervall::MEDIUM);
    emit DrawPlot();
}

void MainWindow::selectLongTimespan()
{
    qDebug() << "7 Tage";
    m_kldatabase->SetTimeIntervall(TimeIntervall::LONG);
    emit DrawPlot();
}
