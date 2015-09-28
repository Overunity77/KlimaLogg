#ifndef KLDATABASE_H
#define KLDATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QMutexLocker>
#include <QMap>

#include "definitions.h"

class KLDatabase
{
public:
    KLDatabase(QWidget *parent);
    ~KLDatabase();

    long readDatabase(void);

    void StoreRecord(Record data);
    int getNrOfValues(TimeInterval timeInterval);
    int getValues(TimeInterval timeInterval, QVector<double> *x1 , QVector<double> *y1, QVector<double> *y2, QVector<double> *y3 , QVector<double> *y4);
    void updateLastRetrievedIndex(long index);
    int getLastRetrievedIndex();

private:
    KLDatabase() {}



    static const QString sDatabaseName;
    QSqlDatabase* db;
    QMap<long, Record> *m_data;
    QSqlQuery* myQuery;
    QMutex m_mutex;

};

#endif // KLDATABASE_H
