/************************************************************************/
/* Quellcode zum Buch                                                   */
/*                     Linux Treiber entwickeln                         */
/* (3. Auflage) erschienen im dpunkt.verlag                             */
/* Copyright (c) 2004-2011 Juergen Quade und Eva-Katharina Kunst        */
/*                                                                      */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         */
/* GNU General Public License for more details.                         */
/*                                                                      */
/************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/fs.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define USB_VENDOR_ID 0x6666
#define USB_DEVICE_ID 0x5555

#define FREQUENCY_EU 868300000

#define AX5051REGISTER_REVISION       0x0
#define AX5051REGISTER_SCRATCH        0x1
#define AX5051REGISTER_POWERMODE      0x2
#define AX5051REGISTER_XTALOSC        0x3
#define AX5051REGISTER_FIFOCTRL       0x4
#define AX5051REGISTER_FIFODATA       0x5
#define AX5051REGISTER_IRQMASK        0x6
#define AX5051REGISTER_IFMODE 0x8
#define AX5051REGISTER_PINCFG1 0x0C
#define AX5051REGISTER_PINCFG2 0x0D
#define AX5051REGISTER_MODULATION 0x10
#define AX5051REGISTER_ENCODING 0x11
#define AX5051REGISTER_FRAMING 0x12
#define AX5051REGISTER_CRCINIT3 0x14
#define AX5051REGISTER_CRCINIT2 0x15
#define AX5051REGISTER_CRCINIT1 0x16
#define AX5051REGISTER_CRCINIT0 0x17
#define AX5051REGISTER_FREQ3 0x20
#define AX5051REGISTER_FREQ2 0x21
#define AX5051REGISTER_FREQ1 0x22
#define AX5051REGISTER_FREQ0 0x23
#define AX5051REGISTER_FSKDEV2 0x25
#define AX5051REGISTER_FSKDEV1 0x26
#define AX5051REGISTER_FSKDEV0 0x27
#define AX5051REGISTER_IFFREQHI 0x28
#define AX5051REGISTER_IFFREQLO 0x29
#define AX5051REGISTER_PLLLOOP 0x2C
#define AX5051REGISTER_PLLRANGING 0x2D
#define AX5051REGISTER_PLLRNGCLK 0x2E
#define AX5051REGISTER_TXPWR 0x30
#define AX5051REGISTER_TXRATEHI 0x31
#define AX5051REGISTER_TXRATEMID 0x32
#define AX5051REGISTER_TXRATELO 0x33
#define AX5051REGISTER_MODMISC 0x34
#define AX5051REGISTER_FIFOCONTROL2 0x37
#define AX5051REGISTER_ADCMISC 0x38
#define AX5051REGISTER_AGCTARGET 0x39
#define AX5051REGISTER_AGCATTACK 0x3A
#define AX5051REGISTER_AGCDECAY 0x3B
#define AX5051REGISTER_AGCCOUNTER 0x3C
#define AX5051REGISTER_CICDEC 0x3F
#define AX5051REGISTER_DATARATEHI 0x40
#define AX5051REGISTER_DATARATELO 0x41
#define AX5051REGISTER_TMGGAINHI 0x42
#define AX5051REGISTER_TMGGAINLO 0x43
#define AX5051REGISTER_PHASEGAIN 0x44
#define AX5051REGISTER_FREQGAIN 0x45
#define AX5051REGISTER_FREQGAIN2 0x46
#define AX5051REGISTER_AMPLGAIN 0x47
#define AX5051REGISTER_TRKFREQHI 0x4C
#define AX5051REGISTER_TRKFREQLO 0x4D
#define AX5051REGISTER_XTALCAP 0x4F
#define AX5051REGISTER_SPAREOUT 0x60
#define AX5051REGISTER_TESTOBS 0x68
#define AX5051REGISTER_APEOVER 0x70
#define AX5051REGISTER_TMMUX 0x71
#define AX5051REGISTER_PLLVCOI 0x72
#define AX5051REGISTER_PLLCPEN 0x73
#define AX5051REGISTER_PLLRNGMISC 0x74
#define AX5051REGISTER_AGCMANUAL 0x78
#define AX5051REGISTER_ADCDCLEVEL 0x79
#define AX5051REGISTER_RFMISC 0x7A
#define AX5051REGISTER_TXDRIVER 0x7B
#define AX5051REGISTER_REF 0x7C
#define AX5051REGISTER_RXMISC 0x7D

#define AX5051REGISTER_IFMODE_DATA      0x00
#define AX5051REGISTER_MODULATION_DATA  0x41
#define AX5051REGISTER_ENCODING_DATA    0x07
#define AX5051REGISTER_FRAMING_DATA     0x84
#define AX5051REGISTER_CRCINIT3_DATA    0xff
#define AX5051REGISTER_CRCINIT2_DATA    0xff
#define AX5051REGISTER_CRCINIT1_DATA    0xff
#define AX5051REGISTER_CRCINIT0_DATA    0xff
#define AX5051REGISTER_FREQ3_DATA       0x38
#define AX5051REGISTER_FREQ2_DATA       0x90
#define AX5051REGISTER_FREQ1_DATA       0x00
#define AX5051REGISTER_FREQ0_DATA       0x01
#define AX5051REGISTER_PLLLOOP_DATA     0x1d
#define AX5051REGISTER_PLLRANGING_DATA  0x08
#define AX5051REGISTER_PLLRNGCLK_DATA   0x03
#define AX5051REGISTER_MODMISC_DATA     0x03
#define AX5051REGISTER_SPAREOUT_DATA    0x00
#define AX5051REGISTER_TESTOBS_DATA     0x00
#define AX5051REGISTER_APEOVER_DATA     0x00
#define AX5051REGISTER_TMMUX_DATA       0x00
#define AX5051REGISTER_PLLVCOI_DATA     0x01
#define AX5051REGISTER_PLLCPEN_DATA     0x01
#define AX5051REGISTER_RFMISC_DATA      0xb0
#define AX5051REGISTER_REF_DATA         0x23
#define AX5051REGISTER_IFFREQHI_DATA    0x20
#define AX5051REGISTER_IFFREQLO_DATA    0x00
#define AX5051REGISTER_ADCMISC_DATA     0x01
#define AX5051REGISTER_AGCTARGET_DATA   0x0e
#define AX5051REGISTER_AGCATTACK_DATA   0x11
#define AX5051REGISTER_AGCDECAY_DATA    0x0e
#define AX5051REGISTER_CICDEC_DATA      0x3f
#define AX5051REGISTER_DATARATEHI_DATA  0x19
#define AX5051REGISTER_DATARATELO_DATA  0x66
#define AX5051REGISTER_TMGGAINHI_DATA   0x01
#define AX5051REGISTER_TMGGAINLO_DATA   0x96
#define AX5051REGISTER_PHASEGAIN_DATA   0x03
#define AX5051REGISTER_FREQGAIN_DATA    0x04
#define AX5051REGISTER_FREQGAIN2_DATA   0x0a
#define AX5051REGISTER_AMPLGAIN_DATA    0x06
#define AX5051REGISTER_AGCMANUAL_DATA   0x00
#define AX5051REGISTER_ADCDCLEVEL_DATA  0x10
#define AX5051REGISTER_RXMISC_DATA      0x35
#define AX5051REGISTER_FSKDEV2_DATA     0x00
#define AX5051REGISTER_FSKDEV1_DATA     0x31
#define AX5051REGISTER_FSKDEV0_DATA     0x27
#define AX5051REGISTER_TXPWR_DATA       0x03
#define AX5051REGISTER_TXRATEHI_DATA    0x00
#define AX5051REGISTER_TXRATEMID_DATA   0x51
#define AX5051REGISTER_TXRATELO_DATA    0xec
#define AX5051REGISTER_TXDRIVER_DATA    0x88

struct usb_device *dev;
static DEFINE_MUTEX(ulock);

/* Entspricht getState aus kl.py */
static ssize_t usb_test_read(struct file *instanz, char *buffer,
			     size_t count, loff_t * ofs)
{
	int ret;
	char pbuf[0x111];
/*	__u16 *status = kmalloc(sizeof(__u16), GFP_KERNEL); */
	unsigned char *data = kcalloc(10, 1, GFP_KERNEL);

	unsigned char *rawdata = kcalloc(0x111, 1, GFP_KERNEL);

	if (!data)
		goto read_out;

	printk("usb_test: read\n");
	mutex_lock(&ulock);	/* Jetzt nicht disconnecten... */
	printk("read vor = %02x %02x %02x %02x\n", data[0], data[1], data[2],
	       data[3]);
	ret =
	    usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), USB_REQ_CLEAR_FEATURE,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
			    0x3de, 0, data, 10, 5 * HZ);

	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		count = -EIO;
		goto read_out;
	}

	printk("read nach= %02x %02x %02x %02x\n", data[0], data[1], data[2],
	       data[3]);
	/* def getFrame(self): in kl.py */
	if (data[1] == 0x16) {
		printk("Success!\n");
		ret =
		    usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
				    USB_REQ_CLEAR_FEATURE,
				    USB_TYPE_CLASS | USB_RECIP_INTERFACE |
				    USB_DIR_IN, 0x3d6, 0, rawdata, 0x111, 5 * HZ);
		if (ret < 0) {
			printk("Error in getFrame Nr: %d\n", ret);
			count = -EIO;
			goto read_out;
		}
		
		
		
	}

	ssleep(1);

	snprintf(pbuf, sizeof(pbuf), "data=%d\n", *rawdata);
	if (strlen(pbuf) < count)
		count = strlen(pbuf);
	count -= copy_to_user(buffer, pbuf, count);

read_out:
	mutex_unlock(&ulock);
	kfree(rawdata);
	kfree(data);
	return count;
}

/*
  def doRFSetup(self):
        self.hid.execute(5)
        self.hid.setPreamblePattern(0xaa)
        self.hid.setState(0)
        time.sleep(1)
        self.hid.setRX()

        self.hid.setPreamblePattern(0xaa)
        self.hid.setState(0x1e)
        time.sleep(1)
        self.hid.setRX()
        self.setSleep(0.075, 0.005)
*/
static int usb_test_open(struct inode *devicefile, struct file *instanz)
{
	int ret;

	unsigned char *data = kcalloc(0x15, 1, GFP_KERNEL);
//      noch auf 0 initialisieren

	if (!data)
		goto read_out;

	printk("usb_test_open start\n");
	mutex_lock(&ulock);	/* Jetzt nicht disconnecten... */

	/* Entspricht execute aus kl.py */
	data[0] = 0xd9;
	data[1] = 5;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d9, 0, data, 0x0f, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	/* Entspricht setPreamblePattern aus kl.py */
	data[0] = 0xd8;
	data[1] = 0xaa;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d8, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	/* Entspricht setState(0) aus kl.py */
	data[0] = 0xd7;
	data[1] = 0;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d7, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	ssleep(1);

	/* Entspricht setRX aus kl.py */
	data[0] = 0xd0;
	data[1] = 0;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d0, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	/* Entspricht setPreamblePattern aus kl.py */
	data[0] = 0xd8;
	data[1] = 0xaa;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d8, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	/* Entspricht setState(0x1e) aus kl.py */
	data[0] = 0xd7;
	data[1] = 0x1e;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d7, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	ssleep(1);

	/* Entspricht setRX aus kl.py */
	data[0] = 0xd0;
	data[1] = 0;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d0, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	printk("usb_test_open end\n");

read_out:
	mutex_unlock(&ulock);
	kfree(data);
	return 0;
}

static ssize_t usb_test_read2(struct file *instanz, char *buffer,
			      size_t count, loff_t * ofs)
{

	return count;
}

static void writeReg(unsigned char *buf, int regAddr, int data)
{
	int ret = 0;
	buf[0] = 0xf0;
	buf[1] = regAddr & 0x7F;	// IFMODE_register
	buf[2] = 0x01;
	buf[3] = data;		// AX5051RegisterNames.IFMODE_data
	buf[4] = 0x00;

	ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			      USB_REQ_SET_CONFIGURATION,
			      USB_TYPE_CLASS | USB_RECIP_INTERFACE |
			      USB_DIR_OUT, 0x3f0, 0, buf, 5, 5 * HZ);
	if (ret < 0) {
		printk("Error in writeReg Nr: %d\n", ret);
	}
	printk("writeReg nach= %02x %02x %02x %02x %02x\n", buf[0], buf[1],
	       buf[2], buf[3], buf[4]);
	return;
}

static int usb_test_open2(struct inode *devicefile, struct file *instanz)
{
	int ret;
	int count = 0;
	int freq = FREQUENCY_EU;
	int freq3;
	int freq2;
	int freq1;
	int freq0;

	int freqVal = (freq / 16000000.0 * 16777216.0);
	int corVal;
	int tid;

	unsigned char *data = kmalloc(0x15, GFP_KERNEL);
	unsigned char *new_data = kcalloc(4, 1, GFP_KERNEL);
	unsigned char *data2;
	unsigned char *register_buf;

	printk("freqVal %d\n", freqVal);
	data[0] = 0xdd;
	data[1] = 0x0a;
	data[2] = 0x01;
	data[3] = 0xF5;
	data[4] = data[5] = data[6] = data[7] = 0xcc;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0xcc;
	data[14] = data[15] = 0xcc;

	if (!data)
		goto read_out;

	/*printk("usb_test: read\n"); */
	mutex_lock(&ulock);	/* Jetzt nicht disconnecten... */
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3dd, 0, data, 0x0f, 5 * HZ);

	if (ret < 0) {
		printk("Error Open 1 Nr: %d\n", ret);
		count = -EIO;
		goto read_out;
	}

	ret =
	    usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), USB_REQ_CLEAR_FEATURE,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
			    0x3dc, 0, data, 0x15, 5 * HZ);

	if (ret < 0) {
		printk("Error Open 2 Nr: %d\n", ret);
		count = -EIO;
		goto read_out;
	}

	new_data[0] = data[4];
	new_data[1] = data[5];
	new_data[2] = data[6];
	new_data[3] = data[7];
	printk("open1 nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	printk("open2 nach= %x %x %x %x\n", data[4], data[5], data[6], data[7]);
	printk("open3 nach= %x %x %x %x\n", data[8], data[9], data[10],
	       data[11]);
	printk("open4 nach= %x %x %x %x\n", data[12], data[13], data[14],
	       data[15]);
	printk("open5 nach= %x %x %x %x\n", data[16], data[17], data[18],
	       data[19]);

	corVal = new_data[0] << 8;
	corVal |= new_data[1];
	corVal <<= 8;
	corVal |= new_data[2];
	corVal <<= 8;
	corVal |= new_data[3];

	printk("frequency correction: %d (0x%x)\n", corVal, corVal);

	freqVal += corVal;
	if (!(freqVal % 2))
		freqVal += 1;

	printk("adjusted frequency: %d (0x%x)\n", freqVal, freqVal);

	freq3 = (freqVal >> 24) & 0xFF;
	freq2 = (freqVal >> 16) & 0xFF;
	freq1 = (freqVal >> 8) & 0xFF;
	freq0 = (freqVal >> 0) & 0xFF;

	printk("frequency registers: %x %x %x %x\n", freq3, freq2, freq1,
	       freq0);

	data2 = kmalloc(0x15, GFP_KERNEL);
	data2[0] = 0xdd;
	data2[1] = 0x0a;
	data2[2] = 0x01;
	data2[3] = 0xF9;
	data2[4] = data2[5] = data2[6] = data2[7] = 0xcc;
	data2[8] = data2[9] = data2[10] = data2[11] = data2[12] = data2[13] =
	    0xcc;
	data2[14] = data2[15] = 0xcc;

	if (!data2)
		goto read_out;

	/*printk("usb_test: read\n"); */

	printk("open vor = %x %x %x %x\n", data2[0], data2[1], data2[2],
	       data2[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3dd, 0, data2, 0x0f, 5 * HZ);

	if (ret < 0) {
		printk("Error Open 1 Nr: %d\n", ret);
		count = -EIO;
		goto read_out;
	}

	ret =
	    usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), USB_REQ_CLEAR_FEATURE,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
			    0x3dc, 0, data2, 0x15, 5 * HZ);

	if (ret < 0) {
		printk("Error Open 2 Nr: %d\n", ret);
		count = -EIO;
		goto read_out;
	}

	new_data[0] = data2[4];
	new_data[1] = data2[5];
	new_data[2] = data2[6];
	new_data[3] = data2[7];
	new_data[4] = data2[8];
	new_data[5] = data2[9];
	new_data[6] = data2[10];
	printk("open1 nach= %02x %02x %02x %02x\n", data2[0], data2[1],
	       data2[2], data2[3]);
	printk("open2 nach= %02x %02x %02x %02x\n", data2[4], data2[5],
	       data2[6], data2[7]);
	printk("open3 nach= %02x %02x %02x %02x\n", data2[8], data2[9],
	       data2[10], data2[11]);
	printk("open4 nach= %02x %02x %02x %02x\n", data2[12], data2[13],
	       data2[14], data2[15]);
	printk("open5 nach= %02x %02x %02x %02x\n", data2[16], data2[17],
	       data2[18], data2[19]);

	tid = (new_data[5] << 8) + new_data[6];

	printk("transceiver identifier: %d (0x%04x)\n", tid, tid);

	register_buf = kcalloc(5, 1, GFP_KERNEL);

	if (!register_buf)
		goto read_out;

	writeReg(register_buf, AX5051REGISTER_ADCDCLEVEL,
		 AX5051REGISTER_ADCDCLEVEL_DATA);
	writeReg(register_buf, AX5051REGISTER_ADCMISC,
		 AX5051REGISTER_ADCMISC_DATA);

	writeReg(register_buf, AX5051REGISTER_AGCATTACK,
		 AX5051REGISTER_AGCATTACK_DATA);

	writeReg(register_buf, AX5051REGISTER_AGCDECAY,
		 AX5051REGISTER_AGCDECAY_DATA);

	writeReg(register_buf, AX5051REGISTER_AGCMANUAL,
		 AX5051REGISTER_AGCMANUAL_DATA);

	writeReg(register_buf, AX5051REGISTER_AGCTARGET,
		 AX5051REGISTER_AGCTARGET_DATA);

	writeReg(register_buf, AX5051REGISTER_AMPLGAIN,
		 AX5051REGISTER_AMPLGAIN_DATA);

	writeReg(register_buf, AX5051REGISTER_APEOVER,
		 AX5051REGISTER_APEOVER_DATA);

	writeReg(register_buf, AX5051REGISTER_CICDEC,
		 AX5051REGISTER_CICDEC_DATA);

	writeReg(register_buf, AX5051REGISTER_CRCINIT0,
		 AX5051REGISTER_CRCINIT0_DATA);

	writeReg(register_buf, AX5051REGISTER_CRCINIT1,
		 AX5051REGISTER_CRCINIT1_DATA);

	writeReg(register_buf, AX5051REGISTER_CRCINIT2,
		 AX5051REGISTER_CRCINIT2_DATA);

	writeReg(register_buf, AX5051REGISTER_CRCINIT3,
		 AX5051REGISTER_CRCINIT3_DATA);

	writeReg(register_buf, AX5051REGISTER_DATARATEHI,
		 AX5051REGISTER_DATARATEHI_DATA);

	writeReg(register_buf, AX5051REGISTER_DATARATELO,
		 AX5051REGISTER_DATARATELO_DATA);

	writeReg(register_buf, AX5051REGISTER_ENCODING,
		 AX5051REGISTER_ENCODING_DATA);

	writeReg(register_buf, AX5051REGISTER_FRAMING,
		 AX5051REGISTER_FRAMING_DATA);

	writeReg(register_buf, AX5051REGISTER_FREQ0, freq0);	// AX5051REGISTER_FREQ0_DATA);
	printk("writeReg freq0 written\n");
	writeReg(register_buf, AX5051REGISTER_FREQ1, freq1);	// AX5051REGISTER_FREQ1_DATA);
	printk("writeReg freq1 written\n");
	writeReg(register_buf, AX5051REGISTER_FREQ2, freq2);	// AX5051REGISTER_FREQ2_DATA);
	printk("writeReg freq2 written\n");
	writeReg(register_buf, AX5051REGISTER_FREQ3, freq3);	//AX5051REGISTER_FREQ3_DATA);
	printk("writeReg freq3 written\n");
	writeReg(register_buf, AX5051REGISTER_FREQGAIN,
		 AX5051REGISTER_FREQGAIN_DATA);

	writeReg(register_buf, AX5051REGISTER_FREQGAIN2,
		 AX5051REGISTER_FREQGAIN2_DATA);

	writeReg(register_buf, AX5051REGISTER_FSKDEV0,
		 AX5051REGISTER_FSKDEV0_DATA);

	writeReg(register_buf, AX5051REGISTER_FSKDEV1,
		 AX5051REGISTER_FSKDEV1_DATA);

	writeReg(register_buf, AX5051REGISTER_FSKDEV2,
		 AX5051REGISTER_FSKDEV2_DATA);

	writeReg(register_buf, AX5051REGISTER_IFFREQHI,
		 AX5051REGISTER_IFFREQHI_DATA);

	writeReg(register_buf, AX5051REGISTER_IFFREQLO,
		 AX5051REGISTER_IFFREQLO_DATA);

	writeReg(register_buf, AX5051REGISTER_IFMODE,
		 AX5051REGISTER_IFMODE_DATA);

	writeReg(register_buf, AX5051REGISTER_MODMISC,
		 AX5051REGISTER_MODMISC_DATA);

	writeReg(register_buf, AX5051REGISTER_MODULATION,
		 AX5051REGISTER_MODULATION_DATA);

	writeReg(register_buf, AX5051REGISTER_PHASEGAIN,
		 AX5051REGISTER_PHASEGAIN_DATA);

	writeReg(register_buf, AX5051REGISTER_PINCFG1, AX5051REGISTER_PINCFG2);

	writeReg(register_buf, AX5051REGISTER_PLLCPEN,
		 AX5051REGISTER_PLLCPEN_DATA);

	writeReg(register_buf, AX5051REGISTER_PLLLOOP,
		 AX5051REGISTER_PLLLOOP_DATA);

	writeReg(register_buf, AX5051REGISTER_PLLRANGING,
		 AX5051REGISTER_PLLRANGING_DATA);

	writeReg(register_buf, AX5051REGISTER_PLLRNGCLK,
		 AX5051REGISTER_PLLRNGCLK_DATA);

	writeReg(register_buf, AX5051REGISTER_PLLVCOI,
		 AX5051REGISTER_PLLVCOI_DATA);

	writeReg(register_buf, AX5051REGISTER_REF, AX5051REGISTER_REF_DATA);

	writeReg(register_buf, AX5051REGISTER_RFMISC,
		 AX5051REGISTER_RFMISC_DATA);

	writeReg(register_buf, AX5051REGISTER_RXMISC,
		 AX5051REGISTER_RXMISC_DATA);

	writeReg(register_buf, AX5051REGISTER_SPAREOUT,
		 AX5051REGISTER_SPAREOUT_DATA);

	writeReg(register_buf, AX5051REGISTER_TESTOBS,
		 AX5051REGISTER_TESTOBS_DATA);

	writeReg(register_buf, AX5051REGISTER_TMGGAINHI,
		 AX5051REGISTER_TMGGAINHI_DATA);

	writeReg(register_buf, AX5051REGISTER_TMGGAINLO,
		 AX5051REGISTER_TMGGAINLO_DATA);

	writeReg(register_buf, AX5051REGISTER_TMMUX, AX5051REGISTER_TMMUX_DATA);

	writeReg(register_buf, AX5051REGISTER_TXDRIVER,
		 AX5051REGISTER_TXDRIVER_DATA);

	writeReg(register_buf, AX5051REGISTER_TXPWR, AX5051REGISTER_TXPWR_DATA);

	writeReg(register_buf, AX5051REGISTER_TXRATEHI,
		 AX5051REGISTER_TXRATEHI_DATA);

	writeReg(register_buf, AX5051REGISTER_TXRATELO,
		 AX5051REGISTER_TXRATELO_DATA);

	writeReg(register_buf, AX5051REGISTER_TXRATEMID,
		 AX5051REGISTER_TXRATEMID_DATA);

	/* Entspricht execute aus kl.py */
	data[0] = 0xd9;
	data[1] = 5;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d9, 0, data, 0x0f, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	/* Entspricht setPreamblePattern aus kl.py */
	data[0] = 0xd8;
	data[1] = 0xaa;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d8, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	/* Entspricht setState(0) aus kl.py */
	data[0] = 0xd7;
	data[1] = 0;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d7, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	ssleep(1);

	/* Entspricht setRX aus kl.py */
	data[0] = 0xd0;
	data[1] = 0;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d0, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	/* Entspricht setPreamblePattern aus kl.py */
	data[0] = 0xd8;
	data[1] = 0xaa;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d8, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	/* Entspricht setState(0x1e) aus kl.py */
	data[0] = 0xd7;
	data[1] = 0x1e;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d7, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	ssleep(1);

	/* Entspricht setRX aus kl.py */
	data[0] = 0xd0;
	data[1] = 0;
	data[2] = data[3] = data[4] = data[5] = data[6] = data[7] = 0;
	data[8] = data[9] = data[10] = data[11] = data[12] = data[13] = 0;
	data[14] = data[15] = data[16] = data[17] = data[18] = data[19] = 0;
	data[20] = data[21] = 0;
	printk("open vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    USB_REQ_SET_CONFIGURATION,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			    0x3d0, 0, data, 0x15, 5 * HZ);
	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		goto read_out;
	}
	printk("open nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);

	printk("usb_test_open end\n");

read_out:
	mutex_unlock(&ulock);
	kfree(register_buf);
	kfree(data2);
	kfree(new_data);
	kfree(data);
	return count;

}

static struct file_operations usb_fops = {
	.owner = THIS_MODULE,
	.open = usb_test_open2,
	.read = usb_test_read,
};

static struct usb_device_id usbid[] = {
	{USB_DEVICE(USB_VENDOR_ID, USB_DEVICE_ID),},
	/*      {USB_DEVICE_INTERFACE_CLASS(USB_VENDOR_ID, USB_DEVICE_ID, USB_CLASS_VENDOR_SPEC) }, */
	{}			/* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, usbid);

struct usb_class_driver class_descr = {
	.name = "usb_test",
	.fops = &usb_fops,
	.minor_base = 16,
};

static int usb_test_probe(struct usb_interface *interface,
			  const struct usb_device_id *id)
{
	printk("usb_test: probe\n");
	dev = interface_to_usbdev(interface);
	printk("usb_test: 0x%4.4x|0x%4.4x, if=%p\n", dev->descriptor.idVendor,
	       dev->descriptor.idProduct, interface);
	if (dev->descriptor.idVendor == USB_VENDOR_ID
	    && dev->descriptor.idProduct == USB_DEVICE_ID) {
		if (usb_register_dev(interface, &class_descr)) {
			return -EIO;
		}
		printk("got minor= %d\n", interface->minor);
		return 0;
	}
	return -ENODEV;
}

static void usb_test_disconnect(struct usb_interface *iface)
{
	printk("usb_test: disconnect\n");
	/* Ausstehende Auftraege muessen abgearbeitet sein... */
	mutex_lock(&ulock);
	usb_deregister_dev(iface, &class_descr);
	mutex_unlock(&ulock);
}

static struct usb_driver usb_test = {
	.name = "usb_test",
	.id_table = usbid,
	.probe = usb_test_probe,
	.disconnect = usb_test_disconnect,
};

static int __init usb_test_init(void)
{
	printk("usb_test: init");
	if (usb_register(&usb_test)) {
		printk("usb_test: unable to register usb driver\n");
		return -EIO;
	}
	return 0;
}

static void __exit usb_test_exit(void)
{
	printk("usb_test: exit\n");
	usb_deregister(&usb_test);
}

module_init(usb_test_init);
module_exit(usb_test_exit);
MODULE_LICENSE("GPL");
/* MODULE_ALIAS("usb:v6666p5555d0100dc03dsc00dpFFic03isc00ip00in00"); */
