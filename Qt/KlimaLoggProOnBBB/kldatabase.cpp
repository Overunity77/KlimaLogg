#include "kldatabase.h"
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

const QString KLDatabase::sDatabaseName = KLIMALOGG_DATABASE;

KLDatabase::KLDatabase(QWidget *parent)
{
    int size;

    m_data = new QMap<long, Record>();
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

    size = readDatabase();

    qDebug() << "KLDatabase Constructor - read " << size << " records from database";
}

KLDatabase::~KLDatabase()
{
    //    delete plainModel;
    db->close();
    delete db;
}

long KLDatabase::readDatabase(void)
{
    int size;

    myQuery = new QSqlQuery("select dateTime,"
                            "       temp0, humidity0,"
                            "       temp1, humidity1,"
                            "       temp2, humidity2,"
                            "       temp3, humidity3,"
                            "       temp4, humidity4,"
                            "       temp5, humidity5,"
                            "       temp6, humidity6,"
                            "       temp7, humidity7,"
                            "       temp8, humidity8 "
                            "from measurement order by datetime asc", QSqlDatabase::database("KlimaLoggDb"));

    if(!myQuery->exec())
    {
        qDebug() << "query execution went wrong: " << myQuery->lastError().text();
        return -1;
    }

    //get record count
    myQuery->last();
    size = myQuery->at() + 1;

    //iterate trough all records beginnin on the first
    if(size > 0)
    {
        myQuery->first();
        do
        {
            Record data;

            QSqlRecord myRecord = myQuery->record();

            data.Timestamp = myRecord.field("dateTime").value().toLongLong();
            data.TimeValid = true;

            data.SensorDatas[0].Humidity = myRecord.field("humidity0").value().toDouble();
            data.SensorDatas[0].HumValid = true;
            data.SensorDatas[0].Temperature = myRecord.field("temp0").value().toDouble();
            data.SensorDatas[0].TempValid = true;

            data.SensorDatas[1].Humidity = myRecord.field("humidity1").value().toDouble();
            data.SensorDatas[1].HumValid = true;
            data.SensorDatas[1].Temperature = myRecord.field("temp1").value().toDouble();
            data.SensorDatas[1].TempValid = true;

            data.SensorDatas[2].Humidity = myRecord.field("humidity2").value().toDouble();
            data.SensorDatas[2].HumValid = true;
            data.SensorDatas[2].Temperature = myRecord.field("temp2").value().toDouble();
            data.SensorDatas[2].TempValid = true;

            data.SensorDatas[3].Humidity = myRecord.field("humidity3").value().toDouble();
            data.SensorDatas[3].HumValid = true;
            data.SensorDatas[3].Temperature = myRecord.field("temp3").value().toDouble();
            data.SensorDatas[3].TempValid = true;

            data.SensorDatas[4].Humidity = myRecord.field("humidity4").value().toDouble();
            data.SensorDatas[4].HumValid = true;
            data.SensorDatas[4].Temperature = myRecord.field("temp4").value().toDouble();
            data.SensorDatas[4].TempValid = true;

            data.SensorDatas[5].Humidity = myRecord.field("humidity5").value().toDouble();
            data.SensorDatas[5].HumValid = true;
            data.SensorDatas[5].Temperature = myRecord.field("temp5").value().toDouble();
            data.SensorDatas[5].TempValid = true;

            data.SensorDatas[6].Humidity = myRecord.field("humidity6").value().toDouble();
            data.SensorDatas[6].HumValid = true;
            data.SensorDatas[6].Temperature = myRecord.field("temp6").value().toDouble();
            data.SensorDatas[6].TempValid = true;

            data.SensorDatas[7].Humidity = myRecord.field("humidity7").value().toDouble();
            data.SensorDatas[7].HumValid = true;
            data.SensorDatas[7].Temperature = myRecord.field("temp7").value().toDouble();
            data.SensorDatas[7].TempValid = true;

            data.SensorDatas[8].Humidity = myRecord.field("humidity8").value().toDouble();
            data.SensorDatas[8].HumValid = true;
            data.SensorDatas[8].Temperature = myRecord.field("temp8").value().toDouble();
            data.SensorDatas[8].TempValid = true;

            // store to in-memory map
            m_data->insert(data.Timestamp, data);

        }while(myQuery->next());
    }

    return m_data->size();
}


void KLDatabase::StoreRecord(Record data)
{
    QDateTime timestamp;
    timestamp.setTime_t(data.Timestamp);


    qDebug() << "Start StoreRecord() - Timestamp: " << data.Timestamp << "-->" << timestamp.toString(Qt::SystemLocaleShortDate);
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

//    QMutexLocker locker(&m_mutex);
    // store to in-memory map
    if(!m_data->contains(data.Timestamp))
    {
        m_data->insert(data.Timestamp, data);
    }


}

void KLDatabase::updateLastRetrievedIndex(long index)
{
//    QMutexLocker locker(&m_mutex);

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
//    QMutexLocker locker(&m_mutex);

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

void KLDatabase::SetTimeIntervall(TimeIntervall value)
{
//    QMutexLocker locker(&m_mutex);

    m_TimeDiff = value;
}

TimeIntervall KLDatabase::GetTimeIntervall()
{
//    QMutexLocker locker(&m_mutex);

    return m_TimeDiff;
}

void KLDatabase::SetTickSpacing (TickSpacing spacing)
{
//    QMutexLocker locker(&m_mutex);
    m_TickSpacing = spacing;
}

TickSpacing KLDatabase::GetTickSpacing()
{
//    QMutexLocker locker(&m_mutex);
    return m_TickSpacing;
}

int KLDatabase::getNrOfValues()
{
    int counter = 0;
    long timediff = m_data->last().Timestamp - (long)m_TimeDiff;

    QMap<long, Record>::iterator it = m_data->find(timediff);

    while (it != m_data->end()) {
        counter++;
        ++it;
    }
    qDebug() << "KLDatabase::getNrOfValues() - return " << counter << " values";
    return counter;
}

int KLDatabase::getValues(QVector<double> *x1 , QVector<double> *y1, QVector<double> *y2, QVector<double> *y3 , QVector<double> *y4)
{
//    QMutexLocker locker(&m_mutex);
    QDateTime timestamp;

    int counter = 0;
    long timediff = m_data->last().Timestamp - (long)m_TimeDiff;

    QMap<long, Record>::iterator it = m_data->find(timediff);

    while (it != m_data->end()) {
        qDebug()  << " counter: " << counter ;
        (*x1)[counter] = it->Timestamp;
        timestamp.setTime_t(it->Timestamp);

        (*y1)[counter] = it->SensorDatas[0].Temperature;
        (*y2)[counter] = it->SensorDatas[0].Humidity;
        (*y3)[counter] = it->SensorDatas[1].Temperature;
        (*y4)[counter] = it->SensorDatas[1].Humidity;

        // qDebug() << "Record Nr: " << counter << "," << timestamp.toString(Qt::SystemLocaleShortDate) << "," << (*y1)[counter] << "," << (*y2)[counter] << "," << (*y3)[counter] << "," << *(y4)[counter];
        counter++;
        ++it;
    }
    qDebug() << "KLDatabase::getValues() - return " << counter << " values";
    return counter;
}


