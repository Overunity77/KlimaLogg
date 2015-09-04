#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>			/* kmalloc() */
#include <linux/usb.h>			/* USB stuff */
#include <linux/mutex.h>		/* mutexes */
#include <linux/ioctl.h>

#include <asm/uaccess.h>		/* copy_*_user */

#define DEBUG_LEVEL_DEBUG		0x1F
#define DEBUG_LEVEL_INFO		0x0F
#define DEBUG_LEVEL_WARN		0x07
#define DEBUG_LEVEL_ERROR		0x03
#define DEBUG_LEVEL_CRITICAL	0x01

#define DBG_DEBUG(fmt, args...) \
if ((debug_level & DEBUG_LEVEL_DEBUG) == DEBUG_LEVEL_DEBUG) \
	printk( KERN_DEBUG "[debug] %s(%d): " fmt "\n", \
			__FUNCTION__, __LINE__, ## args)
#define DBG_INFO(fmt, args...) \
if ((debug_level & DEBUG_LEVEL_INFO) == DEBUG_LEVEL_INFO) \
	printk( KERN_DEBUG "[info]  %s(%d): " fmt "\n", \
			__FUNCTION__, __LINE__, ## args)
#define DBG_WARN(fmt, args...) \
if ((debug_level & DEBUG_LEVEL_WARN) == DEBUG_LEVEL_WARN) \
	printk( KERN_DEBUG "[warn]  %s(%d): " fmt "\n", \
			__FUNCTION__, __LINE__, ## args)
#define DBG_ERR(fmt, args...) \
if ((debug_level & DEBUG_LEVEL_ERROR) == DEBUG_LEVEL_ERROR) \
	printk( KERN_DEBUG "[err]   %s(%d): " fmt "\n", \
			__FUNCTION__, __LINE__, ## args)
#define DBG_CRIT(fmt, args...) \
if ((debug_level & DEBUG_LEVEL_CRITICAL) == DEBUG_LEVEL_CRITICAL) \
	printk( KERN_DEBUG "[crit]  %s(%d): " fmt "\n", \
			__FUNCTION__, __LINE__, ## args)




MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniel Reimann");
MODULE_DESCRIPTION("KlimaLogg Pro USB Driver");
MODULE_VERSION("0.1");


#define KL_VENDOR_ID	0x6666
#define KL_PRODUCT_ID	0x5555

#define ML_CTRL_BUFFER_SIZE 	8
#define ML_CTRL_REQUEST_TYPE	0x21
#define ML_CTRL_REQUEST		0x09
#define ML_CTRL_VALUE		0x0
#define ML_CTRL_INDEX		0x0

#define ML_STOP			0x00
#define ML_UP			0x01
#define ML_DOWN			0x02

#define ML_LEFT			0x04
#define ML_RIGHT		0x08
#define ML_UP_LEFT		(ML_UP | ML_LEFT)
#define ML_DOWN_LEFT		(ML_DOWN | ML_LEFT)
#define ML_UP_RIGHT		(ML_UP | ML_RIGHT)
#define ML_DOWN_RIGHT		(ML_DOWN | ML_RIGHT)
#define ML_FIRE			0x10

#define ML_MAX_UP		0x80 		/* 80 00 00 00 00 00 00 00 */
#define ML_MAX_DOWN		0x40		/* 40 00 00 00 00 00 00 00 */
#define ML_MAX_LEFT		0x04		/* 00 04 00 00 00 00 00 00 */
#define ML_MAX_RIGHT		0x08 		/* 00 08 00 00 00 00 00 00 */

#ifdef CONFIG_USB_DYNAMIC_MINORS
#define ML_MINOR_BASE	0
#else
#define ML_MINOR_BASE	96
#endif


struct usb_kl {
    /* One structure for each connected device */
	struct usb_device 	*udev;
	struct usb_interface 	*interface;
	unsigned char		minor;
	char			serial_number[8];

	int			open_count;     /* Open count for this port */
	struct 			semaphore sem;	/* Locks this structure */
	spinlock_t		cmd_spinlock;	/* locks dev->command */

	char				*int_in_buffer;
	struct usb_endpoint_descriptor  *int_in_endpoint;
	struct urb 			*int_in_urb;
	int				int_in_running;

	char			*ctrl_buffer; /* 8 byte buffer for ctrl msg */
	struct urb		*ctrl_urb;
	struct usb_ctrlrequest  *ctrl_dr;     /* Setup packet information */
	int			correction_required;

	__u8			command;/* Last issued command */
};


static struct usb_device_id kl_table [] = {
	{ USB_DEVICE(KL_VENDOR_ID, KL_PRODUCT_ID) },
	{ }
};
MODULE_DEVICE_TABLE (usb, kl_table);


static int debug_level = DEBUG_LEVEL_DEBUG;
static int debug_trace = 1;
//static int debug_level = DEBUG_LEVEL_INFO;
//static int debug_trace = 0;

module_param(debug_level, int, S_IRUGO | S_IWUSR);
module_param(debug_trace, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level, "debug level (bitmask)");
MODULE_PARM_DESC(debug_trace, "enable function tracing");


/* Prevent races between open() and disconnect */
static DEFINE_MUTEX(disconnect_mutex);
static struct usb_driver kl_driver;

static inline void kl_debug_data(const char *function, int size,
		const unsigned char *data)
{
	int i;
	if ((debug_level & DEBUG_LEVEL_DEBUG) == DEBUG_LEVEL_DEBUG) {
		printk(KERN_DEBUG "[debug] %s: length = %d, data = ",
		       function, size);
		for (i = 0; i < size; ++i)
			printk("%.2x ", data[i]);
		printk("\n");
	}
}

static void kl_abort_transfers(struct usb_kl *dev)
{
	if (! dev) {
		DBG_ERR("dev is NULL");
		return;
	}

	if (! dev->udev) {
		DBG_ERR("udev is NULL");
		return;
	}

	if (dev->udev->state == USB_STATE_NOTATTACHED) {
		DBG_ERR("udev not attached");
		return;
	}

	/* Shutdown transfer */
	if (dev->int_in_running) {
		dev->int_in_running = 0;
		mb();
		if (dev->int_in_urb)
			usb_kill_urb(dev->int_in_urb);
	}

	if (dev->ctrl_urb)
		usb_kill_urb(dev->ctrl_urb);
}

static inline void kl_delete(struct usb_kl *dev)
{
	kl_abort_transfers(dev);

	/* Free data structures. */
	if (dev->int_in_urb)
		usb_free_urb(dev->int_in_urb);
	if (dev->ctrl_urb)
		usb_free_urb(dev->ctrl_urb);

	kfree(dev->int_in_buffer);
	kfree(dev->ctrl_buffer);
	kfree(dev->ctrl_dr);
	kfree(dev);
}

static void kl_ctrl_callback(struct urb *urb)
{
	struct usb_kl *dev = urb->context;
	dev->correction_required = 0;	/* TODO: do we need race protection? */
}

static void kl_int_in_callback(struct urb *urb)
{
	struct usb_kl *dev = urb->context;
	int retval;

	kl_debug_data(__FUNCTION__, urb->actual_length, urb->transfer_buffer);

	if (urb->status) {
		if (urb->status == -ENOENT ||
				urb->status == -ECONNRESET ||
				urb->status == -ESHUTDOWN) {
			return;
		} else {
			DBG_ERR("non-zero urb status (%d)", urb->status);
			goto resubmit; /* Maybe we can recover. */
		}
	}

	if (urb->actual_length > 0) {
		spin_lock(&dev->cmd_spinlock);

		if (dev->int_in_buffer[0] & ML_MAX_UP && dev->command & ML_UP) {
			dev->command &= ~ML_UP;
			dev->correction_required = 1;
		} else if (dev->int_in_buffer[0] & ML_MAX_DOWN &&
				dev->command & ML_DOWN) {
			dev->command &= ~ML_DOWN;
			dev->correction_required = 1;
		}

		if (dev->int_in_buffer[1] & ML_MAX_LEFT
		    && dev->command & ML_LEFT) {
			dev->command &= ~ML_LEFT;
			dev->correction_required = 1;
		} else if (dev->int_in_buffer[1] & ML_MAX_RIGHT
			   && dev->command & ML_RIGHT) {
			dev->command &= ~ML_RIGHT;
			dev->correction_required = 1;
		}


		if (dev->correction_required) {
			dev->ctrl_buffer[0] = dev->command;
			spin_unlock(&dev->cmd_spinlock);
			retval = usb_submit_urb(dev->ctrl_urb, GFP_ATOMIC);
			if (retval) {
				DBG_ERR("submitting correction control URB failed (%d)",
						retval);
			}
		} else {
			spin_unlock(&dev->cmd_spinlock);
		}
	}

resubmit:
	/* Resubmit if we're still running. */
	if (dev->int_in_running && dev->udev) {
		retval = usb_submit_urb(dev->int_in_urb, GFP_ATOMIC);
		if (retval) {
			DBG_ERR("resubmitting urb failed (%d)", retval);
			dev->int_in_running = 0;
		}
	}
}

static int kl_open(struct inode *inode, struct file *file)
{
    /* open syscall */
	struct usb_kl *dev = NULL;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;

	DBG_INFO("Open device");
	subminor = iminor(inode);

	mutex_lock(&disconnect_mutex);

	interface = usb_find_interface(&kl_driver, subminor);
	if (! interface) {
		DBG_ERR("can't find device for minor %d", subminor);
		retval = -ENODEV;
		goto exit;
	}

	dev = usb_get_intfdata(interface);
	if (! dev) {
		retval = -ENODEV;
		goto exit;
	}

	/* lock this device */
	if (down_interruptible (&dev->sem)) {
		DBG_ERR("sem down failed");
		retval = -ERESTARTSYS;
		goto exit;
	}

	/* Increment our usage count for the device. */
	++dev->open_count;
	if (dev->open_count > 1)
		DBG_DEBUG("open_count = %d", dev->open_count);

	/* Initialize interrupt URB. */
	usb_fill_int_urb(dev->int_in_urb, dev->udev,
			usb_rcvintpipe(dev->udev,
					   dev->int_in_endpoint->bEndpointAddress),
			dev->int_in_buffer,
			le16_to_cpu(dev->int_in_endpoint->wMaxPacketSize),
			kl_int_in_callback,
			dev,
			dev->int_in_endpoint->bInterval);

	dev->int_in_running = 1;
	mb();

	retval = usb_submit_urb(dev->int_in_urb, GFP_KERNEL);
	if (retval) {
		DBG_ERR("submitting int urb failed (%d)", retval);
		dev->int_in_running = 0;
		--dev->open_count;
		goto unlock_exit;
	}

	/* Save our object in the file's private structure. */
	file->private_data = dev;

unlock_exit:
	up(&dev->sem);

exit:
	mutex_unlock(&disconnect_mutex);
	return retval;
}




static int kl_release(struct inode *inode, struct file *file)
{
    /* close syscall */
	struct usb_kl *dev = NULL;
	int retval = 0;

	DBG_INFO("Release driver");
	dev = file->private_data;

	if (! dev) {
		DBG_ERR("dev is NULL");
		retval =  -ENODEV;
		goto exit;
	}

	/* Lock our device */
	if (down_interruptible(&dev->sem)) {
		retval = -ERESTARTSYS;
		goto exit;
	}

	if (dev->open_count <= 0) {
		DBG_ERR("device not opened");
		retval = -ENODEV;
		goto unlock_exit;
	}

	if (! dev->udev) {
		DBG_DEBUG("device unplugged before the file was released");
		up (&dev->sem);	/* Unlock here as kl_delete frees dev. */
		kl_delete(dev);
		goto exit;
	}

	if (dev->open_count > 1)
		DBG_DEBUG("open_count = %d", dev->open_count);

	kl_abort_transfers(dev);
	--dev->open_count;

unlock_exit:
	up(&dev->sem);

exit:
	return retval;
}

static ssize_t kl_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    /* write syscall */
	DBG_INFO("Send command");

	return 0;
}

static struct file_operations kl_fops = {
	.owner =	THIS_MODULE,
	.write =	kl_write,
	.open =		kl_open,
	.release =	kl_release,
};

static struct usb_class_driver kl_class = {
	.name = "kl%d",
	.fops = &kl_fops,
	.minor_base = ML_MINOR_BASE,
};


static int kl_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    /* called when a USB device is connected to the computer. */
	struct usb_device *udev = interface_to_usbdev(interface);
	struct usb_kl *dev = NULL;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int i, int_end_size;
	int retval = -ENODEV;
	DBG_INFO("Probe KlimaLogg Pro");

	if (! udev) {
		DBG_ERR("udev is NULL");
		goto exit;
	}

	dev = kzalloc(sizeof(struct usb_kl), GFP_KERNEL);
	if (! dev) {
		DBG_ERR("cannot allocate memory for struct usb_kl");
		retval = -ENOMEM;
		goto exit;
	}

	dev->command = ML_STOP;

	sema_init(&dev->sem, 1);
	spin_lock_init(&dev->cmd_spinlock);

	dev->udev = udev;
	dev->interface = interface;
	iface_desc = interface->cur_altsetting;

	/* Set up interrupt endpoint information. */
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK)
			 == USB_DIR_IN)
			&& ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
				== USB_ENDPOINT_XFER_INT))
			dev->int_in_endpoint = endpoint;

	}
	if (! dev->int_in_endpoint) {
		DBG_ERR("could not find interrupt in endpoint");
		goto error;
	}

	int_end_size = le16_to_cpu(dev->int_in_endpoint->wMaxPacketSize);

	dev->int_in_buffer = kmalloc(int_end_size, GFP_KERNEL);
	if (! dev->int_in_buffer) {
		DBG_ERR("could not allocate int_in_buffer");
		retval = -ENOMEM;
		goto error;
	}

	dev->int_in_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (! dev->int_in_urb) {
		DBG_ERR("could not allocate int_in_urb");
		retval = -ENOMEM;
		goto error;
	}

	/* Set up the control URB. */
	dev->ctrl_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (! dev->ctrl_urb) {
		DBG_ERR("could not allocate ctrl_urb");
		retval = -ENOMEM;
		goto error;
	}

	dev->ctrl_buffer = kzalloc(ML_CTRL_BUFFER_SIZE, GFP_KERNEL);
	if (! dev->ctrl_buffer) {
		DBG_ERR("could not allocate ctrl_buffer");
		retval = -ENOMEM;
		goto error;
	}

	dev->ctrl_dr = kmalloc(sizeof(struct usb_ctrlrequest), GFP_KERNEL);
	if (! dev->ctrl_dr) {
		DBG_ERR("could not allocate usb_ctrlrequest");
		retval = -ENOMEM;
		goto error;
	}
	dev->ctrl_dr->bRequestType = ML_CTRL_REQUEST_TYPE;
	dev->ctrl_dr->bRequest = ML_CTRL_REQUEST;
	dev->ctrl_dr->wValue = cpu_to_le16(ML_CTRL_VALUE);
	dev->ctrl_dr->wIndex = cpu_to_le16(ML_CTRL_INDEX);
	dev->ctrl_dr->wLength = cpu_to_le16(ML_CTRL_BUFFER_SIZE);

	usb_fill_control_urb(dev->ctrl_urb, dev->udev,
			usb_sndctrlpipe(dev->udev, 0),
			(unsigned char *)dev->ctrl_dr,
			dev->ctrl_buffer,
			ML_CTRL_BUFFER_SIZE,
			kl_ctrl_callback,
			dev);

	/* Retrieve a serial. */
	if (! usb_string(udev, udev->descriptor.iSerialNumber,
			 dev->serial_number, sizeof(dev->serial_number))) {
		DBG_ERR("could not retrieve serial number");
		goto error;
	}

	/* Save our data pointer in this interface device. */
	usb_set_intfdata(interface, dev);

	/* We can register the device now, as it is ready. */
	retval = usb_register_dev(interface, &kl_class);
	if (retval) {
		DBG_ERR("not able to get a minor for this device.");
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	dev->minor = interface->minor;

	DBG_INFO("USB KlimaLogg Pro now attached to /dev/kl%d",
			interface->minor - ML_MINOR_BASE);

exit:
	return retval;

error:
	kl_delete(dev);
	return retval;
}

static void kl_disconnect(struct usb_interface *interface)
{
    /* called when unplugging a USB device. */
	struct usb_kl *dev;
	int minor;

	mutex_lock(&disconnect_mutex);	/* Not interruptible */

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);

	down(&dev->sem); /* Not interruptible */

	minor = dev->minor;

	/* Give back our minor. */
	usb_deregister_dev(interface, &kl_class);

	/* If the device is not opened, then we clean up right now. */
	if (! dev->open_count) {
		up(&dev->sem);
		kl_delete(dev);
	} else {
		dev->udev = NULL;
		up(&dev->sem);
	}

	mutex_unlock(&disconnect_mutex);

	DBG_INFO("USB KlimaLogg Pro /dev/kl%d now disconnected",
			minor - ML_MINOR_BASE);
}

static struct usb_driver kl_driver = {
	.name = "klimalogg_driver",
	.id_table = kl_table,
	.probe = kl_probe,
	.disconnect = kl_disconnect,
};


static int __init usb_kl_init(void)
{
    /* called on module loading */
	int result;

	DBG_INFO("Register driver");
	//result = usb_register(&kl_driver);
	result = usb_register_driver(&kl_driver, THIS_MODULE, "kl_usb_drv");

	if (result) {
		DBG_ERR("registering driver failed");
	} else {
		DBG_INFO("driver registered successfully");
	}

	return result;
}


static void __exit usb_kl_exit(void)
{
    /* called on module unloading */
	usb_deregister(&kl_driver);
	DBG_INFO("module deregistered");
}




module_init(usb_kl_init);
module_exit(usb_kl_exit);
