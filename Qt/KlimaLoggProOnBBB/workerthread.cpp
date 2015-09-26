#include "workerthread.h"


bool Worker::ReadUSBFrame()
{
//    char *usbframe = new char[238];
//    int retValue = 0;
//    ResponseType response = ResponseType::INVALID;
//    bool hasNewValues = false;
//    FILE *fd = NULL;
//    fd = fopen(SENSOR, "rb");
//    if(!fd)
//    {
//        qDebug() << "could not open" << SENSOR;
//        return false;
//    }
//    do
//    {
//        errno = 0;
//        retValue = fread(usbframe,238,1,fd);
//        qDebug() << "fread retValue: "<< retValue << " errno: " << strerror(errno);
//        //send error to gui, to make some user interaction
//        emit readErrno(errno);
//        if(retValue <= 0)
//        {
//            break;
//        }
//        response = BitConverter::GetResponseType(usbframe,238);
//        if(response == RESPONSE_GET_CURRENT)
//        {
//            hasNewValues = true;
//            qDebug() << "RESPONSE_GET_CURRENT";
//            Record rec = BitConverter::GetSensorValuesFromCurrentData(usbframe);
//            m_kldatabase->StoreRecord(rec);
//        }
//        else if(response == RESPONSE_GET_HISTORY)
//        {
//            hasNewValues = true;
//            qDebug() << "RESPONSE_GET_HISTORY";

//            intlatestIndex = BitConverter::GetLatestIndex(usbframe);
//            long thisIndex = BitConverter::GetThisIndex(usbframe);
//            qDebug() << "latestIndex = "<<latestIndex;
//            qDebug() << "thisIndex = "<<thisIndex;

//            for(int i = 0; i < 6;i++)
//            {
//                Record rec = BitConverter::GetSensorValuesFromHistoryData(usbframe,i);
//                m_kldatabase->StoreRecord(rec);
//            }

//            m_kldatabase->updateLastRetrievedIndex(thisIndex);
//        }
//        else
//        {
//            qDebug() << "Response Type " << response << " unknown";
//        }
//    }while(response == RESPONSE_GET_HISTORY);
//    fclose(fd);
//    return hasNewValues;
}
