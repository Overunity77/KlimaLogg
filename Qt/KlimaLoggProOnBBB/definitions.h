#ifndef DEFINITIONS
#define DEFINITIONS


#define SENSOR "/dev/kl0"
//#define SENSOR "/dev/kl2"

#define TIME_BASIS 1430431200
#define KLIMALOGG_DATABASE "/usr/local/bin/database/KlimaLoggPro.sdb"

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

enum TimeInterval
{
    SHORT = 900,    //15 min
    MEDIUM = 86400, // 24 hours
    LONG = 604800     //7 days
};

enum TickSpacing
{
    MINUTES = 300,  // 5 min
    HOURS = 14400,  // 4 hour
    DAYS = 86400   // 24 hours
};

#endif // DEFINITIONS

