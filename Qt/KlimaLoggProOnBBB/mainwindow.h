#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTimer>
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


private slots:
    void HandleErrNo(int error);

    void selectShortTimespan();
    void selectMediumTimespan();
    void selectLongTimespan();

    void OnDrawPlot();

    void newData();

signals:
    void DrawPlot();
    void shutdownReader();

protected:
    void closeEvent(QCloseEvent * bar) Q_DECL_OVERRIDE;

private:
    void makePlot();


    Ui::MainWindow *ui;
    KLDatabase* m_kldatabase;


    QThread *m_AcquisitionThread;
    ReadDataWorker *m_reader;

    QTimer *m_UpdatePlotTimer;

    QMessageBox *m_MSGBox;

};

#endif // MAINWINDOW_H
