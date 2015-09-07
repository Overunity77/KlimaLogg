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

struct usb_device *dev;
static DEFINE_MUTEX(ulock);

/* Entspricht getState aus kl.py */
static ssize_t usb_test_read(struct file *instanz, char *buffer,
			     size_t count, loff_t * ofs)
{
	int ret;
	char pbuf[20];
/*	__u16 *status = kmalloc(sizeof(__u16), GFP_KERNEL); */
	unsigned char *data = kcalloc(10, 1, GFP_KERNEL);

	if (!data)
		goto read_out;

	/*printk("usb_test: read\n"); */
	mutex_lock(&ulock);	/* Jetzt nicht disconnecten... */
	printk("read vor = %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	ret =
	    usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), USB_REQ_CLEAR_FEATURE,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
			    0x3de, 0, data, 10, 5 * HZ);

	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		count = -EIO;
		goto read_out;
	}

	printk("read nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	
	ssleep(1);
	
	snprintf(pbuf, sizeof(pbuf), "data=%d\n", *data);
	if (strlen(pbuf) < count)
		count = strlen(pbuf);
	count -= copy_to_user(buffer, pbuf, count);

read_out:
	mutex_unlock(&ulock);
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
  
static int usb_test_open2(struct inode *devicefile, struct file *instanz)
{
	int ret;
	int count = 0;

	unsigned char *data = kmalloc(0x15, GFP_KERNEL);
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

	printk("open1 nach= %x %x %x %x\n", data[0], data[1], data[2], data[3]);
	printk("open2 nach= %x %x %x %x\n", data[4], data[5], data[6], data[7]);
	printk("open3 nach= %x %x %x %x\n", data[8], data[9], data[10], data[11]);
	printk("open4 nach= %x %x %x %x\n", data[12], data[13], data[14], data[15]);
	printk("open5 nach= %x %x %x %x\n", data[16], data[17], data[18], data[19]);

read_out:
	mutex_unlock(&ulock);
	kfree(data);
	return count;	
  
  
}
  
static struct file_operations usb_fops = {
	.owner = THIS_MODULE,
	.open = usb_test_open2,
	.read = usb_test_read2,
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
