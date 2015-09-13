#ifndef BITCONVERTER_H
#define BITCONVERTER_H

#include "bitconverter_global.h"

class BITCONVERTERSHARED_EXPORT BitConverter
{

public:
    BitConverter();
    static double ConvertTemperature(short data, bool highByteFull);
    static int ConvertHumidity(char data);
   // static int ConvertTimestamp(char )
};

#endif // BITCONVERTER_H
