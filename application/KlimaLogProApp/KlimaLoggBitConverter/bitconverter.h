#ifndef BITCONVERTER_H
#define BITCONVERTER_H

#include "bitconverter_global.h"

struct SensorData
{
    double Temperature;
    bool TempValid;
    double Humidity;
    bool HumValid;
};

struct Record
{
    long Timestamp;
    bool TimeValid;
    SensorData SensorDatas[9];
};


class BITCONVERTERSHARED_EXPORT BitConverter
{

public:
    BitConverter();
    static bool ConvertTemperature(short data, bool highByteFull, double *value);
    static bool ConvertHumidity(char data, double *value);
    static bool ConvertHistoryTimestamp(char *data, long *value);
    static bool ConvertCurrentTimestamp(char *data, bool startOnHighNibble, long *value);

    static Record GetSensorValuesFromHistoryData(char* frame, int index);
    static Record GetSensorValuesFromCurrentData(char* frame);


    //static void ConvertUSBFrame(char* data, long *timestamp, SesnorData *values);
};

#endif // BITCONVERTER_H
