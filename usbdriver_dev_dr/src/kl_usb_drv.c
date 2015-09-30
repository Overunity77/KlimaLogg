#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/usb.h>			/* USB stuff */
#include <linux/mutex.h>		/* mutexes */
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <asm/uaccess.h>		/* copy_*_user */
#include "kl_usb_drv.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniel Reimann");
MODULE_DESCRIPTION("KlimaLogg Pro USB Driver");
MODULE_VERSION("0.1");


#define DEBUG_LEVEL_DEBUG		0x1F
#define DEBUG_LEVEL_INFO		0x0F
#define DEBUG_LEVEL_WARN		0x07
#define DEBUG_LEVEL_ERROR		0x03
#define DEBUG_LEVEL_CRITICAL		0x01

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


#ifdef CONFIG_USB_DYNAMIC_MINORS
#define ML_MINOR_BASE	0
#else
#define ML_MINOR_BASE	96
#endif




/* init driver module parameter */
static int debug_level = DEBUG_LEVEL_DEBUG;
static int debug_trace = 1;
//static int debug_level = DEBUG_LEVEL_INFO;
//static int debug_trace = 0;


/* Prevent races between open() and disconnect */
static DEFINE_MUTEX(disconnect_mutex);

/* forward declaration */
static struct usb_driver kl_driver;

/* function prototypes */
static int setup_instance(struct kl_usb *hw);
static int kl_probe(struct usb_interface *intf,
		    const struct usb_device_id *id);
static void kl_disconnect(struct usb_interface *intf);
static ssize_t kl_write(struct file *file,
		        const char __user *user_buf,
			size_t count,
			loff_t *ppos);
static ssize_t kl_read(struct file *file,
		       char *buffer,
		       size_t count,
		       loff_t * ppos);
static int kl_open(struct inode *inode,
		   struct file *file);
static int kl_release(struct inode *inode,
		      struct file *file);
static int __init usb_kl_init(void);
static void __exit usb_kl_exit(void);


/* usb device id table */
static struct usb_device_id kl_table[] = { {
		USB_DEVICE(KL_VENDOR_ID, KL_PRODUCT_ID) }, { } };
MODULE_DEVICE_TABLE(usb, kl_table);


/* driver module parameters */
module_param(debug_level, int, S_IRUGO | S_IWUSR);
module_param(debug_trace, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level, "debug level (bitmask)");
MODULE_PARM_DESC(debug_trace, "enable function tracing");

/* driver module init/exit */
module_init(usb_kl_init);
module_exit(usb_kl_exit);

static struct usb_driver kl_driver = { .name 	   = "klimalogg_driver",
				       .id_table   = kl_table,
				       .probe 	   = kl_probe,
				       .disconnect = kl_disconnect, };

static struct file_operations kl_fops = { .owner   = THIS_MODULE,
					  .write   = kl_write,
					  .read    = kl_read,
					  .open    = kl_open,
					  .release = kl_release };

static struct usb_class_driver kl_class = { .name = "kl%d",
		                            .fops = &kl_fops,
					    .minor_base = ML_MINOR_BASE };


/*
 * write content of given unsigned char buffer to debug output
 */
static inline void kl_debug_data(const char *function, int size,
		const unsigned char *data)
{
	int i;
	if ((debug_level & DEBUG_LEVEL_DEBUG) == DEBUG_LEVEL_DEBUG)
	{
		printk(KERN_DEBUG "[debug] %s: length = %d, data = ", function,
				size);
		for (i = 0; i < size; ++i)
			printk("%.2x ", data[i]);
		printk("\n");
	}
}


/*
 * start next background transfer for control channel
 */
static void ctrl_start_transfer(struct kl_usb *hw)
{
	int ret;

	if (hw->ctrl_cnt) {
		DBG_INFO("start next usb ctrl transfer");

		hw->ctrl_urb->pipe = hw->usb_ctrl_buff[hw->ctrl_out_idx].pipe;
		hw->ctrl_urb->setup_packet = (u_char *)&hw->usb_ctrl_buff[hw->ctrl_out_idx].ctrlrequest;
		hw->ctrl_urb->transfer_buffer = hw->usb_ctrl_buff[hw->ctrl_out_idx].buf;
		hw->ctrl_urb->transfer_buffer_length = hw->usb_ctrl_buff[hw->ctrl_out_idx].buflen;

		ret = usb_submit_urb(hw->ctrl_urb, GFP_ATOMIC);
		if (ret)
			DBG_ERR("failed to submit usb urb: %s (%d)", symbolic(urb_errlist, ret), ret);
	}
	else
	{
		DBG_INFO("usb ctrl transfer queue is empty");
	}
}

/*
 * queue a control transfer write request
 * the param: void* data will be de-allocated when the urb completes
 */
static int write_usb_ctrl(struct kl_usb *hw, __u8 reportId, __u16 len, void* data)
{
	struct usb_ctrl_buf *buf;

	spin_lock(&hw->ctrl_lock);
	if (hw->ctrl_cnt >= KL_USB_CTRL_BUFSIZE) {
		DBG_ERR("usb control buffer full!");
		spin_unlock(&hw->ctrl_lock);
		return 1;
	}
	buf = &hw->usb_ctrl_buff[hw->ctrl_in_idx];

	buf->ctrlrequest.bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT;
	buf->ctrlrequest.bRequest = USB_REQ_SET_CONFIGURATION;
	buf->ctrlrequest.wValue = cpu_to_le16((USB_HID_FEATURE_REPORT << 8) | reportId); /* convert to little-endian */
	buf->ctrlrequest.wIndex = cpu_to_le16(0);
	buf->ctrlrequest.wLength = cpu_to_le16(len);

	buf->buf = data;
	buf->buflen = len;

	buf->pipe = hw->ctrl_out_pipe;

	if (++hw->ctrl_in_idx >= KL_USB_CTRL_BUFSIZE)
		hw->ctrl_in_idx = 0;
	if (++hw->ctrl_cnt == 1) 	/* start transfer if it's the first item in the queue */
		ctrl_start_transfer(hw);
	spin_unlock(&hw->ctrl_lock);

	return 0;
}

/*
 * queue a control transfer write request
 * the param: void* data will be de-allocated when the urb completes
 */
static int read_usb_ctrl(struct kl_usb *hw, __u8 reportId, __u16 len, void* data)
{
	struct usb_ctrl_buf *buf;

	spin_lock(&hw->ctrl_lock);
	if (hw->ctrl_cnt >= KL_USB_CTRL_BUFSIZE) {
		DBG_ERR("usb control buffer full!");
		spin_unlock(&hw->ctrl_lock);
		return 1;
	}
	buf = &hw->usb_ctrl_buff[hw->ctrl_in_idx];

	buf->ctrlrequest.bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN;
	buf->ctrlrequest.bRequest = USB_REQ_CLEAR_FEATURE;
	buf->ctrlrequest.wValue = cpu_to_le16((USB_HID_FEATURE_REPORT << 8) | reportId); /* convert to little-endian */
	buf->ctrlrequest.wIndex = cpu_to_le16(0);
	buf->ctrlrequest.wLength = cpu_to_le16(len);

	buf->buf = data;
	buf->buflen = len;

	buf->pipe = hw->ctrl_out_pipe;

	if (++hw->ctrl_in_idx >= KL_USB_CTRL_BUFSIZE)
		hw->ctrl_in_idx = 0;
	if (++hw->ctrl_cnt == 1)	/* start transfer if it's the first item in the queue */
		ctrl_start_transfer(hw);
	spin_unlock(&hw->ctrl_lock);

	return 0;
}



/*
 * queue a control transfer request to write ax5051
 * chip register using CTRL request queue
 */
static int write_reg(struct kl_usb *hw, struct klusb_ax5015_register_list *reg)
{
	struct usb_ctrl_buf *buf;

	reg->buf = kcalloc(KL_LEN_WRITE_REG, 1, GFP_KERNEL); 	/* reg->buf will be de-allocated when the urb completes */

	if (!reg->buf)
		return -ENOMEM;

	spin_lock(&hw->ctrl_lock);
	if (hw->ctrl_cnt >= KL_USB_CTRL_BUFSIZE) {
		DBG_ERR("usb control buffer full!");
		spin_unlock(&hw->ctrl_lock);
		kfree(reg->buf);
		return 1;	// TODO handle this in caller (make retry)
	}
	buf = &hw->usb_ctrl_buff[hw->ctrl_in_idx];

	buf->ctrlrequest.bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT;
	buf->ctrlrequest.bRequest = USB_REQ_SET_CONFIGURATION;
	buf->ctrlrequest.wValue = cpu_to_le16((USB_HID_FEATURE_REPORT << 8) |
			                       KL_MSG_WRITE_REG); /* convert to little-endian */
	buf->ctrlrequest.wIndex = cpu_to_le16(0);
	buf->ctrlrequest.wLength = cpu_to_le16(KL_LEN_WRITE_REG);

	reg->buf[0] = 0xf0;
	reg->buf[1] = reg->addr & 0x7f;
	reg->buf[2] = 0x01;
	reg->buf[3] = reg->value;
	reg->buf[4] = 0x00;

	buf->buf = reg->buf;
	buf->buflen = KL_LEN_WRITE_REG;

	buf->pipe = hw->ctrl_out_pipe;

//	kl_debug_data(__FUNCTION__, buf->buflen, buf->buf);

	if (++hw->ctrl_in_idx >= KL_USB_CTRL_BUFSIZE)
		hw->ctrl_in_idx = 0;
	if (++hw->ctrl_cnt == 1)	/* start transfer if it's the first item in the queue */
		ctrl_start_transfer(hw);
	spin_unlock(&hw->ctrl_lock);

	return 0;
}


/*
 * control completion routine handling background control cmds
 */
static void ctrl_complete(struct urb *urb)
{
	struct kl_usb *hw = (struct kl_usb *) urb->context;

	kl_debug_data(__FUNCTION__, urb->actual_length, urb->transfer_buffer);

	/* release memory of completed urb buffer */
	kfree(urb->transfer_buffer);

	spin_lock(&hw->ctrl_lock);

	if (hw->ctrl_cnt) {
		hw->ctrl_cnt--;	/* decrement actual count */
		if (++hw->ctrl_out_idx >= KL_USB_CTRL_BUFSIZE)
			hw->ctrl_out_idx = 0;	/* pointer wrap */

		ctrl_start_transfer(hw); /* start next transfer */
	}

	spin_unlock(&hw->ctrl_lock);
}


/*
 * receive completion routine for rx interrupt endpoint
 */
static void rx_int_complete(struct urb *urb)
{
	struct kl_usb *hw = (struct kl_usb *) urb->context;
	int retval = 0;

	if(!hw) {
		DBG_ERR("hw is null");
		DBG_INFO("urb->status: %s (%d)", symbolic(urb_errlist, urb->status), urb->status);
		return;
	}

	if (urb->status) {
		DBG_WARN("urb->status: %s (%d)", symbolic(urb_errlist, urb->status), urb->status);
		if (urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN) {
			return;
		} else {
			DBG_ERR("non-zero urb status: %s (%d)",symbolic(urb_errlist, urb->status), urb->status);
			goto resubmit; /* Maybe we can recover. */
		}
	}

	atomic_set(&hw->logger_state, hw->rx_int_buffer[1]);

	/* USB data log for RX INT in */
//	DBG_DEBUG("RX INT length(%d)", urb->actual_length);
//	kl_debug_data(__FUNCTION__, urb->actual_length, hw->rx_int_buffer);

resubmit:
	/* Resubmit if we're still running. */
	if (hw->rx_int_running && hw->dev) {
		retval = usb_submit_urb(hw->rx_int_urb, GFP_ATOMIC);
		if (retval) {
			DBG_ERR("resubmitting urb failed: %s (%d)", symbolic(urb_errlist, retval), retval);
			hw->rx_int_running = 0;
		}
	}
}


static void abort_rx_int_transfer(struct kl_usb *hw)
{
	if(!hw) {
		DBG_ERR("hw is NULL");
		return;
	}

	if(!hw->dev) {
		DBG_ERR("dev is NULL");
		return;
	}

	if(!hw->dev->state == USB_STATE_NOTATTACHED) {
		DBG_ERR("dev not attached");
		return;
	}

	/* Shutdown transfer */
	if(hw->rx_int_running) {
		hw->rx_int_running = 0;
		mb();
		if(hw->rx_int_urb)
			usb_kill_urb(hw->rx_int_urb);
	}
}

/*
 * start the rx interrupt endpoint transfer
 */
static int start_rx_int_transfer(struct kl_usb *hw)
{
	int retval = 0;

	DBG_INFO("starting rx interrupt transfer");

	hw->rx_int_urb = usb_alloc_urb(0, GFP_KERNEL);

	if(!hw->rx_int_urb)
	{
		return -ENOMEM;
	}

	usb_fill_int_urb(hw->rx_int_urb,
			 hw->dev,
			 hw->rx_int_in_pipe,
			 hw->rx_int_buffer,
			 le16_to_cpu(hw->rx_int_endpoint_desc->wMaxPacketSize),
			 (usb_complete_t)rx_int_complete,
			 hw,
			 hw->rx_int_endpoint_desc->bInterval);

	hw->rx_int_running = 1;
	mb();

	retval = usb_submit_urb(hw->rx_int_urb, GFP_KERNEL);

	if (retval) {
		DBG_ERR("submitting rx interrupt urb failed: %s (%d)", symbolic(urb_errlist, retval), retval);
		hw->rx_int_running = 0;
		kfree(hw->rx_int_urb);
		return retval;
	}

	return retval;
}

/*
 * read from the USB transceiver configuration flash
 */
static int readConfigFlash(struct kl_usb *hw, struct usb_read_config_flash *config) {
	int ret = 0;
	unsigned char *writebuf;

	writebuf = kmalloc(KL_LEN_READ_CONFIG_FLASH_OUT, GFP_KERNEL);

	if(!writebuf) {
		DBG_ERR("no memory");
		return -ENOMEM;
	}

	memset(writebuf, 0xcc, KL_LEN_READ_CONFIG_FLASH_OUT);

	writebuf[0]  = 0xdd;
	writebuf[1]  = 0x0a;
	writebuf[2]  = (config->addr >> 8) & 0xff;
	writebuf[3]  = (config->addr >> 0) & 0xff;

	DBG_INFO("write buffer");
	kl_debug_data(__FUNCTION__, KL_LEN_READ_CONFIG_FLASH_OUT, writebuf);

	/* if successful, returns the number of bytes transferred */
	ret = usb_control_msg(hw->dev, hw->ctrl_out_pipe, USB_REQ_SET_CONFIGURATION,
			      USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			      (USB_HID_FEATURE_REPORT << 8) | KL_MSG_READ_CONFIG_FLASH_OUT,
			      0, writebuf, KL_LEN_READ_CONFIG_FLASH_OUT, KL_USB_CTRL_TIMEOUT
			     );
	if (ret < 0) {
		DBG_ERR("could not prepare to read config flash: %s (%d)", symbolic(urb_errlist, ret), ret);
		kfree(writebuf);
		return ret;
	}

	/* if successful, returns the number of bytes transferred */
	ret = usb_control_msg(hw->dev, hw->ctrl_in_pipe, USB_REQ_CLEAR_FEATURE,
			      USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
			      (USB_HID_FEATURE_REPORT << 8) | KL_MSG_READ_CONFIG_FLASH_IN,
			      0, config->readBuf, config->buflen, KL_USB_CTRL_TIMEOUT
			     );

	if (ret < 0) {
		DBG_ERR("Could not read from config flash: %s (%d)", symbolic(urb_errlist, ret), ret);
		kfree(writebuf);
		return ret;
	}

	kfree(writebuf);
	return 0;
}

/*
 * setup RF-Communication
 */
static int doRfSetup(struct kl_usb *hw)
{
	int ret = 0;

	/* execute(5) */
	hw->rf_setup_buffers.buf_execute = kcalloc(KL_LEN_EXECUTE, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_execute) {
		ret = -ENOMEM;
		DBG_ERR("execute(5) failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	hw->rf_setup_buffers.buf_execute[0] = KL_MSG_EXECUTE;
	hw->rf_setup_buffers.buf_execute[1] = 0x05;
	ret = write_usb_ctrl(hw,
		       KL_MSG_EXECUTE,
		       KL_LEN_EXECUTE,
		       hw->rf_setup_buffers.buf_execute);

	if (ret < 0) {
		DBG_ERR("execute(5) failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	/* setPreamblePattern(0xaa) */
	hw->rf_setup_buffers.buf_preamble_first = kcalloc(KL_LEN_SET_PREAMBLE_PATTERN, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_preamble_first) {
		ret = -ENOMEM;
		DBG_ERR("first setPreamblePattern(0xaa) failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	hw->rf_setup_buffers.buf_preamble_first[0] = KL_MSG_SET_PREAMBLE_PATTERN;
	hw->rf_setup_buffers.buf_preamble_first[1] = 0xaa;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_PREAMBLE_PATTERN,
		       KL_LEN_SET_PREAMBLE_PATTERN,
		       hw->rf_setup_buffers.buf_preamble_first);

	if (ret < 0) {
		DBG_ERR("first setPreamblePattern(0xaa) failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	/* setState(0) */
	hw->rf_setup_buffers.buf_setstate_first = kcalloc(KL_LEN_SET_STATE, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_setstate_first) {
		ret = -ENOMEM;
		DBG_ERR("first setState(0) failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	hw->rf_setup_buffers.buf_setstate_first[0] = KL_MSG_SET_STATE;
	hw->rf_setup_buffers.buf_setstate_first[1] = 0x00;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_STATE,
		       KL_LEN_SET_STATE,
		       hw->rf_setup_buffers.buf_setstate_first);

	if (ret < 0) {
		DBG_ERR("first setState(0) failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	/* The pyton driver needed to sleep here for 1s */
	// msleep(1000);

	/* setRx() */
	hw->rf_setup_buffers.buf_setRx_first = kcalloc(KL_LEN_SET_RX, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_setRx_first) {
		ret = -ENOMEM;
		DBG_ERR("first setRx() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	hw->rf_setup_buffers.buf_setRx_first[0] = KL_MSG_SET_RX;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_RX,
		       KL_LEN_SET_RX,
		       hw->rf_setup_buffers.buf_setRx_first);

	if (ret < 0) {
		DBG_ERR("first setRx() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	/* setPreamblePattern(0xaa) */
	hw->rf_setup_buffers.buf_preamble_second = kcalloc(KL_LEN_SET_PREAMBLE_PATTERN, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_preamble_second) {
		ret = -ENOMEM;
		DBG_ERR("second setPreamblePattern(0xaa) failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	hw->rf_setup_buffers.buf_preamble_second[0] = KL_MSG_SET_PREAMBLE_PATTERN;
	hw->rf_setup_buffers.buf_preamble_second[1] = 0xaa;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_PREAMBLE_PATTERN,
		       KL_LEN_SET_PREAMBLE_PATTERN,
		       hw->rf_setup_buffers.buf_preamble_second);

	if (ret < 0) {
		DBG_ERR("second setPreamblePattern(0xaa) failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	/* setState(0x1e) */
	hw->rf_setup_buffers.buf_setstate_second = kcalloc(KL_LEN_SET_STATE, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_setstate_second) {
		ret = -ENOMEM;
		DBG_ERR("second setState(0x1e) failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	hw->rf_setup_buffers.buf_setstate_second[0] = KL_MSG_SET_STATE;
	hw->rf_setup_buffers.buf_setstate_second[1] = 0x1e;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_STATE,
		       KL_LEN_SET_STATE,
		       hw->rf_setup_buffers.buf_setstate_second);

	if (ret < 0) {
		DBG_ERR("second setState(0x1e) failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	/* The pyton driver needed to sleep here for 1s */
	//msleep(1000);

	/* setRx() */
	hw->rf_setup_buffers.buf_setRx_second = kcalloc(KL_LEN_SET_RX, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_setRx_second) {
		ret = -ENOMEM;
		DBG_ERR("second setRx() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

	hw->rf_setup_buffers.buf_setRx_second[0] = KL_MSG_SET_RX;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_RX,
		       KL_LEN_SET_RX,
		       hw->rf_setup_buffers.buf_setRx_second);

	if (ret < 0) {
		DBG_ERR("second setRx() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		goto out;
	}

out:
	return ret;
}

/*
 * set value of a ax5051 register
 */
static void setRegisterValue(__u8 addr, int value)
{
	int i, countReg;

	countReg = sizeof(ax5051_reglist) / sizeof(ax5051_reglist[0]);

	for (i = 0; i < countReg; i++) {
		if (ax5051_reglist[i].addr == addr) {
			ax5051_reglist[i].value = value;
			break;
		}
	}
}

/*
 * initialize the USB transceiver
 */
static int initTranseiver(struct kl_usb *hw)
{
	struct usb_read_config_flash freqCorrection;
	struct usb_read_config_flash transceiverId;
	unsigned int corVal = 0;
	unsigned int freqVal = 0;
	unsigned char *readbuf;

	int ret;
	int countReg, i;

	DBG_INFO("%s", __func__);

	readbuf = kzalloc(KL_LEN_READ_CONFIG_FLASH_IN, GFP_KERNEL);

	if (!readbuf) {
		DBG_ERR("no memory");
		return -ENOMEM;
	}

	/* read frequency correction */
	freqCorrection.addr = 0x1f5;
	freqCorrection.buflen = KL_LEN_READ_CONFIG_FLASH_IN;
	freqCorrection.readBuf = readbuf;

	ret = readConfigFlash(hw, &freqCorrection);
	if(ret) {
		DBG_ERR("read frequency correction failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		kfree(readbuf);
		return ret;
	}
	DBG_INFO("freqCorrection read buffer:");
	kl_debug_data(__FUNCTION__, freqCorrection.buflen, freqCorrection.readBuf);

	corVal = freqCorrection.readBuf[4] << 8;
	corVal |= freqCorrection.readBuf[5];
	corVal <<= 8;
	corVal |= freqCorrection.readBuf[6];
	corVal <<= 8;
	corVal |= freqCorrection.readBuf[7];

	DBG_INFO("frequency correction: %u (0x%08x)", corVal, corVal);

	freqVal = KL_FREQ_VAL + corVal;

	if (!(freqVal % 2))
		freqVal += 1;

	DBG_INFO("adjusted frequency: %u (0x%08x)", freqVal, freqVal);


	/* save adjusted frequency in ax5051 register data */
	setRegisterValue(AX5051REGISTER_FREQ3, (freqVal >> 24) & 0xff);
	setRegisterValue(AX5051REGISTER_FREQ2, (freqVal >> 16) & 0xff);
	setRegisterValue(AX5051REGISTER_FREQ1, (freqVal >> 8) & 0xff);
	setRegisterValue(AX5051REGISTER_FREQ0, (freqVal >> 0) & 0xff);

	/* read transceiver identifier */
	transceiverId.addr = 0x1f9;
	transceiverId.buflen = KL_LEN_READ_CONFIG_FLASH_IN;
	transceiverId.readBuf = readbuf;

	ret = readConfigFlash(hw, &transceiverId);
	if(ret) {
		DBG_ERR("read transceiver identifier failed: %s (%d)", symbolic(urb_errlist, ret), ret);
		kfree(readbuf);
		return ret;
	}
	DBG_INFO("transceiverId read buffer:");
	kl_debug_data(__FUNCTION__, transceiverId.buflen, transceiverId.readBuf);

	hw->transceiver_id = (transceiverId.readBuf[9] << 8) + transceiverId.readBuf[10];

	DBG_INFO("transceiver identifier: %u (0x%04x)", hw->transceiver_id, hw->transceiver_id);

	/* write ax5051 registers */
	countReg = sizeof(ax5051_reglist) / sizeof(ax5051_reglist[0]);
	DBG_INFO("number of registers: %d", countReg);
	for (i = 0; i < countReg; i++) {
		if (ax5051_reglist[i].value != -1) {
			// DBG_INFO("&ax5051_reglist[%d]: 0x%p",i, &ax5051_reglist[i]);
			// DBG_INFO(" ax5051_reglist[i]: 0x%x",  ax5051_reglist[i]);
			// kl_debug_data(__FUNCTION__, 5, ax5051_reglist[i].buf);
			ret = write_reg(hw, &ax5051_reglist[i]);
			if (ret) {
				DBG_ERR("write_reg(addr: 0x%02hhx, value: 0x%02x) failed: %s (%d)",
					ax5051_reglist[i].addr,
					ax5051_reglist[i].value,
					symbolic(urb_errlist, ret), ret);
				kfree(readbuf);
				return ret;
			}
			// kl_debug_data(__FUNCTION__, 5, ax5051_reglist[i].buf);
		}
	}

	/* do RF Setup */
	ret = doRfSetup(hw);

	kfree(readbuf);
	return ret;
}

/*
 * Hardware Initialization
 */
static int setup_klusb(struct kl_usb *hw)
{
	int retval = 0;

	DBG_INFO("%s",__func__);

	// start usb rx interrupt endpoint
	retval = start_rx_int_transfer(hw);
	if(retval)
		return retval;

	/* init the background machinery for control requests */
	hw->ctrl_urb->dev = hw->dev;
	hw->ctrl_urb->complete = (usb_complete_t)ctrl_complete;
	hw->ctrl_urb->context = hw;

	/* set default history record number to begin with reading */
	hw->history_record_nr = -1;

	/* set default Logger ID */
	hw->logger_id = LOGGER_1;

	hw->nextSleepMs = 5;
	hw->config_changed = false;
	hw->last_sent_history_record_nr = -1;

	retval = initTranseiver(hw);
	return retval;
}

//static inline void kl_delete(struct kl_usb *hw)
//{
//	kl_abort_transfers(hw);
//
//	/* Free data structures. */
//	if (hw->int_in_urb)
//		usb_free_urb(hw->int_in_urb);
//	if (hw->ctrl_urb)
//		usb_free_urb(hw->ctrl_urb);
////	if (hw->ctrl_read_urb)
////		usb_free_urb(hw->ctrl_read_urb);
////	if (hw->ctrl_read_urb2)
////		usb_free_urb(hw->ctrl_read_urb2);
//
//	kfree(hw->int_in_buffer);
//	kfree(hw->ctrl_buffer);
//	kfree(hw->ctrl_dr);
////	kfree(hw->ctrl_read_buffer);
////	kfree(hw->ctrl_read_buffer2);
////	kfree(hw->ctrl_read_dr);
////	kfree(hw->ctrl_read_dr2);
//	kfree(hw);
//}

/*
 * stop rx interrupt and free resources
 */
static void kl_delete(struct kl_usb *hw)
{
	DBG_INFO("%s", __func__);

	/* rx endpoint using USB INT IN method */
	abort_rx_int_transfer(hw);

	/* rx/tx control endpoint */
	if (hw->ctrl_urb) {
		usb_kill_urb(hw->ctrl_urb);
		usb_free_urb(hw->ctrl_urb);
		hw->ctrl_urb = NULL;
	}

//	if (hw->intf)
//		usb_set_intfdata(hw->intf, NULL);

	if(hw->rx_int_buffer);
		kfree(hw->rx_int_buffer);

	kfree(hw);
	hw = NULL;
}

/*
 * open() syscall
 */
static int kl_open(struct inode *inode, struct file *file)
{
	struct kl_usb *hw = NULL;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;

	DBG_INFO("Open device");
	subminor = iminor(inode);

	/* lock this device */
	mutex_lock(&disconnect_mutex);

	interface = usb_find_interface(&kl_driver, subminor);
	if (!interface)
	{
		DBG_ERR("can't find device for minor %d", subminor);
		retval = -ENODEV;
		goto exit;
	}

	hw = usb_get_intfdata(interface);
	if (!hw)
	{
		retval = -ENODEV;
		goto exit;
	}

	/* Increment our usage count for the device. */
	++hw->open_count;

	DBG_DEBUG("open_count = %d", hw->open_count);

	/* Save our object in the file's private structure. */
	file->private_data = hw;

exit:
	mutex_unlock(&disconnect_mutex);
	return retval;
}

/*
 * close() syscall
 */
static int kl_release(struct inode *inode, struct file *file)
{
	struct kl_usb *hw = NULL;
	int retval = 0;

	DBG_INFO("Release driver");
	hw = file->private_data;

	if (!hw)
	{
		DBG_ERR("hw is NULL");
		retval = -ENODEV;
		goto exit;
	}

	/* Lock our device */
	mutex_lock(&disconnect_mutex);


	if (hw->open_count <= 0)
	{
		DBG_ERR("device not opened");
		retval = -ENODEV;
		goto unlock_exit;
	}

	if (!hw->dev)
	{
		DBG_DEBUG("device unplugged before the file was released");
		kl_delete(hw);
		goto unlock_exit;
	}

	--hw->open_count;

	DBG_DEBUG("open_count = %d", hw->open_count);


unlock_exit:
	mutex_unlock(&disconnect_mutex);

exit:
	return retval;
}

/*
 * write() syscall
 */
static ssize_t kl_write(struct file *file, const char __user *user_buf,
			size_t count, loff_t *ppos)
{

	struct kl_usb *hw = NULL;
	size_t not_copied;
	int value = 0;
	int retval = 0;

	__u8 rcv_buf[8];

	/* write syscall */
	DBG_INFO("send command");

	hw = file->private_data;

	if (!hw)
	{
		DBG_ERR("hw is NULL");
		retval = -ENODEV;
		goto exit;
	}

	mutex_lock(&disconnect_mutex);	/* Jetzt nicht disconnecten... */

	/* Verify that the device wasn't unplugged. */
	if (!hw->dev) {
		DBG_ERR("No device or device unplugged (%d)", retval);
		retval = -ENODEV;
		goto unlock_exit;
	}

	/* Verify that we actually have some data to write. */
	if (count == 0) {
		DBG_ERR("count = 0");
		goto unlock_exit;
	}

	if (count > 8) {
		DBG_ERR("count > 8");
		goto unlock_exit;
	}

	memset(&rcv_buf, 0, sizeof(rcv_buf));

	not_copied = copy_from_user(&rcv_buf, user_buf, count);

	retval = count - not_copied;
	DBG_INFO("copied %4d bytes from user-space", retval);

	kl_debug_data(__FUNCTION__, 8, rcv_buf);

	value = (rcv_buf[3] << 24 |
		 rcv_buf[2] << 16 |
		 rcv_buf[1] <<  8 |
		 rcv_buf[0] <<  0) & 0x7fffffff;

	DBG_INFO("received value: %d", value);

	if(value >= KL_MAX_RECORD)
	{
		retval = -EOVERFLOW;
		goto unlock_exit;
	}
	else
	{
		hw->history_record_nr = value;
	}


unlock_exit:
	mutex_unlock(&disconnect_mutex);

exit:
	return retval;
}


/*
 * build acknowledge frame
 */
static void kl_buildACKFrame(struct kl_usb *hw, int deviceID, int checksum, unsigned char action, unsigned char *framebuf)
{
	int history_addr;

	if((hw->history_record_nr < 0) || (hw->history_record_nr >= KL_MAX_RECORD))
	{
		history_addr = 0xffffff;
	}
	else
	{
		history_addr = 32 * hw->history_record_nr + 0x070000;
	}

	framebuf[0]  = (deviceID >> 8) & 0xff;
	framebuf[1]  = (deviceID >> 0) & 0xff;
	framebuf[2]  = hw->logger_id;
	framebuf[3]  = action & 0x0f;
	framebuf[4]  = (checksum >> 8) & 0xff;
	framebuf[5]  = (checksum >> 8) & 0xff;
	framebuf[6]  = 0x80;	// TODO: not known what this means
	framebuf[7]  = KL_COMM_INT & 0xff;
	framebuf[8]  = (history_addr >> 16) & 0xff;
	framebuf[9]  = (history_addr >>  8) & 0xff;
	framebuf[10] = (history_addr >>  0) & 0xff;

//	DBG_INFO("history_addr     : 0x%06x", (framebuf[8]  << 16) |
//					      (framebuf[9]  <<  8) |
//					      (framebuf[10] <<  0));

}

/*
 * send setFrame message
 */
static int kl_msg_setFrame(struct kl_usb *hw, int deviceID, int checksum, unsigned char action)
{
	int i, ret;
	unsigned char *frameAckBuf;
	unsigned char *setFramebuf;

	frameAckBuf = kcalloc(11, 1, GFP_KERNEL);
	if(!frameAckBuf) {
		return -ENOMEM;
	}

	setFramebuf = kcalloc(KL_LEN_SET_FRAME, 1, GFP_KERNEL);
	if(!frameAckBuf) {
		kfree(frameAckBuf);
		return -ENOMEM;
	}

	/* buildACKFrame in kl.py */
	kl_buildACKFrame(hw, deviceID, checksum, action, frameAckBuf);

	setFramebuf[0] = KL_MSG_SET_FRAME;
	setFramebuf[1] = 0x00;
	setFramebuf[2] = 11;	// message length (starting with next byte)
	for(i = 0; i < 11; i++)
	{
		setFramebuf[i+3] = frameAckBuf[i];
	}

	/* if successful, returns the number of bytes transferred */
	ret = usb_control_msg(hw->dev,
			      usb_sndctrlpipe(hw->dev, 0),
			      USB_REQ_SET_CONFIGURATION,
			      USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			      (USB_HID_FEATURE_REPORT << 8) | KL_MSG_SET_FRAME,
			      0,
			      setFramebuf,
			      KL_LEN_SET_FRAME,
			      KL_USB_CTRL_TIMEOUT);

	kfree(frameAckBuf);
	kfree(setFramebuf);
	return ret;
}

/*
 * send setTx message
 */
static int kl_msg_setTX(struct kl_usb *hw)
{
	int ret;
	unsigned char *setTXbuf = kcalloc(KL_LEN_SET_TX, 1, GFP_KERNEL);

	if(!setTXbuf) {
		return -ENOMEM;
	}

	setTXbuf[0] = KL_MSG_SET_TX;

	/* if successful, returns the number of bytes transferred */
	ret = usb_control_msg(hw->dev,
			      usb_sndctrlpipe(hw->dev, 0),
			      USB_REQ_SET_CONFIGURATION,
			      USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			      (USB_HID_FEATURE_REPORT << 8) | KL_MSG_SET_TX,
			      0,
			      setTXbuf,
			      KL_LEN_SET_TX,
			      KL_USB_CTRL_TIMEOUT);

	kfree(setTXbuf);
	return ret;
}

/*
 * set history record number which will be requested on the next getHistory()
 */
static void setNextHistoryRecordNr(struct kl_usb *hw, int receivedIndex)
{
	if(hw->history_record_nr < 0)
		hw->history_record_nr = receivedIndex;
	else
		hw->history_record_nr += 6;

	if(hw->history_record_nr >= KL_MAX_RECORD)
		hw->history_record_nr = 0;
}

/*
 * read() syscall
 */
static ssize_t kl_read(struct file *file, char *buffer,
		       size_t count, loff_t * ppos)
{
	int 		ret;
	int 		tryGetStateCounter;
	int 		framelen;
	int 		bufferID;
	int 		loggerID;
	int 		respType;
	int 		signalQuality;
	int 		cs;
	int 		latestIndex;
	int 		thisIndex;
	size_t 		to_copy, not_copied;
	unsigned char	*getStateBuf;
	unsigned char	*rawdata;
	struct kl_usb *hw = NULL;

	DBG_INFO("*** read called (count=%u) ***", (unsigned int)count);


	//TODO check size of user-space buffer

	getStateBuf = kcalloc(KL_LEN_GET_STATE, 1, GFP_KERNEL);
	if(!getStateBuf) {
		DBG_ERR("no memory\n");
		return -ENOMEM;
	}

	rawdata = kcalloc(KL_LEN_GET_FRAME, 1, GFP_KERNEL);
	if (!rawdata) {
		DBG_ERR("no memory\n");
		kfree(getStateBuf);
		return -ENOMEM;
	}


//	DBG_INFO("read command, read by \"%s\" (pid %i), size=%lu",
//			current->comm, current->pid, (unsigned long ) count);

	hw = (struct kl_usb *)file->private_data;

	if (!hw) {
		DBG_ERR("hw is NULL!");
		count = -ENODEV;
		goto read_out;
	}


	mutex_lock(&disconnect_mutex);	/* Jetzt nicht disconnecten... */

	//firstSleep
	msleep(75);

getState:
	tryGetStateCounter = 0;

	// try for about 1 second to get connection state == 0x16
	while(tryGetStateCounter < 100)
	{
		/* getState in kl.py */

		/* if successful, returns the number of bytes transferred */
		ret =
		    usb_control_msg(hw->dev,
				    usb_rcvctrlpipe(hw->dev, 0),
				    USB_REQ_CLEAR_FEATURE,
				    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
				    (USB_HID_FEATURE_REPORT << 8) | KL_MSG_GET_STATE,
				    0,
				    getStateBuf,
				    KL_LEN_GET_STATE,
				    KL_USB_CTRL_TIMEOUT);

		if (ret < 0) {
			DBG_ERR("getState() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
			count = -EIO;
			goto read_out;
		}

		// read logger state
//		state = atomic_read(&hw->logger_state);
//		DBG_INFO("INT state: 0x%x GetState: 0x%x tryGetStateCounter: %d", state, getStateBuf[1], tryGetStateCounter);

		if(getStateBuf[1] != 0x16){
			tryGetStateCounter++;
			msleep(hw->nextSleepMs);
		}
		else
		{
			break;
		}
	}


	if (getStateBuf[1] == 0x16) {
		DBG_INFO("getFrame() - tryGetStateCounter: %2d", tryGetStateCounter);

		/* getFrame in kl.py */

		/* if successful, returns the number of bytes transferred */
		ret =
		    usb_control_msg(hw->dev,
				    usb_rcvctrlpipe(hw->dev, 0),
				    USB_REQ_CLEAR_FEATURE,
				    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
				    (USB_HID_FEATURE_REPORT << 8) | KL_MSG_GET_FRAME,
				    0,
				    rawdata,
				    KL_LEN_GET_FRAME,
				    KL_USB_CTRL_TIMEOUT);
		if (ret < 0) {
			DBG_ERR("getFrame() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
			count = -EIO;
			goto read_out;
		}


//		kl_debug_data(__FUNCTION__, 0x111, rawdata);

		// generateResponse in kl.py
		framelen	= (rawdata[1] << 8 | rawdata[2]) & 0x1ff;
		bufferID	= (rawdata[3] << 8) | rawdata[4];
		loggerID	= rawdata[5];
		respType	= rawdata[6] & 0xF0;
		signalQuality	= rawdata[7];
		DBG_INFO("framelen: 0x%02x, bufferID: 0x%02x, loggerID: 0x%02x, response: 0x%02x, signalQuality: 0x%02x", framelen, bufferID, loggerID, rawdata[6], signalQuality);


		if (bufferID == 0xF0F0)
		{
			DBG_ERR("generateResponse: console not paired (synchronized)");
		} else
		{
			if (respType == RESPONSE_DATA_WRITTEN)		// length: 0x07 (7)
			{
				DBG_INFO("RESPONSE_DATA_WRITTEN");
				hw->nextSleepMs = 10;
				goto getState;
			}
			else if (respType == RESPONSE_GET_CONFIG)	// length: 0x7d (125)
			{
				DBG_INFO("RESPONSE_GET_CONFIG");

				/* handleConfig in kl.py */
				cs = rawdata[127] | (rawdata[126] << 8);
//				DBG_INFO("handleGetConfig: cs = 0x%04x", cs);

				/* setFrame in kl.py */
				ret = kl_msg_setFrame(hw, bufferID, cs, ACTION_GET_HISTORY);

				if (ret < 0) {
					DBG_ERR("setFrame() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
					count = -EIO;
					goto read_out;
				}

				/*  setTX in kl.py */
				ret = kl_msg_setTX(hw);

				if (ret < 0) {
					DBG_ERR("setTx() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
					count = -EIO;
					goto read_out;
				}

				hw->nextSleepMs = 10;
				goto getState;


			}
			else if (respType == RESPONSE_GET_CURRENT)	// length: 0xe5 (229)
			{
				DBG_INFO("RESPONSE_GET_CURRENT");

				/* handleCurrentData in kl.py */
				cs = rawdata[9] | (rawdata[8] << 8);
//				DBG_INFO("handleCurrentData: cs = 0x%04x", cs);


				// on first read
				if(hw->config_changed)
				{

					/* setFrame in kl.py */
					ret = kl_msg_setFrame(hw, bufferID, cs, ACTION_GET_CONFIG);

					if (ret < 0) {
						DBG_ERR("setFrame() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
						count = -EIO;
						goto read_out;
					}

					hw->nextSleepMs = 10;
					hw->config_changed = false;
				}
				else
				{
					/* setFrame in kl.py */
					ret = kl_msg_setFrame(hw, bufferID, cs, ACTION_GET_HISTORY);

					if (ret < 0) {
						DBG_ERR("setFrame() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
						count = -EIO;
						goto read_out;
					}

					hw->nextSleepMs = 10;
				}

				/* setTX in kl.py */
				ret = kl_msg_setTX(hw);

				if (ret < 0) {
					DBG_ERR("setTX() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
					count = -EIO;
					goto read_out;
				}

				goto getState;


//				to_copy = 0x111;
//				not_copied = copy_to_user(buffer, rawdata, to_copy);
//				count = to_copy - not_copied;
//
//				DBG_INFO("copied %4ld bytes to user-space", count);

			}
			else if (respType == RESPONSE_GET_HISTORY)	// length: 0xb5 (181)
			{
				DBG_INFO("RESPONSE_GET_HISTORY");

				/* handleHistoryData in kl.py */
				cs = rawdata[9] | (rawdata[8] << 8);
//				DBG_INFO("handleHistoryData: cs = 0x%04x", cs);

				// bytes_to_addr
				// index_to_addr
				latestIndex =
				    (((((rawdata[10] << 8) | rawdata[11]) << 8) |
						    rawdata[12]) - 0x070000) / 32;
				thisIndex =
				    (((((rawdata[13] << 8) | rawdata[14]) << 8)
				      | rawdata[15]) - 0x070000) / 32;


				DBG_INFO("latestIndex      : 0x%06x (%5d)", latestIndex, latestIndex);
				DBG_INFO("thisIndex        : 0x%06x (%5d)", thisIndex, thisIndex);
				DBG_INFO("history_record_nr: 0x%06x (%5d)", hw->history_record_nr, hw->history_record_nr);


				if(latestIndex != thisIndex)
					setNextHistoryRecordNr(hw, thisIndex);


				/* setFrame in kl.py */
				ret = kl_msg_setFrame(hw, bufferID, cs, ACTION_GET_HISTORY);

				if (ret < 0) {
					DBG_ERR("setFrame() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
					count = -EIO;
					goto read_out;
				}

				/* setTX in kl.py */
				ret = kl_msg_setTX(hw);

				if (ret < 0) {
					DBG_ERR("setTX() failed: %s (%d)", symbolic(urb_errlist, ret), ret);
					count = -EIO;
					goto read_out;
				}

				hw->nextSleepMs = 5;

				if(latestIndex != thisIndex)
				{
					to_copy = KL_LEN_GET_FRAME;
					not_copied = copy_to_user(buffer, rawdata, to_copy);
					count = to_copy - not_copied;
					if(count)
					{
						hw->last_sent_history_record_nr = thisIndex;
					}
					DBG_INFO("copied %4u bytes to user-space", (unsigned int)count);
				}
				else
				{
					// TODO try latestIndex
					if(thisIndex != hw->last_sent_history_record_nr)
					{
						to_copy = KL_LEN_GET_FRAME;
						not_copied = copy_to_user(buffer, rawdata, to_copy);
						count = to_copy - not_copied;
						if(count)
						{
							hw->last_sent_history_record_nr = thisIndex;
						}
						DBG_INFO("copied %4u bytes to user-space", (unsigned int)count);
					}
					else
					{
						DBG_INFO("No new history data available");
						count = 0;	/* EOF */
						goto read_out;
					}
				}

			}
			else if (respType == RESPONSE_REQUEST)	// length: 0x7d (125)
			{
				DBG_INFO("RESPONSE_REQUEST (0x%x)", rawdata[6]);
				hw->nextSleepMs = 10;
				goto getState;
			}
			else
			{
				DBG_INFO("RESPONSE: 0x%02x", respType);
				hw->nextSleepMs = 5;
				goto getState;
			}
		}
	} else
	{
		count = -200;	/* Press USB button error code */
		goto read_out;
	}


read_out:
	mutex_unlock(&disconnect_mutex);

	kfree(rawdata);
	kfree(getStateBuf);
	return count;
}


/*
 * setup driver instance
 */
static int setup_instance(struct kl_usb *hw)
{
	int	err = 0;

	DBG_INFO("%s", __func__);

//	sema_init(&hw->sem, 1);

	spin_lock_init(&hw->ctrl_lock);

	err = setup_klusb(hw);
	if (err)
		goto out;

	return 0;

out:
	kfree(hw);
	return err;
}

/*
 * probe() function, called when a USB device is connected to the computer
 */
static int kl_probe(struct usb_interface *intf,
		const struct usb_device_id *id)
{
	struct kl_usb 			*hw = NULL;
	struct usb_device 		*dev = interface_to_usbdev(intf);
	struct usb_host_interface	*iface_desc = intf->cur_altsetting;
	struct usb_endpoint_descriptor	*ep_desc;

	int idVendor = le16_to_cpu(dev->descriptor.idVendor);
	int idProduct = le16_to_cpu(dev->descriptor.idProduct);
	int int_end_packet_size; /* interrupt endpoint packet size */
	int i;

	int retval = -ENODEV;

	DBG_INFO("Probe KlimaLogg Pro: Vendor(0x%x) Product(0x%x)", idVendor, idProduct);

	if (!dev)
	{
		DBG_ERR("dev is NULL");
		return -ENODEV;
	}

	hw = kzalloc(sizeof(struct kl_usb), GFP_KERNEL);
	if (!hw)
	{
		DBG_ERR("cannot allocate memory for struct kl_usb");
		return -ENOMEM;
	}

	/* Set up interrupt endpoint information. */
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {

		ep_desc = &iface_desc->endpoint[i].desc;

		DBG_INFO("examining endpoint (interface nr: %hhd, endpoint address: 0x%hhx, attributes: 0x%hhx)",
			 iface_desc->desc.bInterfaceNumber,
			 ep_desc->bEndpointAddress,
			 ep_desc->bmAttributes);

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// don't use:
		//   if ( ((ep_desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK)  == USB_DIR_IN)
		//
		// USB_ENDPOINT_DIR_MASK is defined as 0x81 on the ARM target which is against the USB standard!!!
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		if ( ((ep_desc->bEndpointAddress & 0x80)  == USB_DIR_IN)
		     &&
		     ((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT))
		{
			hw->rx_int_endpoint_desc = ep_desc;
		}
	}

	if (!hw->rx_int_endpoint_desc) {
		DBG_ERR("could not find rx interrupt in endpoint");
		retval = -ENODEV;
		goto error;
	}

	hw->rx_int_in_pipe = usb_rcvintpipe(dev,
					    hw->rx_int_endpoint_desc->bEndpointAddress);

	int_end_packet_size = le16_to_cpu(hw->rx_int_endpoint_desc->wMaxPacketSize);
	DBG_INFO("rx interrupt endpoint packet size: %d", int_end_packet_size);

	hw->rx_int_buffer = kmalloc(int_end_packet_size, GFP_KERNEL);
	if (!hw->rx_int_buffer) {
		DBG_ERR("could not allocate rx_int_buffer");
		retval = -ENOMEM;
		goto error;
	}


	atomic_set(&hw->logger_state, 0);

	hw->dev = dev; /* save device */
	hw->ctrl_packet_size = dev->descriptor.bMaxPacketSize0; /* control size */

	/* create the control pipes needed for register access */
	hw->ctrl_in_pipe = usb_rcvctrlpipe(hw->dev, 0);
	hw->ctrl_out_pipe = usb_sndctrlpipe(hw->dev, 0);


	hw->ctrl_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!hw->ctrl_urb) {
		DBG_ERR("No memory for control urb");
		retval = -ENOMEM;
		goto error;
	}

	if (setup_instance(hw))
	{
		DBG_ERR("setup_instance failed!");
		return -EIO;
	}

	/* Retrieve a serial. */
	if (!usb_string(dev, dev->descriptor.iSerialNumber,
			hw->serial_number, sizeof(hw->serial_number)))
	{
		DBG_ERR("could not retrieve serial number");
		retval = -ENODEV;
		goto error;
	}

	/* Save our data pointer in this interface device. */
	hw->intf = intf;
	usb_set_intfdata(intf, hw);

	/* We can register the device now, as it is ready. */
	retval = usb_register_dev(intf, &kl_class);
	if (retval)
	{
		DBG_ERR("not able to get a minor for this device.");
		usb_set_intfdata(intf, NULL);
		goto error;
	}

	hw->minor = intf->minor;

	DBG_INFO("USB KlimaLogg Pro now attached to /dev/kl%d (minor=%d, ML_MINOR_BASE=%d)" ,
			intf->minor - ML_MINOR_BASE, intf->minor, ML_MINOR_BASE);

	return 0;

error:
	kfree(hw);
	return retval;
}

/*
 * disconnect() function, called when unplugging a USB device
 */
static void kl_disconnect(struct usb_interface *intf)
{
	struct kl_usb *hw;
	int minor;

	mutex_lock(&disconnect_mutex); /* Not interruptible */

	hw = usb_get_intfdata(intf);
	minor = hw->minor;

	kl_delete(hw);

	usb_set_intfdata(intf, NULL);

//	down(&hw->sem); /* Not interruptible */

	/* Give back our minor. */
	usb_deregister_dev(intf, &kl_class);


	mutex_unlock(&disconnect_mutex);

	DBG_INFO("USB KlimaLogg Pro /dev/kl%d now disconnected",
			minor - ML_MINOR_BASE);
}

/*
 * init() function, called on module loading
 */
static int __init usb_kl_init(void)
{
	int result;

	DBG_INFO("Register driver");
	result = usb_register(&kl_driver);
	//result = usb_register_driver(&kl_driver, THIS_MODULE, "kl_usb_drv");

	if (result)
	{
		DBG_ERR("registering driver failed");
	} else
	{
		DBG_INFO("driver registered successfully");
	}

	return result;
}

/*
 * exit() function, called on module unloading
 */
static void __exit usb_kl_exit(void)
{
	usb_deregister(&kl_driver);
	DBG_INFO("module deregistered");
}


