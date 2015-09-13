#include "bitconverter.h"


BitConverter::BitConverter()
{
}

double BitConverter::ConvertTemperature(short data, bool highByteFull)
{
    int dez,one,ten = 0;
    if(highByteFull) // [x*1] [x*0,1] [-] [x*10]
    {
        dez = (data & 0x0F00) >> 8;
        one = (data & 0xF000) >> 12;
        ten = (data & 0x000F);
    }else // [x*0,1] [-] [x*10] [x]
    {
        dez = (data & 0xF000) >> 12;
        one = (data & 0x000F);
        ten = (data & 0x00F0) >> 4;
    }
    return ten * 10 + one + dez * 0.1;
}

int BitConverter::ConvertHumidity(char data)
{
    int one, ten = 0;
    one = (data & 0x0F);
    ten = (data & 0xF0) >> 4;
    return ten * 10 + one;
}
