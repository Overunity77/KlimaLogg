#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include <QMessageBox>
#include "kldatabase.h"
#include "readdataworker.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool startAquisition();

private slots:
    void HandleErrNo(int error);

    void selectShortTimespan();
    void selectMediumTimespan();
    void selectLongTimespan();

    void OnDrawPlot();

    void newData();
    void onMenuExit();

signals:
    void DrawPlot();
    void shutdownReader();

protected:
    void closeEvent(QCloseEvent * bar) Q_DECL_OVERRIDE;

private:
    void makePlot();
    void setButtonActive(QPushButton* button);
    void setButtonNormal(QPushButton* button);

    double getMaxValue(QVector<double> *data);
    double getMinValue(QVector<double> *data);

    void SetTimeInterval(TimeInterval value);
    TimeInterval GetTimeInterval();
    void SetTickSpacing (TickSpacing spacing);
    TickSpacing GetTickSpacing();

    Ui::MainWindow *ui;
    KLDatabase* m_kldatabase;
    QThread *m_AcquisitionThread;
    ReadDataWorker *m_reader;
    QTimer *m_UpdatePlotTimer;
    QMessageBox *m_MSGBox;
    QVector<double> *x1, *y1, *y2, *y3, *y4;
    TimeInterval m_TimeInterval;
    TickSpacing m_TickSpacing;

};

#endif // MAINWINDOW_H
