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

#include "definitions.h"

class KLDatabase
{
public:
    KLDatabase(QWidget *parent);
    ~KLDatabase();

    void StoreRecord(Record data);
    int getValues(QVector<double>& x1 , QVector<double>& y1, QVector<double>& y2, QVector<double>& y3 , QVector<double>& y4);
    void updateLastRetrievedIndex(long index);
private slots:

private:
    static const QString sDatabaseName;
    QSqlDatabase* db;
    //    QSqlQueryModel* plainModel;
    QSqlQuery* myQuery;
    KLDatabase() { };
};

#endif // KLDATABASE_H
