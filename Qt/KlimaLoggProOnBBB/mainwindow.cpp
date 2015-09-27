#include <stdio.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "readdataworker.h"
#include "bitconverter.h"

#include <QDebug>


#include <errno.h>
#include <string.h>

#define INIT_DATA_SIZE  10100

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qDebug() << "MainWindow(): " << QThread::currentThreadId();

    x1 = new QVector<double>(INIT_DATA_SIZE);
    y1 = new QVector<double>(INIT_DATA_SIZE);
    y2 = new QVector<double>(INIT_DATA_SIZE);
    y3 = new QVector<double>(INIT_DATA_SIZE);
    y4 = new QVector<double>(INIT_DATA_SIZE);

    ui->setupUi(this);
    m_kldatabase = new KLDatabase(this);

    m_UpdatePlotTimer = new QTimer(this);
    m_UpdatePlotTimer->setInterval(5000);


    m_AcquisitionThread = new QThread(this);
    m_reader = new ReadDataWorker(m_kldatabase);

    m_reader->moveToThread(m_AcquisitionThread);


    QObject::connect(m_AcquisitionThread, SIGNAL(started()), m_reader, SLOT(process()) );
    QObject::connect(m_AcquisitionThread, SIGNAL(finished()), m_reader, SLOT(deleteLater()) );
    QObject::connect(m_AcquisitionThread, SIGNAL(finished()), m_AcquisitionThread, SLOT(deleteLater()));

    QObject::connect(m_reader, SIGNAL(newData()), this, SLOT(newData()) );
    QObject::connect(m_reader, SIGNAL(readErrno(int)), this, SLOT(HandleErrNo(int)) );

    QObject::connect(m_UpdatePlotTimer, SIGNAL(timeout()), this, SLOT(OnDrawPlot()) );

    QObject::connect(ui->pushButton_1, SIGNAL(clicked()), this, SLOT(selectLongTimespan()) );
    QObject::connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(selectMediumTimespan()) );
    QObject::connect(ui->pushButton_3, SIGNAL(clicked()), this, SLOT(selectShortTimespan()) );
    QObject::connect(this,SIGNAL(DrawPlot()),this,SLOT(OnDrawPlot()) );

    QObject::connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(onMenuExit()) );

    m_MSGBox = new QMessageBox(this);
    m_MSGBox->setDefaultButton(QMessageBox::NoButton);
    m_MSGBox->setWindowTitle("Information");

    setButtonActive(ui->pushButton_1);

    // initialize plot
    makePlot();

}



MainWindow::~MainWindow()
{
    qDebug() << "MainWindow Destructor";

    m_UpdatePlotTimer->stop();
    delete m_UpdatePlotTimer;
    delete m_kldatabase;
    delete ui;
    delete m_MSGBox;
    delete x1;
    delete y1;
    delete y2;
    delete y3;
    delete y4;
}

bool MainWindow::startAquisition()
{
    FILE *fd = fopen(SENSOR,"wb");
    if(!fd)
    {
        qDebug() << "could not open" << SENSOR;

        QMessageBox::critical(0,
                              tr("Could not open %1").arg(SENSOR),
                              tr("Unable to establish a connection.\n"
                                 "to the KlimaLogg Pro USB Transceiver.\n\n"
                                 "Click Cancel to exit."),
                              QMessageBox::Cancel);

        close();
        return false;
    }
    else
    {
        //write last read index to the driver
        int index = m_kldatabase->getLastRetrievedIndex();

        qDebug() << "Writing LastRetrievedIndex from database to Driver: " << index;
        fwrite(&index,sizeof(int),1,fd);
        fclose(fd);

        // start reading form KlimaLogg Pro
        m_AcquisitionThread->start();
        m_AcquisitionThread->setPriority(QThread::LowPriority);

        OnDrawPlot();

        m_UpdatePlotTimer->start();
    }
    return true;

}

void MainWindow::closeEvent(QCloseEvent * bar)
{
    qDebug() << "MainWindow::closeEvent(QCloseEvent * bar)";

    QObject::disconnect(m_reader, SIGNAL(readErrno(int)), this, SLOT(HandleErrNo(int)) );

    m_reader->shutdown();

    if(m_AcquisitionThread->isRunning())
    {
        m_AcquisitionThread->quit();
        m_AcquisitionThread->wait(2000);
    }

    bar->accept();
}


void MainWindow::HandleErrNo(int error)
{
    static int errCounter = 0;

    if(error == 200)
    {
        errCounter++;
        if(errCounter > 1000)
            errCounter = 3;
        if( (!m_MSGBox->isVisible()) && (errCounter >=3))
        {
            m_MSGBox->setText("Please press the USB Button on your KlimaLoggPro");
            m_MSGBox->showNormal();
        }
    }
    else if(error == 0)
    {
        errCounter = 0;
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

    int nrOfValues = m_kldatabase->getNrOfValues();
    x1->resize(nrOfValues);
    y1->resize(nrOfValues);
    y2->resize(nrOfValues);
    y3->resize(nrOfValues);
    y4->resize(nrOfValues);

    int count = m_kldatabase->getValues(x1, y1, y2, y3, y4);

    if(count == 0)
        return;

    double y1Min = getMinValue(y1);
    double y1Max = getMaxValue(y1);
    double y2Min = getMinValue(y2);
    double y2Max = getMaxValue(y2);
    double y3Min = getMinValue(y3);
    double y3Max = getMaxValue(y3);
    double y4Min = getMinValue(y4);
    double y4Max = getMaxValue(y4);

    double minTemperature = y1Min <= y3Min ? y1Min : y3Min;
    double maxTemperature = y1Max >= y3Max ? y1Max : y3Max;
    double minHumidity    = y2Min <= y4Min ? y2Min : y4Min;
    double maxHumidity    = y2Max >= y4Max ? y2Max : y4Max;

    minTemperature = (minTemperature / 5.0 - 1.0) * 5.0;
    maxTemperature = (maxTemperature / 5.0 + 1.0) * 5.0;
    minHumidity    = (minHumidity / 5.0 - 1.0) * 5.0;
    maxHumidity    = (maxHumidity / 5.0 + 1.0) * 5.0;

    //update UI
    // get created graphs
    QCPGraph *graph1 = ui->customPlot->graph(0);
    QCPGraph *graph2 = ui->customPlot->graph(1);
    QCPGraph *graph3 = ui->customPlot->graph(2);
    QCPGraph *graph4 = ui->customPlot->graph(3);

    //.. and update the values
    graph1->setData(*x1, *y1);
    graph2->setData(*x1, *y2);
    graph3->setData(*x1, *y3);
    graph4->setData(*x1, *y4);

    // calculate tick below lowest x axis value and above highest x axis value
    long minTick = ((long)(((*x1)[0] - TIME_BASIS) / m_kldatabase->GetTickSpacing()) * m_kldatabase->GetTickSpacing()) + TIME_BASIS ;
    long maxTick = ((long)(((*x1)[count-1] - TIME_BASIS) /m_kldatabase->GetTickSpacing()+1) * m_kldatabase->GetTickSpacing()) + TIME_BASIS;

    // generete and set ticks for x axis
    QVector<double> xAxisTicks(0);
    int i =0;
    long actualTick = minTick;
    while ( actualTick <= maxTick){
        actualTick =  minTick + i * m_kldatabase->GetTickSpacing();
        xAxisTicks.append(actualTick);
        i = i + 1;
    }
    ui->customPlot->xAxis->setTickVector(xAxisTicks);

    //reconfigure axis
    ui->customPlot->xAxis->setRange((*x1)[0], (*x1)[count-1]);

    ui->customPlot->yAxis->setRange(minTemperature, maxTemperature);
    ui->customPlot->yAxis2->setRange(minHumidity, maxHumidity);

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

    ui->customPlot->xAxis->setLabelColor(Qt::white);
    ui->customPlot->yAxis->setLabelColor(Qt::white);
    ui->customPlot->yAxis2->setLabelColor(Qt::white);

    ui->customPlot->xAxis->setLabel("Zeit");
    ui->customPlot->yAxis->setLabel("Temperatur [°C]");
    ui->customPlot->yAxis2->setLabel("relative Luftfeuchtigkeit [%]");

    ui->customPlot->xAxis->setSubTickCount(3);
    ui->customPlot->xAxis->setDateTimeFormat("dd.MM.yy");

    ui->customPlot->yAxis2->setVisible(true);
}


void MainWindow::setButtonActive(QPushButton* button)
{
    QColor backgroundColour;
    backgroundColour.setNamedColor("red");
    QPalette Pal(palette());
    Pal.setColor(QPalette::Button, backgroundColour);
    button->setAutoFillBackground(true);
    button->setPalette(Pal);
}

void MainWindow::setButtonNormal(QPushButton* button)
{
    QPalette Pal(palette());
    Pal.setColor(QPalette::Button, Pal.color(QPalette::Button));
    button->setAutoFillBackground(true);
    button->setPalette(Pal);
}

double MainWindow::getMaxValue(QVector<double> *data)
{
    double maxValue = -1000;

    QVector<double>::iterator it = data->begin();

    while (it != data->end()) {
        if(*it > maxValue)
        {
            maxValue = *it;
        }
        ++it;
    }
//    qDebug() << "MainWindow::getMaxValue() - maxValue: " << maxValue;
    return maxValue;
}

double MainWindow::getMinValue(QVector<double> *data)
{
    double minValue = 1000;

    QVector<double>::iterator it = data->begin();

    while (it != data->end()) {
        if(*it < minValue)
        {
            minValue = *it;
        }
        ++it;
    }
//    qDebug() << "MainWindow::getMinValue() - minValue: " << minValue;
    return minValue;
}


void MainWindow::selectShortTimespan()
{
    qDebug() << "15 Minuten";

    setButtonNormal(ui->pushButton_1);
    setButtonNormal(ui->pushButton_2);
    setButtonActive(ui->pushButton_3);

    m_kldatabase->SetTimeIntervall(TimeIntervall::SHORT);
    m_kldatabase->SetTickSpacing(TickSpacing::MINUTES);
    ui->customPlot->xAxis->setSubTickCount(4);
    ui->customPlot->xAxis->setDateTimeFormat("dd.MM.yy hh:mm");

    //  OnDrawPlot();
    emit DrawPlot();
}


void MainWindow::selectMediumTimespan()
{
    qDebug() << "24 Stunden";

    setButtonNormal(ui->pushButton_1);
    setButtonActive(ui->pushButton_2);
    setButtonNormal(ui->pushButton_3);

    m_kldatabase->SetTimeIntervall(TimeIntervall::MEDIUM);
    m_kldatabase->SetTickSpacing(TickSpacing::HOURS);
    ui->customPlot->xAxis->setSubTickCount(3);
    ui->customPlot->xAxis->setDateTimeFormat("dd.MM.yy hh:mm");

    //    OnDrawPlot();
    emit DrawPlot();
}

void MainWindow::selectLongTimespan()
{
    qDebug() << "7 Tage";

    setButtonActive(ui->pushButton_1);
    setButtonNormal(ui->pushButton_2);
    setButtonNormal(ui->pushButton_3);

    m_kldatabase->SetTimeIntervall(TimeIntervall::LONG);
    m_kldatabase->SetTickSpacing(TickSpacing::DAYS);
    ui->customPlot->xAxis->setSubTickCount(3);
    ui->customPlot->xAxis->setDateTimeFormat("dd.MM.yy");

    //   OnDrawPlot();
    emit DrawPlot();
}

void MainWindow::newData()
{
    qDebug() << "MainWindow::newData()" << QThread::currentThreadId();
}

void MainWindow::onMenuExit()
{
    close();
    qApp->quit();
}
