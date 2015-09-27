#include <QDebug>
#include <QThread>

#include "readdataworker.h"
#include "bitconverter.h"

#define USB_FRAME_SIZE  0x111

ReadDataWorker::ReadDataWorker(KLDatabase *database)
    : m_kldatabase(database),
      m_shutdown(false)
{

}

ReadDataWorker::~ReadDataWorker()
{
    qDebug() << "ReadDataWorker::~ReadDataWorker()";
}


void ReadDataWorker::process()
{
    unsigned char *usbframe = new unsigned char[USB_FRAME_SIZE];
    int retValue = 0;
    ResponseType response = ResponseType::INVALID;

    FILE *fd = NULL;
    fd = fopen(SENSOR, "rb");
    if(!fd)
    {
        qDebug() << "could not open" << SENSOR;
        emit finished();
        return;
    }

    while(!m_shutdown)
    {
        QThread::msleep(100);

        errno = 0;
        retValue = fread(usbframe, USB_FRAME_SIZE, 1, fd);
        qDebug() << "fread retValue: "<< retValue << " errno: " << strerror(errno);

        //send error to gui, to make some user interaction
        emit readErrno(errno);
        if(retValue <= 0)
        {
            QThread::sleep(1);
            continue;
        }
        response = BitConverter::GetResponseType(usbframe,238);

        if(response == RESPONSE_GET_HISTORY)
        {

            qDebug() << QString("RESPONSE_GET_HISTORY (0x%1)").arg(usbframe[6], 0, 16);

            int latestIndex = BitConverter::GetLatestIndex(usbframe);
            int thisIndex = BitConverter::GetThisIndex(usbframe);
            qDebug() << "latestIndex = "<<latestIndex;
            qDebug() << "thisIndex   = "<<thisIndex;

            for(int i = 0; i < 6;i++)
            {
                Record rec = BitConverter::GetSensorValuesFromHistoryData(usbframe,i);
                m_kldatabase->StoreRecord(rec);
            }

            m_kldatabase->updateLastRetrievedIndex(thisIndex);

            emit newData();
        }
        else
        {
            qDebug() << QString("Ignoring Response Type (0x%1)").arg(response, 0, 16);

        }
    }

    fclose(fd);
    qDebug() << "ReadDataWorker::process() - finished";

    emit finished();

}


void ReadDataWorker::shutdown()
{
    m_shutdown = true;

}
