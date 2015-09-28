#ifndef DEFINITIONS
#define DEFINITIONS


#define SENSOR "/dev/kl0"
//#define SENSOR "/dev/kl2"

#define TIME_BASIS 1430431200
#define KLIMALOGG_DATABASE "/usr/local/bin/database/KlimaLoggPro.sdb"

/**
 * @brief The SensorData struct
 * represents data from one sensor
 */
struct SensorData
{
    double Temperature;
    bool TempValid;
    double Humidity;
    bool HumValid;
};

/**
 * @brief The Record struct
 * holds a timestamp and 9 sensor values (maximu sensor count)
 */
struct Record
{
    long Timestamp;
    bool TimeValid;
    SensorData SensorDatas[9];
};

/**
 * @brief The ResponseType enum
 * enum to determine the type of the response
 */
enum ResponseType
{
    INVALID = 0x00,
    RESPONSE_DATA_WRITTEN = 0x10,
    RESPONSE_GET_CONFIG = 0x20,
    RESPONSE_GET_CURRENT = 0x30,
    RESPONSE_GET_HISTORY = 0x40,
    RESPONE_REQUEST = 0x50
};

/**
 * @brief The TimeInterval enum
 */
enum TimeInterval
{
    SHORT = 900,    //15 min
    MEDIUM = 86400, // 24 hours
    LONG = 604800     //7 days
};

/**
 * @brief The TickSpacing enum
 * the spacing for the plot
 */
enum TickSpacing
{
    MINUTES = 300,  // 5 min
    HOURS = 14400,  // 4 hour
    DAYS = 86400   // 24 hours
};

#endif // DEFINITIONS

