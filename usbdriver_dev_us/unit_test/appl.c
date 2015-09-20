#include <stdio.h>
#include <unistd.h>

/* defined in Grad Celsius */
#define TEMPERATURE_OFFSET 40

//int main(int argc, char **argv)
//{
//	FILE *fd_klimalogg = NULL;
//	char data[100];
//
//	int retValue = 0;
//
//	int counter = 0;
//
//	fd_klimalogg = fopen("/dev/usb_test", "rb");
//	if (!fd_klimalogg) {
//		printf("kann /dev/usb_test nicht oeffnen\n");
//		return -1;
//	}
//
//	//while (counter < 20) {
//	while (retValue == 0) {
//		retValue = (int)fread(data, 8, 1, fd_klimalogg);
//		//printf("retValue= %d\n", retValue);
//		if (retValue) {
//			printf("Days   : %02d\n", data[0]);
//			printf("Month  : %02d\n", data[1]);
//			printf("Year   : %04d\n", (int)data[2] + 2000);
//			printf("Hours  : %02d\n", data[3]);
//			printf("Minutes: %02d\n", data[4]);
//			printf("Humidity : %02d\n", data[5]);
//			printf("Temp : %02d.%1d\n", (int)data[6]- TEMPERATURE_OFFSET, data[7]);
//			counter = counter +1;
//			printf("Record Nr : %d\n\n", counter);
//		}
//	}
//	fclose(fd_klimalogg);
//	return 0;
//}

int main(int argc, char **argv)
{
	FILE *fd_klimalogg = NULL;
	char data[100];

	int retValue = 0;

	int counter = 0;

	fd_klimalogg = fopen("/dev/kl1", "rb");
	if (!fd_klimalogg) {
		printf("kann /dev/kl1 nicht oeffnen\n");
		return -1;
	}

//	usleep(75000);

	while (counter < 20) {
	//while (retValue == 0) {
		retValue = (int)fread(data, 8, 1, fd_klimalogg);
		printf("retValue= %d\n", retValue);
		if (retValue) {
			printf("Days   : %02d\n", data[0]);
			printf("Month  : %02d\n", data[1]);
			printf("Year   : %04d\n", (int)data[2] + 2000);
			printf("Hours  : %02d\n", data[3]);
			printf("Minutes: %02d\n", data[4]);
			printf("Humidity : %02d\n", data[5]);
			printf("Temp : %02d.%1d\n", (int)data[6]- TEMPERATURE_OFFSET, data[7]);
			counter = counter +1;
			printf("Record Nr : %d\n\n", counter);
		}
	}
	fclose(fd_klimalogg);
	return 0;
}

//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.160433] writeReg nach= f0 79 01 10 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.160538] writeReg nach= f0 38 01 01 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.160665] writeReg nach= f0 3a 01 11 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.160796] writeReg nach= f0 3b 01 0e 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.160926] writeReg nach= f0 78 01 00 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.161033] writeReg nach= f0 39 01 0e 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.161138] writeReg nach= f0 47 01 06 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.161243] writeReg nach= f0 70 01 00 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.161348] writeReg nach= f0 3f 01 3f 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.161456] writeReg nach= f0 17 01 ff 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.161561] writeReg nach= f0 16 01 ff 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.161700] writeReg nach= f0 15 01 ff 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.161848] writeReg nach= f0 14 01 ff 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.161976] writeReg nach= f0 40 01 19 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162081] writeReg nach= f0 41 01 66 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162212] writeReg nach= f0 11 01 07 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162341] writeReg nach= f0 12 01 84 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162473] writeReg nach= f0 23 01 ed 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162474] writeReg freq0 written
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162582] writeReg nach= f0 22 01 03 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162583] writeReg freq1 written
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162700] writeReg nach= f0 21 01 46 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162701] writeReg freq2 written
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162830] writeReg nach= f0 20 01 36 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162831] writeReg freq3 written
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.162961] writeReg nach= f0 45 01 04 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.163065] writeReg nach= f0 46 01 0a 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.163173] writeReg nach= f0 27 01 27 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.163279] writeReg nach= f0 26 01 31 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.163385] writeReg nach= f0 25 01 00 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.163515] writeReg nach= f0 28 01 20 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.163637] writeReg nach= f0 29 01 00 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.163768] writeReg nach= f0 08 01 00 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.163898] writeReg nach= f0 34 01 03 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.164002] writeReg nach= f0 10 01 41 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.164106] writeReg nach= f0 44 01 03 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.164221] writeReg nach= f0 0c 01 0d 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.164358] writeReg nach= f0 73 01 01 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.164489] writeReg nach= f0 2c 01 1d 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.164637] writeReg nach= f0 2d 01 08 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.164767] writeReg nach= f0 2e 01 03 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.164898] writeReg nach= f0 72 01 01 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.165028] writeReg nach= f0 7c 01 23 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.165158] writeReg nach= f0 7a 01 b0 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.165265] writeReg nach= f0 7d 01 35 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.165398] writeReg nach= f0 60 01 00 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.165529] writeReg nach= f0 68 01 00 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.165646] writeReg nach= f0 42 01 01 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.165783] writeReg nach= f0 43 01 96 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.165902] writeReg nach= f0 71 01 00 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.166037] writeReg nach= f0 7b 01 88 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.166211] writeReg nach= f0 30 01 03 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.166327] writeReg nach= f0 31 01 00 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.166460] writeReg nach= f0 33 01 ec 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.166564] writeReg nach= f0 32 01 51 00
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.166566] open vor = d9 5 0 0
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.167950] open nach= d9 5 0 0
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.167952] open vor = d8 aa 0 0
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.168079] open nach= d8 aa 0 0
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.168080] open vor = d7 0 0 0
//Sep 15 01:17:13 izu-N550JK kernel: [ 7493.168753] open nach= d7 0 0 0
//Sep 15 01:17:14 izu-N550JK kernel: [ 7494.170848] open vor = d0 0 0 0
//Sep 15 01:17:14 izu-N550JK kernel: [ 7494.170997] open nach= d0 0 0 0
//Sep 15 01:17:14 izu-N550JK kernel: [ 7494.170999] open vor = d8 aa 0 0
//Sep 15 01:17:14 izu-N550JK kernel: [ 7494.171136] open nach= d8 aa 0 0
//Sep 15 01:17:14 izu-N550JK kernel: [ 7494.171138] open vor = d7 1e 0 0
//Sep 15 01:17:14 izu-N550JK kernel: [ 7494.171300] open nach= d7 1e 0 0
//Sep 15 01:17:15 izu-N550JK kernel: [ 7495.175932] open vor = d0 0 0 0
//Sep 15 01:17:15 izu-N550JK kernel: [ 7495.176078] open nach= d0 0 0 0
//Sep 15 01:17:15 izu-N550JK kernel: [ 7495.176080] usb_test_open end
//Sep 15 01:17:15 izu-N550JK kernel: [ 7495.176095] Count is: 4096
//Sep 15 01:17:15 izu-N550JK kernel: [ 7495.176097] usb_test: read
//Sep 15 01:17:15 izu-N550JK kernel: [ 7495.176309] read nach= de 15 00 00
//Sep 15 01:17:16 izu-N550JK kernel: [ 7496.181032] to_copy: 0
//Sep 15 01:17:16 izu-N550JK kernel: [ 7496.181035] not_copied: 0
//Sep 15 01:17:16 izu-N550JK kernel: [ 7496.181036] Return at the end is: 0
//
