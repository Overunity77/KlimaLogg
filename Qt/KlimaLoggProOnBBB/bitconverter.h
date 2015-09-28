#ifndef BITCONVERTER_H
#define BITCONVERTER_H

#include "definitions.h"


class BitConverter
{

public:
    BitConverter();

    /**
     * @brief convertTemperature
     * extracts the temperature value from to bytes
     * @param data1
     * first byte
     * @param data2
     * second byte
     * @param highByteFull
     * if true, the second byte contains the tens and one
     * @param value
     * the extracted tempererature
     * @return
     * true on success (no overflow and and valid value)
     */
    static bool convertTemperature(unsigned char data1, unsigned char data2, bool highByteFull, double *value);

    /**
     * @brief convertHumidity
     * extracts the humidity from one byte
     * @param data
     * the data stored the humidity
     * @param value
     * the extracted humidity
     * @return
     * true on success (no overflow and and valid value)
     */
    static bool convertHumidity(unsigned char data, double *value);

    /**
     * @brief convertHistoryTimestamp
     * converts a timestamp from a datapointer (history data)
     * @param data
     * the pointer where the timestamp is located
     * @param value
     * the extracted unix timestamp
     * @return
     * true on success (no overflow and and valid value)
     */
    static bool convertHistoryTimestamp(unsigned char *data, long *value);

    /**
     * @brief convertCurrentTimestamp
     * converts a timestamp from a datapointer (history data)
     * @param data
     * the pointer where the timestamp is located
     * @param startOnHighNibble
     * @param value
     * the extracted unix timestamp
     * @return
     * rue on success (no overflow and and valid value)
     */
    static bool convertCurrentTimestamp(unsigned char *data, bool startOnHighNibble, long *value);

    /**
     * @brief getResponseType
     * extracrs the response type from a usb fram
     * @param data
     * the datapointer to the data
     * @param size
     * the size of the data
     * @return
     * the corresponding response type
     */
    static ResponseType getResponseType(unsigned char *data, int size);

    /**
     * @brief getThisIndex
     * extracts the index of a history dataset
     * @param frame
     * the datapointer
     * @return
     * the index of this history dataset
     */
    static int getThisIndex(unsigned char *frame);

    /**
     * @brief getLatestIndex
     * extracts the lates read index from the history dataset
     * @param frame
     * the datapointer
     * @return
     * the latest index wich should read from
     */
    static int getLatestIndex(unsigned char *frame);

    /**
     * @brief getSensorValuesFromHistoryData
     * extracts a whole Record from a history data frame
     * @param frame
     * @param index
     * a index from 0 to 5 -> 6 history data sets where stored in one usb frame
     * @return
     * a record containing timestamp and values from 9 sensors
     */
    static Record getSensorValuesFromHistoryData(unsigned char *frame, int index);

    /**
     * @brief getSensorValuesFromCurrentData
     * extracts a whole Record from a current Data frame
     * @param frame
     * @return
     * a record containing timestamp (generated) and values from 9 sensors
     */
    static Record getSensorValuesFromCurrentData(unsigned char *frame);
};

#endif // BITCONVERTER_H

