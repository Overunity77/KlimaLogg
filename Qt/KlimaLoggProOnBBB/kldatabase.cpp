#include "kldatabase.h"
#include <QDebug>

const QString KLDatabase::sDatabaseName = "/usr/local/bin/database/KlimaLoggPro.sdb";

KLDatabase::KLDatabase(QWidget *parent)
{
    db = new QSqlDatabase();
    *db = QSqlDatabase::addDatabase("QSQLITE","KlimaLoggDb");
    db->setDatabaseName(sDatabaseName);
    if (!db->open()) {
        QMessageBox::critical(0, parent->tr("Cannot open database"),
                              parent->tr("Unable to establish a database connection.\n"
                                         "This example needs SQLite support. Please read "
                                         "the Qt SQL driver documentation for information how "
                                         "to build it.\n\n"
                                         "Click Cancel to exit."), QMessageBox::Cancel);
    }
    //    plainModel = new QSqlQueryModel();
    //    plainModel->setQuery("select * from archive", QSqlDatabase::database("KlimaLoggDb"));
}

KLDatabase::~KLDatabase()
{
    //    delete plainModel;
    db->close();
    delete db;
}

bool KLDatabase::getValues(QVector<double>& x1 , QVector<double>& y1)
{
    int counter = 0;
    QSqlQuery* myQuery = new QSqlQuery("select dateTime, temp0 from measurement", QSqlDatabase::database("KlimaLoggDb"));

    //   bool queryOk = myQuery->last();
    while (myQuery->next()) {
        QSqlRecord myRecord = myQuery->record();

        QSqlField dateTime = myRecord.field("dateTime");
        x1[counter] = dateTime.value().toUInt();
        QSqlField temp1 = myRecord.field("temp0");
        y1[counter] = temp1.value().toDouble();
        counter = counter +1;
    }


    return true;

}
