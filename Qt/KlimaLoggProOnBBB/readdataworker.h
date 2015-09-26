#ifndef READDATAWORKER_H
#define READDATAWORKER_H

#include "kldatabase.h"

#include <QObject>

class ReadDataWorker : public QObject
{
        Q_OBJECT
public:
    ReadDataWorker(KLDatabase *database);
    ~ReadDataWorker();

    void shutdown();

public slots:
    void process();

signals:
    void newData();
    void readErrno(int err);

private:
    KLDatabase *m_kldatabase;
    bool m_shutdown;
};

#endif // READDATAWORKER_H
