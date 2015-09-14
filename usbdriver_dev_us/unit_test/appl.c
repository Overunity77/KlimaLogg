#include <stdio.h>

/* defined in Grad Celsius */
#define TEMPERATURE_OFFSET 40

int main(int argc, char **argv)
{
	FILE *fd_klimalogg = NULL;
	char data[100];

	int retValue = 0;
	
	int counter = 0;

	fd_klimalogg = fopen("/dev/usb_test", "rb");
	if (!fd_klimalogg) {
		printf("kann /dev/usb_test nicht oeffnen\n");
		return -1;
	}

	while (counter < 20) {
		retValue = (int)fread(data, 8, 1, fd_klimalogg);
		//printf("retValue= %d\n", retValue);
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
