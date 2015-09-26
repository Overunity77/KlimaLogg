#include "kldatabase.h"
#include <QDebug>
#include "QDateTime"

const QString KLDatabase::sDatabaseName = KLIMALOGG_DATABASE;

KLDatabase::KLDatabase(QWidget *parent)
{
    db = new QSqlDatabase();
    *db = QSqlDatabase::addDatabase("QSQLITE","KlimaLoggDb");
    db->setDatabaseName(sDatabaseName);
    m_TimeDiff = TimeIntervall::LONG;
    m_TickSpacing = TickSpacing::DAYS;

    if (!db->open()) {
        QMessageBox::critical(0, parent->tr("Cannot open database"),
                              parent->tr("Unable to establish a database connection.\n"
                                         "This example needs SQLite support. Please read "
                                         "the Qt SQL driver documentation for information how "
                                         "to build it.\n\n"
                                         "Click Cancel to exit."), QMessageBox::Cancel);
    }
    myQuery = new QSqlQuery("select dateTime, temp0, humidity0, temp3, humidity3 from measurement where datetime >=(1442788380 - 86400) and datetime <= 1442788380 order by datetime asc", QSqlDatabase::database("KlimaLoggDb"));
}

KLDatabase::~KLDatabase()
{
    //    delete plainModel;
    db->close();
    delete db;
}

void KLDatabase::StoreRecord(Record data)
{
    QMutexLocker locker(&m_mutex);

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
    QMutexLocker locker(&m_mutex);

    myQuery->prepare("UPDATE PARAMETER SET VALUE= :index WHERE KEY='lastRetrievedIndex'");

    myQuery->bindValue(":index", (qlonglong)index);

    if (! myQuery->exec() ) {
        qDebug() << myQuery->lastError();
    } else {
        qDebug() << "lastRetrievedIndex updated";
    }

}

int KLDatabase::getLastRetrievedIndex()
{
    QMutexLocker locker(&m_mutex);

    int ret;

    myQuery->prepare("SELECT VALUE from parameter WHERE KEY='lastRetrievedIndex'");

    if (! myQuery->exec() ) {
        qDebug() << myQuery->lastError();
        ret = -1;
    } else {
        myQuery->first();
        QSqlField value = myQuery->record().field("VALUE");
        ret = value.value().toInt();
    }
    return ret;
}


void KLDatabase::SetUpperLimitTemperature()
{
    myQuery->prepare("select max(temp0) as max_temp0, max(temp1) AS max_temp1 from measurement where datetime >= :timediff");
    myQuery->bindValue(":timediff",(int)TimeIntervall::LONG);
    if (! myQuery->exec() ) {
        qDebug() << myQuery->lastError();
    } else {
        myQuery->first();
        QSqlField max_1 = myQuery->record().field("max_temp0");
        QSqlField max_2 = myQuery->record().field("max_temp1");
        if (max_1.value().toInt() >= max_2.value().toInt()) {
            upperLimitTemperature = (((int)(max_1.value().toInt())/5)+1) * 5;
        }
        else {
            upperLimitTemperature = (((int)(max_2.value().toInt())/5)+1) * 5;
        }
    }
}


void KLDatabase::SetLowerLimitTemperature()
{
    myQuery->prepare("select min(temp0) as min_temp0, min(temp1) AS min_temp1 from measurement where datetime >= :timediff");
    myQuery->bindValue(":timediff",(int)TimeIntervall::LONG);
    if (! myQuery->exec() ) {
        qDebug() << myQuery->lastError();
    } else {
        myQuery->first();
        QSqlField min_1 = myQuery->record().field("min_temp0");
        QSqlField min_2 = myQuery->record().field("min_temp1");
        if (min_1.value().toInt() <= min_2.value().toInt()) {
            lowerLimitTemperature = (((int)(min_1.value().toInt())/5)-1) * 5;
        }
        else {
            lowerLimitTemperature = (((int)(min_2.value().toInt())/5)-1) * 5;
        }
    }
}


void KLDatabase::SetUpperLimitHumidity()
{
    myQuery->prepare("select max(humidity0) as max_humidity0, max(humidity1) AS max_humidity1 from measurement where datetime >= :timediff");
    myQuery->bindValue(":timediff",(int)TimeIntervall::LONG);
    if (! myQuery->exec() ) {
        qDebug() << myQuery->lastError();
    } else {
        myQuery->first();
        QSqlField max_1 = myQuery->record().field("max_humidity0");
        QSqlField max_2 = myQuery->record().field("max_humidity1");
        if (max_1.value().toInt() >= max_2.value().toInt()) {
            upperLimitHumidity = (((int)(max_1.value().toInt())/5)+1) * 5;
        }
        else {
            upperLimitHumidity = (((int)(max_2.value().toInt())/5)+1) * 5;
        }
    }
}


void KLDatabase::SetLowerLimitHumidity()
{
    myQuery->prepare("select min(humidity0) as min_humidity0, min(humidity1) AS min_humidity1 from measurement where datetime >= :timediff");
    myQuery->bindValue(":timediff",(int)TimeIntervall::LONG);
    if (! myQuery->exec() ) {
        qDebug() << myQuery->lastError();
    } else {
        myQuery->first();
        QSqlField min_1 = myQuery->record().field("min_humidity0");
        QSqlField min_2 = myQuery->record().field("min_humidity1");
        if (min_1.value().toInt() <= min_2.value().toInt()) {
            lowerLimitHumidity = (((int)(min_1.value().toInt())/5)-1) * 5;
        }
        else {
            lowerLimitHumidity = (((int)(min_2.value().toInt())/5)-1) * 5;
        }
    }
}

int KLDatabase::GetUpperLimitTemperature(){
    return upperLimitTemperature;
}

int KLDatabase::GetLowerLimitTemperature(){
    return lowerLimitTemperature;
}

int KLDatabase::GetUpperLimitHumidity(){
    return upperLimitHumidity;
}

int KLDatabase::GetLowerLimitHumidity(){
    return lowerLimitHumidity;
}


void KLDatabase::SetTimeIntervall(TimeIntervall value)
{
    QMutexLocker locker(&m_mutex);

    m_TimeDiff = value;
}

TimeIntervall KLDatabase::GetTimeIntervall()
{
    QMutexLocker locker(&m_mutex);

    return m_TimeDiff;
}

void KLDatabase::SetTickSpacing (TickSpacing spacing)
{
    m_TickSpacing = spacing;
}

TickSpacing KLDatabase::GetTickSpacing()
{
    return m_TickSpacing;
}


int KLDatabase::getValues(QVector<double>& x1 , QVector<double>& y1, QVector<double>& y2, QVector<double>& y3 , QVector<double>& y4)
{
    QMutexLocker locker(&m_mutex);

    int counter = 0;
    QDateTime timestamp;

    myQuery->prepare("select max(dateTime) as dateTime from measurement");
    if(!myQuery->exec())
    {
        qDebug() << "query execution went wrong";
        return 0;
    }
    myQuery->first();

    QSqlField dateTime = myQuery->record().field("dateTime");

    int timediff = dateTime.value().toInt() - (int)m_TimeDiff;
    myQuery->prepare("select dateTime, temp0, humidity0, temp1, humidity1 from measurement where datetime >= :timediff order by datetime asc");
    myQuery->bindValue(":timediff",timediff);
    if(!myQuery->exec())
    {
        qDebug() << "query execution went wrong";
        return 0;
    }

    //get record count
    myQuery->last();
    int size = myQuery->at() + 1;

    //iterate trough all records beginnin on the first
    if(size > 0)
    {
        myQuery->first();
        do
        {
            QSqlRecord myRecord = myQuery->record();

            QSqlField dateTime = myRecord.field("dateTime");

            x1[counter] = dateTime.value().toUInt();
            timestamp.setTime_t(dateTime.value().toUInt());

            QSqlField temp0 = myRecord.field("temp0");
            y1[counter] = temp0.value().toDouble();

            QSqlField humidity0 = myRecord.field("humidity0");
            y2[counter] = humidity0.value().toDouble();

            QSqlField temp1 = myRecord.field("temp1");
            y3[counter] = temp1.value().toDouble();

            QSqlField humidity1 = myRecord.field("humidity1");
            y4[counter] = humidity1.value().toDouble();

            qDebug() << "Record Nr: " << counter << "," << timestamp.toString(Qt::SystemLocaleShortDate) << "," << y1[counter] << "," << y2[counter] << "," << y3[counter] << "," << y4[counter];
            counter++;
        }while(myQuery->next());
    }
    return size;

}
