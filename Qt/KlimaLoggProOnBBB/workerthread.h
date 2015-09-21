#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QThread>
#include <QTimer>
#include <QDebug>
#include <errno.h>
#include <string.h>

#include "definitions.h"
#include "kldatabase.h"
#include "bitconverter.h"


class Worker : public QObject
{
    Q_OBJECT
public:
    Worker(KLDatabase *database) {
        m_kldatabase = database;
        m_AcquisitionTimer = new QTimer(this);
        m_AcquisitionTimer->setInterval(1000);
        connect(m_AcquisitionTimer, SIGNAL(timeout()), this, SLOT(TimerEvent()));
    }

public slots:
    void doWork(const QString &parameter) {
        TimerEvent();
        m_AcquisitionTimer->start();
    }

signals:
    void resultReady();
    void readErrno(int error);
private slots:
    void TimerEvent() {
        if(ReadUSBFrame())
        {
            emit resultReady();
        }
    }

private:
    bool ReadUSBFrame();
    QTimer *m_AcquisitionTimer;
    KLDatabase *m_kldatabase;
};

class Controller : public QObject
{
    Q_OBJECT
    QThread workerThread;
public:
    Controller(KLDatabase *database) {
        Worker *worker = new Worker(database);
        worker->moveToThread(&workerThread);
        connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(this, &Controller::operate, worker, &Worker::doWork);
        connect(worker, &Worker::resultReady, this, &Controller::handleResults);
        connect(worker,&Worker::readErrno,this,&Controller::handleErrno);
        workerThread.start();
    }
    ~Controller() {
        workerThread.quit();
        workerThread.wait();
    }
public slots:
    void handleResults()
    {
        emit resultReady();
    }

    void handleErrno(int error)
    {
        emit readErrno(error);
    }

signals:
    void operate(const QString &);
    void resultReady();
    void readErrno(int);
};
#endif // WORKERTHREAD_H
