#ifndef KLDATABASE_H
#define KLDATABASE_H


#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QMessageBox>
#include <QDateTime>
#include <QMutexLocker>
#include <QMap>

#include "definitions.h"

class KLDatabase
{
public:
    KLDatabase(QWidget *parent);
    ~KLDatabase();

    void StoreRecord(Record data);
    void SetTimeIntervall(TimeIntervall intervall);
    TimeIntervall GetTimeIntervall();
    int getValues(QVector<double>& x1 , QVector<double>& y1, QVector<double>& y2, QVector<double>& y3 , QVector<double>& y4);
    void updateLastRetrievedIndex(long index);
    int getLastRetrievedIndex();

    void SetTickSpacing (TickSpacing spacing);
    TickSpacing GetTickSpacing();
private slots:

private:
    KLDatabase() {}

    long readDatabase(void);

    static const QString sDatabaseName;
    QSqlDatabase* db;
    QMap<long, Record> *m_data;
    //    QSqlQueryModel* plainModel;
    QSqlQuery* myQuery;
    TimeIntervall m_TimeDiff;
    TickSpacing m_TickSpacing;
    QMutex m_mutex;

};

#endif // KLDATABASE_H
