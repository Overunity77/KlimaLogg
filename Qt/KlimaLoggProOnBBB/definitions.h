#ifndef DEFINITIONS
#define DEFINITIONS

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

enum ResponseType
{
    INVALID = 0x00,
    RESPONSE_DATA_WRITTEN = 0x10,
    RESPONSE_GET_CONFIG = 0x20,
    RESPONSE_GET_CURRENT = 0x30,
    RESPONSE_GET_HISTORY = 0x40,
    RESPONE_REQUEST = 0x50
};


#endif // DEFINITIONS

