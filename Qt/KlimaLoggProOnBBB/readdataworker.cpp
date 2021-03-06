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
    qDebug() << "ReadDataWorker::~ReadDataWorker() - Destructor called";
}


void ReadDataWorker::process()
{
    qDebug() << "ReadDataWorker::process() - ThreadId " << QThread::currentThreadId();

    unsigned char *usbframe = new unsigned char[USB_FRAME_SIZE];
    int retValue = 0;
    ResponseType response = ResponseType::INVALID;

    FILE *fd = NULL;
    fd = fopen(SENSOR, "rb");
    if(!fd)
    {
        qDebug() << "ReadDataWorker::process() - could not open" << SENSOR;
        emit finished();
        return;
    }

    while(!m_shutdown)
    {
        QThread::msleep(100);

        errno = 0;
        retValue = fread(usbframe, USB_FRAME_SIZE, 1, fd);
        qDebug() << "fread() - return:" << retValue << "(" << strerror(errno) << ")";

        //send error to gui, to make some user interaction
        emit readErrno(errno);
        if(retValue <= 0)
        {
            QThread::sleep(1);
            continue;
        }
        response = BitConverter::getResponseType(usbframe,238);

        if(response == RESPONSE_GET_HISTORY)
        {

            qDebug() << QString("RESPONSE_GET_HISTORY (0x%1)").arg(usbframe[6], 0, 16);

            int latestIndex = BitConverter::getLatestIndex(usbframe);
            int thisIndex = BitConverter::getThisIndex(usbframe);
            qDebug() << "latestIndex = "<<latestIndex;
            qDebug() << "thisIndex   = "<<thisIndex;

            for(int i = 0; i < 6;i++)
            {
                Record rec = BitConverter::getSensorValuesFromHistoryData(usbframe,i);
                m_kldatabase->storeRecord(rec);
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
