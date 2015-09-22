#include "bitconverter.h"
#include <ctime>

#include <QDebug>
#include <QDateTime>

bool inline CheckOverflow(char data)
{
	char toTest = (data & 0x0F);
	return toTest >= 10 && toTest != 0x0F;
}


BitConverter::BitConverter()
{
}

bool BitConverter::ConvertTemperature(char data1, char data2, bool highByteFull, double *value)
{
	int tempOffset = 40;
	int dezi, one, ten = 0;
	if (highByteFull) // [x*1] [x*0,1] [-] [x*10]
	{
		dezi = (data2 & 0x0F);
		one = (data2 & 0xF0) >> 4;
		ten = (data1 & 0x0F);
	}
	else // [x*0,1] [-] [x*10] [x]
	{
		dezi = (data2 & 0xF0) >> 4;
		one = (data1 & 0x0F);
		ten = (data1 & 0xF0) >> 4;
	}

	//check overflow
	if (CheckOverflow(dezi) || CheckOverflow(one) || CheckOverflow(ten))
	{
		(*value) = 0;
		return false;
	}
	else
	{
		(*value) = ((ten * 10 + one + dezi * 0.1) - tempOffset);
		return true;
	}
}

bool BitConverter::ConvertHumidity(char data, double *value)
{
	int one, ten = 0;
	one = (data & 0x0F);
	ten = (data & 0xF0) >> 4;

	//check overflow
	if (CheckOverflow(one) || CheckOverflow(ten))
	{
		(*value) = 0;
		return false;
	}
	else
	{
		(*value) = (double)ten * 10 + one;
		return true;
	}
}


bool BitConverter::ConvertHistoryTimestamp(char *data, long *value)
{
	time_t rawTime;
	struct tm *timeinfo;
	int year10, year1, month10, month1, day10, day1, hours10, hours1, minute10, minute1;

	//get raw data from datapointer
	year10 = (data[0] & 0xF0) >> 4;
	year1 = (data[0] & 0x0F);
	month10 = (data[1] & 0xF0) >> 4;
	month1 = (data[1] & 0x0F);
	day10 = (data[2] & 0xF0) >> 4;
	day1 = (data[2] & 0x0F);
	hours10 = (data[3] & 0xF0) >> 4;
	hours1 = (data[3] & 0x0F);
	minute10 = (data[4] & 0xF0) >> 4;
	minute1 = (data[4] & 0x0F);

	//check overflow
	bool check = CheckOverflow(year10);
	check = check || CheckOverflow(year1);
	check = check || CheckOverflow(month10);
	check = check || CheckOverflow(month1);
	check = check || CheckOverflow(day10);
	check = check || CheckOverflow(day1);
	check = check || CheckOverflow(hours10);
	check = check || CheckOverflow(hours1);
	check = check || CheckOverflow(minute10);
	check = check || CheckOverflow(minute1);
	if (check)
	{
		(*value) = 0;
		return false;
	}

	//create timestructs
	rawTime = time(0);
	timeinfo = localtime(&rawTime);

	//fill in date values
	timeinfo->tm_year = (year10 * 10 + year1) + 100; //weewx year is from 2000, timeinfo is from 1900
    timeinfo->tm_mon = (month10 * 10 + month1) - 1; //weewx counts from 1 to 12 unix time from 0 - 11
	timeinfo->tm_mday = day10 * 10 + day1;
    timeinfo->tm_hour = hours10 * 10 + hours1;
	timeinfo->tm_min = minute10 * 10 + minute1;
	timeinfo->tm_sec = 0;

    qDebug() << "Year   : " << timeinfo->tm_year;
    qDebug() << "Month  : " << timeinfo->tm_mon;
    qDebug() << "Days   : " << timeinfo->tm_mday;
    qDebug() << "Hours  : " << timeinfo->tm_hour;
    qDebug() << "Minutes: " << timeinfo->tm_min;

    //convert time to unix timestamp
    (*value) = mktime(timeinfo);
    return true;
}


bool BitConverter::ConvertCurrentTimestamp(char *data, bool startOnHighNibble, long *value)
{
	time_t rawTime;
	struct tm *timeinfo;
	int year10, year1, month1, day10, day1, tim1, tim2, tim3, hours, minutes;

	//get raw data from datapointer
	if (startOnHighNibble)
	{
		year10 = (data[0] & 0xF0) >> 4;
		year1 = (data[0] & 0x0F);
		month1 = (data[1] & 0xF0) >> 4;
		day10 = (data[1] & 0x0F);
		day1 = (data[2] & 0xF0) >> 4;
		tim1 = (data[2] & 0x0F);
		tim2 = (data[3] & 0xF0) >> 4;
		tim3 = (data[3] & 0x0F);
	}
	else
	{
		year10 = (data[0] & 0x0F);
		year1 = (data[1] & 0xF0) >> 4;
		month1 = (data[1] & 0x0F);
		day10 = (data[2] & 0xF0) >> 4;
		day1 = (data[2] & 0x0F);
		tim1 = (data[3] & 0xF0) >> 4;
		tim2 = (data[3] & 0x0F);
		tim3 = (data[4] & 0xF0) >> 4;
	}
	//check overflow
	bool check = CheckOverflow(year10);
	check = check || CheckOverflow(year1);
	check = check || CheckOverflow(month1);
	check = check || CheckOverflow(day10);
	check = check || CheckOverflow(day1);
	check = check || CheckOverflow(tim1);
	check = check || CheckOverflow(tim2);
	check = check || CheckOverflow(tim3);
	if (check)
	{
		(*value) = 0;
		return false;
	}
	//do time conversion
	if (tim1 >= 10)
	{
		hours = tim1 + 10;
	}
	else
	{
		hours = tim1;
	}
	if (tim2 >= 10)
	{
		hours += 10;
		minutes = (tim2 - 10) * 10;
	}
	else
	{
		minutes = tim2 * 10;
	}
	minutes += tim3;


	//create timestructs
	rawTime = time(0);
	timeinfo = localtime(&rawTime);

	//fill in date values
	timeinfo->tm_year = (year10 * 10 + year1) + 100; //weewx year is from 2000, timeinfo is from 1900
    timeinfo->tm_mon = month1 -1;
	timeinfo->tm_mday = day10 * 10 + day1;
	timeinfo->tm_hour = hours;
	timeinfo->tm_min = minutes;
	timeinfo->tm_sec = 0;

	//convert time to unix timestamp
	(*value) = mktime(timeinfo);
	return true;
}

ResponseType BitConverter::GetResponseType(char *data, int size)
{
    if(size < 7)
    {
        return INVALID;
    }
    qDebug() << "Response is " << (int)data[6] ;
    return (ResponseType) data[6] ;
}

long BitConverter::GetThisIndex(char* usbframe)
{
    long thisIndex =
            (((((usbframe[13] << 8) | usbframe[14]) << 8)
            | usbframe[15]) - 0x070000) / 32;
    return thisIndex;
}

long BitConverter::GetLatestIndex(char* usbframe)
{
    long  latestIndex =
            (((((usbframe[10] << 8) | usbframe[11]) << 8) |
            usbframe[12]) - 0x070000) / 32;
    return latestIndex;
}


Record BitConverter::GetSensorValuesFromHistoryData(char* frame, int index)
{
	int offset_index[6] = { 156,128,100,72,44,16 };

	//get pointer for a single HistoryDataSet
	char* data = frame + offset_index[index];

    if ((unsigned char)data[27] == 0xEE)
	{
		//it's AlarmData -> not et supported
		Record rec;
		rec.TimeValid = false;
		return rec;
	}
	//index relativ to one HistoryDataSet
	int offset_h_map[9] = { 8,7,6,5,4,3,2,1,0 };
	int offset_t_map[9] = { 21,20,18,17,15,14,12,11,9 };


	Record record;
	long timestamp;
	record.TimeValid = BitConverter::ConvertHistoryTimestamp(data + 23, &timestamp);
	record.Timestamp = timestamp;

	for (int i = 0; i < 9; i++)
	{
		int offset_h = offset_h_map[i];
		int offset_t = offset_t_map[i];
		double value = 0;
        record.SensorDatas[i].TempValid = BitConverter::ConvertTemperature(data[offset_t], data[offset_t + 1], i % 2 == 0 , &value);
		record.SensorDatas[i].Temperature = value;
		record.SensorDatas[i].HumValid = BitConverter::ConvertHumidity(data[offset_h], &value);
		record.SensorDatas[i].Humidity = value;
	}

    qDebug() << "GetSensorValuesFromHistoryData record is: " << record.Timestamp <<
                ", "<< record.SensorDatas[0].Temperature<<"Grad, "<< record.SensorDatas[0].Humidity;
    qDebug() << "GetSensorValuesFromHistoryData record is: " << record.Timestamp <<
                ", "<< record.SensorDatas[2].Temperature<<"Grad, "<< record.SensorDatas[2].Humidity;
    qDebug() << "GetSensorValuesFromHistoryData record is: " << record.Timestamp <<
                ", "<< record.SensorDatas[3].Temperature<<"Grad, "<< record.SensorDatas[3].Humidity;
	return record;
}



Record BitConverter::GetSensorValuesFromCurrentData(char* frame)
{
    int sizeOFSensorData = 24;
    int start_h = 20;
    int start_t = 32;

	time_t rawTime;
	struct tm *timeinfo;
	rawTime = time(0);
	timeinfo = localtime(&rawTime);


    Record record;
    record.TimeValid = true;
    record.Timestamp = mktime(timeinfo);;

    for (int i = 0; i < 9; i++)
    {
        int offset_h = start_h + i * sizeOFSensorData;
        int offset_t = start_t + i * sizeOFSensorData;
        double value = 0;
		record.SensorDatas[i].TempValid = BitConverter::ConvertTemperature(frame[offset_t], frame[offset_t + 1], true, &value);
        record.SensorDatas[i].Temperature = value;
        qDebug() << "Temperature for "<< i << " is " <<  value;
        record.SensorDatas[i].HumValid = BitConverter::ConvertHumidity(frame[offset_h], &value);
        record.SensorDatas[i].Humidity = value;
        qDebug() << "Humidity for "<< i << " is " <<  value;
    }
    qDebug() << "GetSensorValuesFromCurrentData record is: " << record.Timestamp <<
                ", "<< record.SensorDatas[0].Temperature<<"Grad, "<< record.SensorDatas[0].Humidity;
    qDebug() << "GetSensorValuesFromCurrentData record is: " << record.Timestamp <<
                ", "<< record.SensorDatas[2].Temperature<<"Grad, "<< record.SensorDatas[2].Humidity;
    qDebug() << "GetSensorValuesFromCurrentData record is: " << record.Timestamp <<
                ", "<< record.SensorDatas[3].Temperature<<"Grad, "<< record.SensorDatas[3].Humidity;
    return record;
}
