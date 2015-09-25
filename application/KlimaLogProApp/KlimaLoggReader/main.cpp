#include <QCoreApplication>

#include "bitconverter.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QString>
#include <QDateTime>


#include <stdio.h>
#include <unistd.h>
#include <errno.h>

/* defined in Grad Celsius */
#define TEMPERATURE_OFFSET 40

#define FRAME_SIZE 0x111

int main(int argc, char *argv[])
{
//    QCoreApplication a(argc, argv);

//    return a.exec();

    qint64 bufSize = FRAME_SIZE;
    char *buf = new char[bufSize];
    qint64 dataSize;

    QDateTime timestamp;

    Record dataRecord;
 //   QFile file("/dev/kl2");
    int counter = 0;


    FILE *fd_klimalogg;
    fd_klimalogg = NULL;

    int retValue = 0;

    fd_klimalogg = fopen("/dev/kl0", "r+b");
    //fd_klimalogg = fopen("/dev/kl1", "r+b");

    if (!fd_klimalogg) {
        printf("kann /dev/kl0 nicht oeffnen\n");
        return -1;
    }

//    if (!file.open(QIODevice::ReadOnly)) {
//        printf("Could not open device!\n");
//        return 1;
//    }

//    usleep(75000);

    //int index = 2147483647;
    //int index = 51182;
    int index = -1;

    retValue = (int) fwrite(&index, sizeof(int), 1, fd_klimalogg);

    qDebug() << QString("written %1 bytes to driver, retValue: %2").arg(sizeof(int)).arg(retValue);

    retValue = 0;

//            dataSize = file.read(buf, bufSize);


   // while (counter < 2000) {
    while (true) {

        //while (!file.atEnd()) {

           errno = 0;

           retValue = (int)fread(buf, 1, 0x111, fd_klimalogg);

           if(retValue < 0)
           {
               qDebug() << QString("could not read from file (%1), errno: %2").arg(retValue).arg(errno);
               //usleep(1000000);
               //usleep(75000);
               continue;
           }
           else if(retValue == 0)
           {
               qDebug() << QString("no new history data available (%1), errno: %2").arg(retValue).arg(errno);
               usleep(1000000);
               continue;
           }
           else
           {
               qDebug() << QString("read bytes: %1, errno: %2").arg(retValue).arg(errno);

               printf("buf[6]: 0x%x\n", buf[6]);

           }

//            if(dataSize < 0)
//            {
//                qDebug() << QString("could not read from file (%1)").arg(dataSize);
//                continue;
//            }
//            else
//            {
//                qDebug() << QString("read bytes: %1").arg(dataSize);

//            }

            /* process data */
        //}

        printf("buf[6]: 0x%x\n", buf[6]);

        if(buf[6]==0x30)
        {
            dataRecord = BitConverter::GetSensorValuesFromCurrentData(buf);

            timestamp.setTime_t(dataRecord.Timestamp);


            printf("--------------------------\n" \
                   "currentData\n" \
                   "--------------------------\n" \
                   "Timestamp     : %ld (%s)\n" \
                   "TimeValid     : %s\n" \
                   "Temperature 0 : %03.1f valid: %s\n" \
                   "Humidity 0    : %03.1f valid: %s\n" \
                   "Temperature 1 : %03.1f valid: %s\n" \
                   "Humidity 1    : %03.1f valid: %s\n" \
                   "Temperature 2 : %03.1f valid: %s\n" \
                   "Humidity 2    : %03.1f valid: %s\n" \
                   "Temperature 3 : %03.1f valid: %s\n" \
                   "Humidity 3    : %03.1f valid: %s\n" \
                   "Temperature 4 : %03.1f valid: %s\n" \
                   "Humidity 4    : %03.1f valid: %s\n" \
                   "Temperature 5 : %03.1f valid: %s\n" \
                   "Humidity 5    : %03.1f valid: %s\n" \
                   "Temperature 6 : %03.1f valid: %s\n" \
                   "Humidity 6    : %03.1f valid: %s\n" \
                   "Temperature 7 : %03.1f valid: %s\n" \
                   "Humidity 7    : %03.1f valid: %s\n" \
                   "Temperature 8 : %03.1f valid: %s\n" \
                   "Humidity 8    : %03.1f valid: %s\n",
                   dataRecord.Timestamp,
                   timestamp.toString(Qt::SystemLocaleShortDate).toStdString().c_str(),
                   dataRecord.TimeValid == true ? "true" : "false",
                   dataRecord.SensorDatas[0].Temperature,
                   dataRecord.SensorDatas[0].TempValid == true ? "true" : "false",
                   dataRecord.SensorDatas[0].Humidity,
                   dataRecord.SensorDatas[0].HumValid == true ? "true" : "false",
                   dataRecord.SensorDatas[1].Temperature,
                   dataRecord.SensorDatas[1].TempValid == true ? "true" : "false",
                   dataRecord.SensorDatas[1].Humidity,
                   dataRecord.SensorDatas[1].HumValid == true ? "true" : "false",
                   dataRecord.SensorDatas[2].Temperature,
                   dataRecord.SensorDatas[2].TempValid == true ? "true" : "false",
                   dataRecord.SensorDatas[2].Humidity,
                   dataRecord.SensorDatas[2].HumValid == true ? "true" : "false",
                   dataRecord.SensorDatas[3].Temperature,
                   dataRecord.SensorDatas[3].TempValid == true ? "true" : "false",
                   dataRecord.SensorDatas[3].Humidity,
                   dataRecord.SensorDatas[3].HumValid == true ? "true" : "false",
                   dataRecord.SensorDatas[4].Temperature,
                   dataRecord.SensorDatas[4].TempValid == true ? "true" : "false",
                   dataRecord.SensorDatas[4].Humidity,
                   dataRecord.SensorDatas[4].HumValid == true ? "true" : "false",
                   dataRecord.SensorDatas[5].Temperature,
                   dataRecord.SensorDatas[5].TempValid == true ? "true" : "false",
                   dataRecord.SensorDatas[5].Humidity,
                   dataRecord.SensorDatas[5].HumValid == true ? "true" : "false",
                   dataRecord.SensorDatas[6].Temperature,
                   dataRecord.SensorDatas[6].TempValid == true ? "true" : "false",
                   dataRecord.SensorDatas[6].Humidity,
                   dataRecord.SensorDatas[6].HumValid == true ? "true" : "false",
                   dataRecord.SensorDatas[7].Temperature,
                   dataRecord.SensorDatas[7].TempValid == true ? "true" : "false",
                   dataRecord.SensorDatas[7].Humidity,
                   dataRecord.SensorDatas[7].HumValid == true ? "true" : "false",
                   dataRecord.SensorDatas[7].Temperature,
                   dataRecord.SensorDatas[7].TempValid == true ? "true" : "false",
                   dataRecord.SensorDatas[7].Humidity,
                   dataRecord.SensorDatas[7].HumValid == true ? "true" : "false");


        }
        else if (buf[6] == 0x40)
        {
            for(int i = 0; i < 6; i++)
            {
                dataRecord = BitConverter::GetSensorValuesFromHistoryData(buf, i);

                timestamp.setTime_t(dataRecord.Timestamp);


                printf("--------------------------\n" \
                       "historyData   : %d\n" \
                       "--------------------------\n" \
                       "Hist Timestamp     : %ld (%s)\n" \
                       "Hist TimeValid     : %s\n" \
                       "Hist Temperature 0 : %03.1f valid: %s\n" \
                       "Hist Humidity 0    : %03.1f valid: %s\n" \
                       "Hist Temperature 1 : %03.1f valid: %s\n" \
                       "Hist Humidity 1    : %03.1f valid: %s\n" \
                       "Hist Temperature 2 : %03.1f valid: %s\n" \
                       "Hist Humidity 2    : %03.1f valid: %s\n" \
                       "Hist Temperature 3 : %03.1f valid: %s\n" \
                       "Hist Humidity 3    : %03.1f valid: %s\n" \
                       "Hist Temperature 4 : %03.1f valid: %s\n" \
                       "Hist Humidity 4    : %03.1f valid: %s\n" \
                       "Hist Temperature 5 : %03.1f valid: %s\n" \
                       "Hist Humidity 5    : %03.1f valid: %s\n" \
                       "Hist Temperature 6 : %03.1f valid: %s\n" \
                       "Hist Humidity 6    : %03.1f valid: %s\n" \
                       "Hist Temperature 7 : %03.1f valid: %s\n" \
                       "Hist Humidity 7    : %03.1f valid: %s\n" \
                       "Hist Temperature 8 : %03.1f valid: %s\n" \
                       "Hist Humidity 8    : %03.1f valid: %s\n",
                       i,
                       dataRecord.Timestamp,
                       timestamp.toString(Qt::SystemLocaleShortDate).toStdString().c_str(),
                       dataRecord.TimeValid == true ? "true" : "false",
                       dataRecord.SensorDatas[0].Temperature,
                       dataRecord.SensorDatas[0].TempValid == true ? "true" : "false",
                       dataRecord.SensorDatas[0].Humidity,
                       dataRecord.SensorDatas[0].HumValid == true ? "true" : "false",
                       dataRecord.SensorDatas[1].Temperature,
                       dataRecord.SensorDatas[1].TempValid == true ? "true" : "false",
                       dataRecord.SensorDatas[1].Humidity,
                       dataRecord.SensorDatas[1].HumValid == true ? "true" : "false",
                       dataRecord.SensorDatas[2].Temperature,
                       dataRecord.SensorDatas[2].TempValid == true ? "true" : "false",
                       dataRecord.SensorDatas[2].Humidity,
                       dataRecord.SensorDatas[2].HumValid == true ? "true" : "false",
                       dataRecord.SensorDatas[3].Temperature,
                       dataRecord.SensorDatas[3].TempValid == true ? "true" : "false",
                       dataRecord.SensorDatas[3].Humidity,
                       dataRecord.SensorDatas[3].HumValid == true ? "true" : "false",
                       dataRecord.SensorDatas[4].Temperature,
                       dataRecord.SensorDatas[4].TempValid == true ? "true" : "false",
                       dataRecord.SensorDatas[4].Humidity,
                       dataRecord.SensorDatas[4].HumValid == true ? "true" : "false",
                       dataRecord.SensorDatas[5].Temperature,
                       dataRecord.SensorDatas[5].TempValid == true ? "true" : "false",
                       dataRecord.SensorDatas[5].Humidity,
                       dataRecord.SensorDatas[5].HumValid == true ? "true" : "false",
                       dataRecord.SensorDatas[6].Temperature,
                       dataRecord.SensorDatas[6].TempValid == true ? "true" : "false",
                       dataRecord.SensorDatas[6].Humidity,
                       dataRecord.SensorDatas[6].HumValid == true ? "true" : "false",
                       dataRecord.SensorDatas[7].Temperature,
                       dataRecord.SensorDatas[7].TempValid == true ? "true" : "false",
                       dataRecord.SensorDatas[7].Humidity,
                       dataRecord.SensorDatas[7].HumValid == true ? "true" : "false",
                       dataRecord.SensorDatas[7].Temperature,
                       dataRecord.SensorDatas[7].TempValid == true ? "true" : "false",
                       dataRecord.SensorDatas[7].Humidity,
                       dataRecord.SensorDatas[7].HumValid == true ? "true" : "false");
            }
        }

        //usleep(5000);
        counter++;
    }

    delete(buf);

//    file.close();
    fclose(fd_klimalogg);



//    FILE *fd_klimalogg;
//    fd_klimalogg = NULL;
//    char data[100];

//    int retValue = 0;

//    int counter = 0;

//    fd_klimalogg = fopen("/dev/kl2", "rb");
//    if (!fd_klimalogg) {
//        printf("kann /dev/kl2 nicht oeffnen\n");
//        return -1;
//    }

//    usleep(75000);

//    while (counter < 20) {
//    //while (retValue == 0) {
//        retValue = (int)fread(data, 0x111, 1, fd_klimalogg);
//        printf("retValue= %d\n", retValue);
////		if (retValue) {
////			printf("Days   : %02d\n", data[0]);
////			printf("Month  : %02d\n", data[1]);
////			printf("Year   : %04d\n", (int)data[2] + 2000);
////			printf("Hours  : %02d\n", data[3]);
////			printf("Minutes: %02d\n", data[4]);
////			printf("Humidity : %02d\n", data[5]);
////			printf("Temp : %02d.%1d\n", (int)data[6]- TEMPERATURE_OFFSET, data[7]);
////			counter = counter +1;
////			printf("Record Nr : %d\n\n", counter);
////		}
//    }
//    fclose(fd_klimalogg);

    return 0;

}
