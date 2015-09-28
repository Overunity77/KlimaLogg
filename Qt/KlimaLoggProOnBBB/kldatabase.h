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

    /**
     * @brief readDatabase
     * reads the whole databse into memory map
     * @return size of read data
     */
    long readDatabase(void);

    /**
     * @brief storeRecord
     * stores a record into database
     * @param data
     */
    void storeRecord(Record data);

    /**
     * @brief getNrOfValues
     * gets the count of values from the diff of the latest timestamp and the given TimeInterval
     * @param timeInterval
     * the timeInterval from wich the different gets builded
     * @return
     * the number of values between the time diff
     */
    int getNrOfValues(TimeInterval timeInterval);

    /**
     * @brief getValues
     * read the values within the given timeinterval from the in memory map
     * @param timeInterval
     * the timeinterval from with the data will be read
     * @param x1
     * holder for the timeValues values (x-axis)
     * @param y1
     * holder for the temperature of the first sensor
     * @param y2
     * holder for the humidity of the first sensor
     * @param y3
     * holder for the temperature of the second sensor
     * @param y4
     * holder for the humidity of the second sensor
     * @return
     */
    int getValues(TimeInterval timeInterval, QVector<double> *x1 , QVector<double> *y1, QVector<double> *y2, QVector<double> *y3 , QVector<double> *y4);

    /**
     * @brief updateLastRetrievedIndex
     * stores the latest read index in the database
     * @param index
     */
    void updateLastRetrievedIndex(long index);

    /**
     * @brief getLastRetrievedIndex
    * the index will be transferd to the driver to signal wich index the latest read was
     * @return
     */
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
