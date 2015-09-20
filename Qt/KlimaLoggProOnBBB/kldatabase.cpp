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

    // 900 sec = 15 min
    // 86'400 sec = 24 hours
    // 604'800 sec = 7 days
    myQuery = new QSqlQuery("select dateTime, temp0, humidity0, temp3, humidity3 from measurement where datetime >=(1442788380 - 86400) and datetime <= 1442788380 order by datetime asc", QSqlDatabase::database("KlimaLoggDb"));

    //    plainModel = new QSqlQueryModel();
    //    plainModel->setQuery("select * from archive", QSqlDatabase::database("KlimaLoggDb"));
}

KLDatabase::~KLDatabase()
{
    //    delete plainModel;
    db->close();
    delete db;
}

void KLDatabase::StoreRecord(Record data)
{
    qDebug() << "Start StoreRecord()";
    QVariant null = QVariant();

    myQuery->prepare("INSERT INTO measurement (dateTime,temp0,humidity0,temp1,humidity1,temp2,humidity2,temp3,humidity3"
                     ",temp4,humidity4,temp5,humidity5,temp6,humidity6,temp7,humidity7,temp8,humidity8)"
                     " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");

    myQuery->addBindValue((qlonglong)data.Timestamp);

    for(int i = 0;i< 9;i++)
    {
        QVariant temp = QVariant(data.SensorDatas[i].Temperature);
        QVariant humi = QVariant(data.SensorDatas[i].Humidity);

        myQuery->addBindValue(data.SensorDatas[i].TempValid ? temp : null);
        myQuery->addBindValue(data.SensorDatas[i].HumValid ? humi : null);
    }

    if (! myQuery->exec() ) {
        qDebug() << myQuery->lastError();
    } else {
        qDebug() << "after exec() of INSERT: OK";
    }

}

void KLDatabase::updateLastRetrievedIndex(long index)
{
    myQuery->prepare("UPDATE PARAMETER SET VALUE= :index WHERE KEY='lastRetrievedIndex'");

    myQuery->bindValue(":index", (qlonglong)index);

    if (! myQuery->exec() ) {
        qDebug() << myQuery->lastError();
    } else {
        qDebug() << "lastRetrievedIndex updated";
    }

}



int KLDatabase::getValues(QVector<double>& x1 , QVector<double>& y1, QVector<double>& y2, QVector<double>& y3 , QVector<double>& y4)
{
    int counter = 0;
    //   QSqlQuery* myQuery = new QSqlQuery("select dateTime, temp0, humidity0, temp1, humidity1 from measurement", QSqlDatabase::database("KlimaLoggDb"));

    //   bool queryOk = myQuery->last();
    while (myQuery->next()) {
        QSqlRecord myRecord = myQuery->record();

        QSqlField dateTime = myRecord.field("dateTime");

        x1[counter] = dateTime.value().toUInt();
        QSqlField temp0 = myRecord.field("temp0");
        y1[counter] = temp0.value().toDouble();

        QSqlField humidity0 = myRecord.field("humidity0");
        y2[counter] = humidity0.value().toDouble();

        QSqlField temp1 = myRecord.field("temp3");
        y3[counter] = temp1.value().toDouble();

        QSqlField humidity1 = myRecord.field("humidity3");
        y4[counter] = humidity1.value().toDouble();

        qDebug() << "Record Nr: " << counter << "," << x1[counter] << "," << y1[counter] << "," << y2[counter] << "," << y3[counter] << "," << y4[counter];

        counter = counter +1;
    }
    return (counter -1);

}
