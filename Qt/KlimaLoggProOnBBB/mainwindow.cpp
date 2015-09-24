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
    m_kldatabase = new KLDatabase(this);

    FILE *fd = fopen(SENSOR,"wb");
    if(!fd)
    {
        qDebug() << "could not open" << SENSOR;
    }else
    {
        //write last read index to the driver
        int index = m_kldatabase->getLastRetrievedIndex();
        qDebug() << "LastRetrievedIndex from database: " << index;
//        index = 42000; //static for testing
        fwrite(&index,sizeof(int),1,fd);
        fclose(fd);
    }
    m_controller = new Controller(m_kldatabase);
    m_controller->operate("startThread");

    connect(m_controller,SIGNAL(resultReady()),this,SLOT(OnDrawPlot()));
    connect(m_controller,SIGNAL(readErrno(int)),this,SLOT(HandleErrNo(int)));
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
}



MainWindow::~MainWindow()
{
    delete m_kldatabase;
    delete ui;
    delete m_MSGBox;
}

void MainWindow::HandleErrNo(int error)
{
    if(error == 121)
    {
        if(!m_MSGBox->isVisible())
        {
            m_MSGBox->setText("Please press hte USB Button on your KlimaLoggPro");
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

    //reconfigure axis
    ui->customPlot->xAxis->setRange(x1[0], x1[count-1]);
    int tickStep = (int)m_kldatabase->GetTimeIntervall() / 4;
    ui->customPlot->xAxis->setTickStep(tickStep);

    //redraw
    ui->customPlot->replot();

}

void MainWindow::makePlot()
{
    // create and configure plottables:
    QCPGraph *graph1 = ui->customPlot->addGraph();
    QCPGraph *graph2 = ui->customPlot->addGraph();
    QCPGraph *graph3 = ui->customPlot->addGraph();
    QCPGraph *graph4 = ui->customPlot->addGraph();

    graph1->setPen(QPen(QColor(Qt::yellow), 4));
    graph2->setPen(QPen(QColor(0, 172, 172), 4));
    graph3->setPen(QPen(QColor(255, 0, 0), 4));
    graph4->setPen(QPen(QColor(0, 0, 255), 4));

    graph1->setName("Innen Temperatur");
    graph2->setName("Innen Luftfeuchtigkeit");
    graph3->setName("Aussen Temperatur");
    graph4->setName("Aussen Luftfeuchtigkeit");

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
//    ui->customPlot->xAxis->setTickStep(6);

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
    plotGradient.setColorAt(1, QColor(80, 80, 80));
    ui->customPlot->setBackground(plotGradient);
    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(30, 30, 30));
    axisRectGradient.setColorAt(1, QColor(30, 30, 30));
    ui->customPlot->axisRect()->setBackground(axisRectGradient);
    ui->customPlot->yAxis->setRange(10, 70);
    ui->customPlot->xAxis->setLabelColor(Qt::white);
    ui->customPlot->yAxis->setLabelColor(Qt::white);
    ui->customPlot->xAxis->setLabel("Zeit");
    ui->customPlot->yAxis->setLabel("Temperatur");

//    ui->customPlot->yAxis2->setVisible(true);
//    ui->customPlot->yAxis2->setRange(0, 100);
//    ui->customPlot->yAxis2->setLabel("Luftfeuchtigkeit");
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
