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

    /**
     * @brief shutdown
     * let the worker shutdown
     */
    void shutdown();

public slots:

    /**
     * @brief process
     * starts the worker thread
     */
    void process();

signals:

    /**
     * @brief newData
     * signals that newData was read
     */
    void newData();

    /**
     * @brief readErrno
     * @param err
     * signals wich errno was set, gets also called if no errno is set
     */
    void readErrno(int err);

    /**
     * @brief finished
     * signals the end of the worker thread
     */
    void finished();

private:
    KLDatabase *m_kldatabase;
    bool m_shutdown;
};

#endif // READDATAWORKER_H
