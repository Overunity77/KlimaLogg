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

    void SetUpperLimitTemperature();
    int GetUpperLimitTemperature();
    void SetLowerLimitTemperature();
    int GetLowerLimitTemperature();
    void SetUpperLimitHumidity();
    int GetUpperLimitHumidity();
    void SetLowerLimitHumidity();
    int GetLowerLimitHumidity();

private slots:

private:
    KLDatabase() {}

    static const QString sDatabaseName;
    QSqlDatabase* db;
    //    QSqlQueryModel* plainModel;
    QSqlQuery* myQuery;
    TimeIntervall m_TimeDiff;
    TickSpacing m_TickSpacing;
    QMutex m_mutex;

    int upperLimitTemperature;
    int lowerLimitTemperature;
    int upperLimitHumidity;
    int lowerLimitHumidity;

};

#endif // KLDATABASE_H
