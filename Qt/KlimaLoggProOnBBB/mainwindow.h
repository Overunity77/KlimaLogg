#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "kldatabase.h"

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
    void TimerEvent();
    void ReadUSBFrame();
    void makePlot();
    void DrawPlot();
    void selectShortTimespan();
    void selectMediumTimespan();
    void selectLongTimespan();

private:


    Ui::MainWindow *ui;
    KLDatabase* m_kldatabase;
    QThread *m_AcquisitionThread;
    QTimer *m_AcquisitionTimer;

    FILE *fd = NULL;

};

#endif // MAINWINDOW_H
