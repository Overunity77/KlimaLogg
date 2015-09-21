#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "kldatabase.h"
#include "workerthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
    //void TimerEvent();
    void HandleErrNo(int error);
    void makePlot();
    void OnDrawPlot();
    void selectShortTimespan();
    void selectMediumTimespan();
    void selectLongTimespan();
signals:
    void DrawPlot();

private:


    Ui::MainWindow *ui;
    KLDatabase* m_kldatabase;
    QThread *m_AcquisitionThread;
    QTimer *m_AcquisitionTimer;
    QMessageBox *m_MSGBox;
    FILE *fd = NULL;
    //WorkerThread *m_worker;
    Controller *m_controller;
};

#endif // MAINWINDOW_H
