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
    bool getValues(QVector<double>& x1 , QVector<double>& y1);

private:
    static const QString sDatabaseName;
    QSqlDatabase* db;
    //    QSqlQueryModel* plainModel;

    KLDatabase() { };
};

#endif // KLDATABASE_H
