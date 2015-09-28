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

    /**
     * @brief startAquisition
     * starts the data acquisition thread
     * @return
     * true on success
     */
    bool startAquisition();

private slots:
    /**
     * @brief handleErrNo
     * slot for the ability to communicate from the driver to the user
     * @param error
     * the error number from the driver, suggesting the error
     */
    void handleErrNo(int error);

    /**
     * @brief selectShortTimespan
     * user interaction on button click to set the short timespan (15 min)
     */
    void selectShortTimespan();

    /**
     * @brief selectMediumTimespan
     * user interaction on button click to set the medium timespan (24 hours)
     */
    void selectMediumTimespan();

    /**
     * @brief selectLongTimespan
     * user interaction on button click to set the long timespan (7 days)
     */
    void selectLongTimespan();

    /**
     * @brief onDrawPlot
     * reads the data and draws the plot with the preveously set timespan
     */
    void onDrawPlot();

    /**
     * @brief newData
     * slot wich gets called if new data has arrived
     */
    void newData();

    /**
     * @brief onMenuExit
     * exit method
     */
    void onMenuExit();

signals:

    /**
     * @brief drawPlot
     * signal to singalize draw request
     */
    void drawPlot();

protected:
    void closeEvent(QCloseEvent * bar) Q_DECL_OVERRIDE;

private:
    void makePlot();
    void setButtonActive(QPushButton* button);
    void setButtonNormal(QPushButton* button);

    double getMaxValue(QVector<double> *data);
    double getMinValue(QVector<double> *data);

    void setTimeInterval(TimeInterval value);
    TimeInterval getTimeInterval();
    void setTickSpacing (TickSpacing spacing);
    TickSpacing getTickSpacing();

    Ui::MainWindow *ui;
    KLDatabase* m_kldatabase;
    QThread *m_acquisitionThread;
    ReadDataWorker *m_reader;
    QTimer *m_updatePlotTimer;
    QMessageBox *m_MSGBox;
    QVector<double> *x1, *y1, *y2, *y3, *y4;
    TimeInterval m_timeInterval;
    TickSpacing m_tickSpacing;

};

#endif // MAINWINDOW_H
