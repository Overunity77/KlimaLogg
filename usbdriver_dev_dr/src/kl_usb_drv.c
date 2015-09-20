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

#define KL_CTRL_READ_BUFFER_SIZE 	16
#define KL_CTRL_READ_BUFFER2_SIZE 	21
#define KL_CTRL_READ_REQUEST_TYPE	0x21
#define KL_CTRL_SET_REPORT_REQUEST	0x09

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


static struct usb_device_id kl_table[] = { {
		USB_DEVICE(KL_VENDOR_ID, KL_PRODUCT_ID) }, { } };
MODULE_DEVICE_TABLE(usb, kl_table);

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

/* forward declaration */
static struct usb_driver kl_driver;


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


/* start next background transfer for control channel */
static void ctrl_start_transfer(struct kl_usb *hw)
{
	int ret;

	if (hw->ctrl_cnt) {
		DBG_INFO("start next usb ctrl transfer");

		hw->ctrl_urb->pipe = hw->usb_ctrl_buff[hw->ctrl_out_idx].pipe;
		hw->ctrl_urb->setup_packet = (u_char *)&hw->usb_ctrl_buff[hw->ctrl_out_idx].ctrlrequest;
		hw->ctrl_urb->transfer_buffer = hw->usb_ctrl_buff[hw->ctrl_out_idx].buf;
		hw->ctrl_urb->transfer_buffer_length = hw->usb_ctrl_buff[hw->ctrl_out_idx].buflen;

//		hw->ctrl_write.wIndex =
//			cpu_to_le16(hw->ctrl_buff[hw->ctrl_out_idx].ax5051_reg);
//		hw->ctrl_write.wValue =
//			cpu_to_le16(hw->ctrl_buff[hw->ctrl_out_idx].reg_val);

		ret = usb_submit_urb(hw->ctrl_urb, GFP_ATOMIC);
		if (ret)
			DBG_ERR("failed to submit usb urb (%d)", ret);
	}
	else
	{
		DBG_INFO("usb ctrl transfer queue is empty");
	}
}

/* queue a control transfer write request */
/* the param: void* data will be de-allocated when the urb completes */
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

/* queue a control transfer write request */
/* the param: void* data will be de-allocated when the urb completes */
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
 * queue a control transfer request to write HFC-S USB
 * chip register using CTRL request queue
 */
static int write_reg(struct kl_usb *hw, struct klusb_ax5015_register_list *reg)
{
	struct usb_ctrl_buf *buf;
	int ret = 0;

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



/* control completion routine handling background control cmds */
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


static inline void kl_delete(struct kl_usb *hw)
{
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
}



/* receive completion routine for rx interrupt endpoint */
static void rx_int_complete(struct urb *urb)
{
	struct kl_usb *hw = (struct kl_usb *) urb->context;
	int retval = 0;

	if (urb->status) {
		DBG_INFO("urb->status: %s (%d)", symbolic(urb_errlist, urb->status), urb->status);
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
	DBG_INFO("RX INT length(%d)", urb->actual_length);
	kl_debug_data(__FUNCTION__, urb->actual_length, hw->rx_int_buffer);

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

/* start the rx interrupt endpoint transfer */
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
		DBG_ERR("submitting rx interrupt urb failed (%d)", retval);
		hw->rx_int_running = 0;
		kfree(hw->rx_int_urb); // TODO check for release on driver exit
		return retval;
	}

	return retval;
}

static int readConfigFlash(struct kl_usb *hw, struct usb_read_config_flash *config) {
	int ret = 0;
	__u8 writebuf[KL_LEN_READ_CONFIG_FLASH_OUT];
	int i;

	for (i = 0; i < KL_LEN_READ_CONFIG_FLASH_OUT; i++)
		writebuf[i] = 0xcc;

	writebuf[0]  = 0xdd;
	writebuf[1]  = 0x0a;
	writebuf[2]  = (config->addr >> 8) & 0xff;
	writebuf[3]  = (config->addr >> 0) & 0xff;

	DBG_INFO("write buffer:");
	kl_debug_data(__FUNCTION__, KL_LEN_READ_CONFIG_FLASH_OUT, writebuf);


	ret = usb_control_msg(hw->dev, hw->ctrl_out_pipe, USB_REQ_SET_CONFIGURATION,
			      USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT,
			      (USB_HID_FEATURE_REPORT << 8) | KL_MSG_READ_CONFIG_FLASH_OUT,
			      0, writebuf, KL_LEN_READ_CONFIG_FLASH_OUT, KL_USB_CTRL_TIMEOUT
			     );
	if (ret < 0) {
		DBG_ERR("Could not prepare to read config flash");
		return ret;
	}

	ret = usb_control_msg(hw->dev, hw->ctrl_out_pipe, USB_REQ_CLEAR_FEATURE,
			      USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
			      (USB_HID_FEATURE_REPORT << 8) | KL_MSG_READ_CONFIG_FLASH_IN,
			      0, config->readBuf, config->buflen, KL_USB_CTRL_TIMEOUT
			     );

	if (ret < 0) {
		DBG_ERR("Could not read from config flash");
		return ret;
	}

	return ret;
}

static int doRfSetup(struct kl_usb *hw)
{
	int ret = 0;

	/* execute(5) */
	hw->rf_setup_buffers.buf_execute = kcalloc(KL_LEN_EXECUTE, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_execute)
		return -ENOMEM;

	hw->rf_setup_buffers.buf_execute[0] = 0xd9;
	hw->rf_setup_buffers.buf_execute[1] = 0x05;
	ret = write_usb_ctrl(hw,
		       KL_MSG_EXECUTE,
		       KL_LEN_EXECUTE,
		       hw->rf_setup_buffers.buf_execute);

	if (ret < 0) {
		printk("Error execute(5): %d\n", ret);
		return ret;
	}

	/* setPreamblePattern(0xaa) */
	hw->rf_setup_buffers.buf_preamble_first = kcalloc(KL_LEN_SET_PREAMBLE_PATTERN, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_preamble_first)
		return -ENOMEM;

	hw->rf_setup_buffers.buf_preamble_first[0] = 0xd8;
	hw->rf_setup_buffers.buf_preamble_first[1] = 0xaa;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_PREAMBLE_PATTERN,
		       KL_LEN_SET_PREAMBLE_PATTERN,
		       hw->rf_setup_buffers.buf_preamble_first);

	if (ret < 0) {
		printk("Error setPreamblePattern(0xaa): %d\n", ret);
		return ret;
	}

	/* setState(0) */
	hw->rf_setup_buffers.buf_setstate_first = kcalloc(KL_LEN_SET_STATE, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_setstate_first)
		return -ENOMEM;

	hw->rf_setup_buffers.buf_setstate_first[0] = 0xd7;
	hw->rf_setup_buffers.buf_setstate_first[1] = 0x00;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_STATE,
		       KL_LEN_SET_STATE,
		       hw->rf_setup_buffers.buf_setstate_first);

	if (ret < 0) {
		printk("Error setState(0): %d\n", ret);
		return ret;
	}

	// TODO sleep(1) ???
	msleep(1000);

	/* setRx() */
	hw->rf_setup_buffers.buf_setRx_first = kcalloc(KL_LEN_SET_RX, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_setRx_first)
		return -ENOMEM;

	hw->rf_setup_buffers.buf_setRx_first[0] = 0xd0;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_RX,
		       KL_LEN_SET_RX,
		       hw->rf_setup_buffers.buf_setRx_first);

	if (ret < 0) {
		printk("Error setRx(): %d\n", ret);
		return ret;
	}

	/* setPreamblePattern(0xaa) */
	hw->rf_setup_buffers.buf_preamble_second = kcalloc(KL_LEN_SET_PREAMBLE_PATTERN, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_preamble_second)
		return -ENOMEM;

	hw->rf_setup_buffers.buf_preamble_second[0] = 0xd8;
	hw->rf_setup_buffers.buf_preamble_second[1] = 0xaa;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_PREAMBLE_PATTERN,
		       KL_LEN_SET_PREAMBLE_PATTERN,
		       hw->rf_setup_buffers.buf_preamble_second);

	if (ret < 0) {
		printk("Error setPreamblePattern(0xaa): %d\n", ret);
		return ret;
	}

	/* setState(0x1e) */
	hw->rf_setup_buffers.buf_setstate_second = kcalloc(KL_LEN_SET_STATE, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_setstate_second)
		return -ENOMEM;

	hw->rf_setup_buffers.buf_setstate_second[0] = 0xd7;
	hw->rf_setup_buffers.buf_setstate_second[1] = 0x1e;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_STATE,
		       KL_LEN_SET_STATE,
		       hw->rf_setup_buffers.buf_setstate_second);

	if (ret < 0) {
		printk("Error setState(0x1e): %d\n", ret);
		return ret;
	}

	// TODO sleep(1) ???
	msleep(1000);

	/* setRx() */
	hw->rf_setup_buffers.buf_setRx_second = kcalloc(KL_LEN_SET_RX, 1, GFP_KERNEL);
	if (!hw->rf_setup_buffers.buf_setRx_second)
		return -ENOMEM;

	hw->rf_setup_buffers.buf_setRx_second[0] = 0xd0;
	ret = write_usb_ctrl(hw,
		       KL_MSG_SET_RX,
		       KL_LEN_SET_RX,
		       hw->rf_setup_buffers.buf_setRx_second);

	if (ret < 0) {
		printk("Error setRx(): %d\n", ret);
		return ret;
	}

	return 0;
}

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

static int initTranseiver(struct kl_usb *hw)
{
	struct usb_read_config_flash freqCorrection;
	struct usb_read_config_flash transceiverId;
	unsigned int corVal = 0;
	unsigned int freqVal = 0;
	__u8 readbuf[KL_LEN_READ_CONFIG_FLASH_IN];
	int ret;
	int countReg, i;

	DBG_INFO("%s", __func__);

	/* read frequency correction */
	freqCorrection.addr = 0x1f5;
	freqCorrection.buflen = KL_LEN_READ_CONFIG_FLASH_IN;
	freqCorrection.readBuf = readbuf;

	ret = readConfigFlash(hw, &freqCorrection);

	DBG_INFO("freqCorrection read buffer:");
	kl_debug_data(__FUNCTION__, freqCorrection.buflen, freqCorrection.readBuf);

	corVal = freqCorrection.readBuf[4] << 8;
	corVal |= freqCorrection.readBuf[5];
	corVal <<= 8;
	corVal |= freqCorrection.readBuf[6];
	corVal <<= 8;
	corVal |= freqCorrection.readBuf[7];

	DBG_INFO("frequency correction: %u (0x%x)", corVal, corVal);

	freqVal = KL_FREQ_VAL + corVal;

	if (!(freqVal % 2))
		freqVal += 1;

	DBG_INFO("adjusted frequency: %u (0x%x)", freqVal, freqVal);


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

	DBG_INFO("transceiverId read buffer:");
	kl_debug_data(__FUNCTION__, transceiverId.buflen, transceiverId.readBuf);

	hw->transceiver_id = (transceiverId.readBuf[9] << 8) + transceiverId.readBuf[10];

	DBG_INFO("transceiver identifier: %u (0x%x)", hw->transceiver_id, hw->transceiver_id);

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
				DBG_ERR("write_reg failed!");
				return ret;
			}

			// msleep(10);
			// kl_debug_data(__FUNCTION__, 5, ax5051_reglist[i].buf);
		}

	}

	/* do RF Setup */
	ret = doRfSetup(hw);

	return ret;
}

/* Hardware Initialization */
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

	retval = initTranseiver(hw);
	return retval;
}

static void release_hw(struct kl_usb *hw)
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

	if (hw->intf)
		usb_set_intfdata(hw->intf, NULL);

	kfree(hw);
	hw = NULL;
}

static int kl_open(struct inode *inode, struct file *file)
{
	/* open syscall */
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

	/* start rx interrupt URB. */
	// start_rx_int_transfer(hw);

	/* Increment our usage count for the device. */
	++hw->open_count;
	if (hw->open_count > 1)
		DBG_DEBUG("open_count = %d", hw->open_count);

	/* Save our object in the file's private structure. */
	file->private_data = hw;

exit:
	mutex_unlock(&disconnect_mutex);
	return retval;
}

static int kl_release(struct inode *inode, struct file *file)
{
	/* close syscall */
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

//	/* rx endpoint using USB INT IN method */
//	abort_rx_int_transfer(hw);

	if (!hw->dev)
	{
		DBG_DEBUG("device unplugged before the file was released");
		kl_delete(hw);
		goto unlock_exit;
	}

	if (hw->open_count > 1)
		DBG_DEBUG("open_count = %d", hw->open_count);

	--hw->open_count;

unlock_exit:
	mutex_unlock(&disconnect_mutex);

exit:
	return retval;
}

static ssize_t kl_write(struct file *file, const char __user *user_buf,
		size_t count, loff_t *ppos)
{

//	struct kl_usb *hw = NULL;
//	ssize_t retval = count;
//	int ret = 0;
//
//	/* write syscall */
//	DBG_INFO("send command");
//
//	hw = file->private_data;
//
//	hw->ctrl_read_transfer_count = 0;
//
//	if (!hw)
//	{
//		DBG_ERR("hw is NULL");
//		retval = -ENODEV;
//		goto exit;
//	}
//	/* Set up the control URB. */
//	hw->ctrl_read_urb = usb_alloc_urb(0, GFP_KERNEL);
//	if (!hw->ctrl_read_urb)
//	{
//		DBG_ERR("could not allocate ctrl_read_urb");
//		retval = -ENOMEM;
//		goto error;
//	}
//
//	hw->ctrl_read_urb2 = usb_alloc_urb(0, GFP_KERNEL);
//	if (!hw->ctrl_read_urb2)
//	{
//		DBG_ERR("could not allocate ctrl_read_urb2");
//		retval = -ENOMEM;
//		goto error;
//	}
//
//	hw->ctrl_read_buffer = kzalloc(KL_CTRL_READ_BUFFER_SIZE, GFP_KERNEL);
//	if (!hw->ctrl_read_buffer)
//	{
//		DBG_ERR("could not allocate ctrl_read_buffer");
//		retval = -ENOMEM;
//		goto error;
//	}
//	hw->ctrl_read_buffer2 = kzalloc(KL_CTRL_READ_BUFFER2_SIZE, GFP_KERNEL);
//	if (!hw->ctrl_read_buffer2)
//	{
//		DBG_ERR("could not allocate ctrl_read_buffer2");
//		retval = -ENOMEM;
//		goto error;
//	}
//
//	hw->ctrl_read_dr = kmalloc(sizeof(struct usb_ctrlrequest), GFP_KERNEL);
//	if (!hw->ctrl_read_dr)
//	{
//		DBG_ERR("could not allocate usb_ctrlrequest");
//		retval = -ENOMEM;
//		goto error;
//	}
//	hw->ctrl_read_dr2 = kmalloc(sizeof(struct usb_ctrlrequest),
//			GFP_KERNEL);
//	if (!hw->ctrl_read_dr2)
//	{
//		DBG_ERR("could not allocate usb_ctrlrequest2");
//		retval = -ENOMEM;
//		goto error;
//	}
//
//	hw->ctrl_read_dr->bRequestType = KL_CTRL_READ_REQUEST_TYPE;
//	hw->ctrl_read_dr->bRequest = KL_CTRL_SET_REPORT_REQUEST;
//	hw->ctrl_read_dr->wValue = cpu_to_le16(0x03dd); /* convert to little-endian */
//	hw->ctrl_read_dr->wIndex = cpu_to_le16(0);
//	hw->ctrl_read_dr->wLength = cpu_to_le16(KL_CTRL_READ_BUFFER_SIZE);
//
//	usb_fill_control_urb(hw->ctrl_read_urb, hw->dev,
//			usb_sndctrlpipe(hw->dev, 0),
//			(unsigned char *) hw->ctrl_read_dr,
//			hw->ctrl_read_buffer,
//			KL_CTRL_READ_BUFFER_SIZE, kl_ctrl_read_callback, hw);
//
//	hw->ctrl_read_buffer[0] = 0xdd;
//	hw->ctrl_read_buffer[1] = 0x0a;
//	hw->ctrl_read_buffer[2] = 0x01;
//	hw->ctrl_read_buffer[3] = 0xF5;
//	hw->ctrl_read_buffer[4] = hw->ctrl_read_buffer[5] =
//			hw->ctrl_read_buffer[6] = hw->ctrl_read_buffer[7] =
//					0xcc;
//	hw->ctrl_read_buffer[8] = hw->ctrl_read_buffer[9] =
//			hw->ctrl_read_buffer[10] = hw->ctrl_read_buffer[11] =
//					0xcc;
//	hw->ctrl_read_buffer[12] = hw->ctrl_read_buffer[13] =
//			hw->ctrl_read_buffer[14] = hw->ctrl_read_buffer[15] =
//					0xcc;
//
//	kl_debug_data(__FUNCTION__, KL_CTRL_READ_BUFFER_SIZE,
//			hw->ctrl_read_buffer);
//
//	DBG_INFO("going to submit control urb");
//
//	ret = usb_submit_urb(hw->ctrl_read_urb, GFP_ATOMIC);
//	if (ret)
//	{
//		DBG_ERR("submitting set_report control URB failed (%d)", ret);
//	}
//
//	exit: return retval;
//
//	error: return retval;

}

//static ssize_t kl_read(struct file *file, char *buffer, size_t count,
//		loff_t *ppos)
//{
//
//	/* read syscall */
//	struct kl_usb *hw = NULL;
//	ssize_t retval = 0;
//
//	DBG_INFO("read command, read by \"%s\" (pid %i), size=%lu",
//			current->comm, current->pid, (unsigned long ) count);
//
//	hw = file->private_data;
//
//	DBG_INFO("read transfer count: %u", hw->ctrl_read_transfer_count);
//
//	retval = hw->ctrl_read_transfer_count;
//
//	if (*ppos == retval)
//		return 0;
//
//	if (copy_to_user(buffer, hw->ctrl_read_buffer2, retval))
//	{
//		retval = -EFAULT;
//	}
//
//	*ppos = retval;
//	//readcount++;
//	return retval;
//
//}



#define RESPONSE_DATA_WRITTEN  0x10
#define RESPONSE_GET_CONFIG  0x20
#define RESPONSE_GET_CURRENT  0x30
#define RESPONSE_GET_HISTORY  0x40
#define RESPONSE_REQUEST  0x50
#define RESPONSE_REQ_READ_HISTORY  0x50
#define RESPONSE_REQ_FIRST_CONFIG  0x51
#define RESPONSE_REQ_SET_CONFIG  0x52
#define RESPONSE_REQ_SET_TIME  0x53

#define ACTION_GET_HISTORY   0x00
#define ACTION_REQ_SET_TIME   0x01
#define ACTION_REQ_SET_CONFIG   0x02
#define ACTION_GET_CONFIG   0x03
#define ACTION_GET_CURRENT   0x04
#define ACTION_SEND_CONFIG   0x20
#define ACTION_SEND_TIME   0x60

#define LOGGER_1   0
#define LOGGER_2   1
#define LOGGER_3   2
#define LOGGER_4   3
#define LOGGER_5   4
#define LOGGER_6   5
#define LOGGER_7   6
#define LOGGER_8   7
#define LOGGER_9   8
#define LOGGER_10   9

/* defined in dezi Grad Celsius */
#define TEMPERATURE_OFFSET 400

//static DEFINE_MUTEX(ulock);

static atomic_t bytes_available = ATOMIC_INIT(0);

int klimaloggRecord = 30000;

/* Entspricht getState aus kl.py */
static ssize_t kl_read(struct file *instanz, char *buffer,
			     size_t count, loff_t * ofs)
{
	/* read syscall */

	size_t to_copy, not_copied;


	int ret;
	int nbytes;
	int i;
	int bufferID;
	int respType;
	int cs;
	int haddr;
	int latestIndex;
	int thisIndex;
	unsigned char *retBuf = kcalloc(0x131, 1, GFP_KERNEL);
	unsigned char *data = kcalloc(10, 1, GFP_KERNEL);

	unsigned char *rawdata = kcalloc(0x111, 1, GFP_KERNEL);
	unsigned char *setFramebuf = kcalloc(0x111, 1, GFP_KERNEL);
	unsigned char *setTXbuf = kcalloc(0x15, 1, GFP_KERNEL);

	unsigned char *resultBuf = kcalloc(128, 1, GFP_KERNEL);

	struct kl_usb *hw = NULL;
	ssize_t retval = 0;

	int state;

//	printk("Count is: %lu\n", count);

	if (!retBuf || !data || !rawdata || !setFramebuf || !setTXbuf || !resultBuf) {
		printk("no memory");
		count = -ENOMEM;
		goto read_out;
	}




//	DBG_INFO("read command, read by \"%s\" (pid %i), size=%lu",
//			current->comm, current->pid, (unsigned long ) count);

	hw = (struct kl_usb *)instanz->private_data;

	if (!hw) {
		printk("hw is NULL!");
		count = -ENODEV;
		goto read_out;
	}

	printk("usb_test: read\n");
	mutex_lock(&disconnect_mutex);	/* Jetzt nicht disconnecten... */

//      printk("read vor = %02x %02x %02x %02x\n", data[0], data[1], data[2],
//             data[3]);
//	ret =
//	    usb_control_msg(hw->dev, usb_rcvctrlpipe(hw->dev, 0), USB_REQ_CLEAR_FEATURE,
//			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
//			    0x3de, 0, data, 10, KL_USB_CTRL_TIMEOUT);
//
//	if (ret < 0) {
//		printk("Error read Nr: %d\n", ret);
//		count = -EIO;
//		goto read_out;
//	}
//
//	kl_debug_data(__FUNCTION__, 10, data);

	// read logger state
	state = atomic_read(&hw->logger_state);
	DBG_INFO("INT state: 0x%x", state);
//	printk("read nach= %02x %02x %02x %02x\n", data[0], data[1], data[2],
//	       data[3]);


	if (state == 0x16) {
		DBG_INFO("getFrame()");

		ret =
		    usb_control_msg(hw->dev, usb_rcvctrlpipe(hw->dev, 0),
				    USB_REQ_CLEAR_FEATURE,
				    USB_TYPE_CLASS | USB_RECIP_INTERFACE |
				    USB_DIR_IN, 0x3d6, 0, rawdata, 0x111,
				    KL_USB_CTRL_TIMEOUT);
		if (ret < 0) {
			printk("Error in getFrame Nr: %d\n", ret);
			count = -EIO;
			goto read_out;
		}

		kl_debug_data(__FUNCTION__, 0x111, rawdata);

		// generateResponse in kl.py
		bufferID = (rawdata[3] << 8) | rawdata[4];
		respType = (rawdata[6] & 0xF0);
		DBG_INFO("bufferID: 0x%02x respType: 0x%02x", bufferID, respType);


		if (bufferID == 0xF0F0) {
			DBG_ERR("generateResponse: console not paired (synchronized)");
		} else {
			if (respType == RESPONSE_DATA_WRITTEN) {
				DBG_INFO("RESPONSE_DATA_WRITTEN");
			} else if (respType == RESPONSE_GET_CONFIG) {
				DBG_INFO("RESPONSE_GET_CONFIG");
			} else if (respType == RESPONSE_GET_CURRENT) {
				DBG_INFO("RESPONSE_GET_CURRENT");

				not_copied = copy_to_user(buffer, rawdata, 0x111);


			} else {
				DBG_INFO("RESPONSE: 0x%02x", respType);
			}
		}
	} else
	{
		count = -121;
		goto read_out;
	}

	/* getFrame in kl.py */
//	if (data[1] == 0x16) {
//		printk("Success!\n");
//		ret =
//		    usb_control_msg(hw->dev, usb_rcvctrlpipe(hw->dev, 0),
//				    USB_REQ_CLEAR_FEATURE,
//				    USB_TYPE_CLASS | USB_RECIP_INTERFACE |
//				    USB_DIR_IN, 0x3d6, 0, rawdata, 0x111,
//				    KL_USB_CTRL_TIMEOUT);
//		if (ret < 0) {
//			printk("Error in getFrame Nr: %d\n", ret);
//			count = -EIO;
//			goto read_out;
//		}
//		nbytes = (rawdata[1] << 8 | rawdata[2]) & 0x1ff;
//		printk("nbytes = %d\n", nbytes);
//
//		for (i = 0; i < nbytes; i++) {
//			retBuf[i] = rawdata[i + 3];
//		}
//		printk("getFrame = %02x %02x %02x %02x %02x %02x %02x %02x\n",
//		       rawdata[0], rawdata[1], rawdata[2], rawdata[3],
//		       rawdata[4], rawdata[5], rawdata[6], rawdata[7]);
//
//		printk("getFrame = %02x %02x %02x %02x %02x %02x %02x %02x\n",
//		       rawdata[8], rawdata[9], rawdata[10], rawdata[11],
//		       rawdata[12], rawdata[13], rawdata[14], rawdata[15]);
//
//		printk("getFrame = %02x %02x %02x %02x %02x %02x %02x %02x\n",
//		       rawdata[16], rawdata[17], rawdata[18], rawdata[19],
//		       rawdata[20], rawdata[21], rawdata[22], rawdata[23]);
//
//		printk("getFrame = %02x %02x %02x %02x %02x %02x %02x %02x\n",
//		       rawdata[24], rawdata[25], rawdata[26], rawdata[27],
//		       rawdata[28], rawdata[29], rawdata[30], rawdata[31]);
//
//		printk("retBuf   = %02x %02x %02x %02x %02x %02x %02x %02x\n",
//		       retBuf[0], retBuf[1], retBuf[2], retBuf[3],
//		       retBuf[4], retBuf[5], retBuf[6], retBuf[7]);
//
//		printk("retBuf   = %02x %02x %02x %02x %02x %02x %02x %02x\n",
//		       retBuf[8], retBuf[9], retBuf[10], retBuf[11],
//		       retBuf[12], retBuf[13], retBuf[14], retBuf[15]);
//
//		printk("retBuf   = %02x %02x %02x %02x %02x %02x %02x %02x\n",
//		       retBuf[16], retBuf[17], retBuf[18], retBuf[19],
//		       retBuf[20], retBuf[21], retBuf[22], retBuf[23]);
//
//		printk("retBuf   = %02x %02x %02x %02x %02x %02x %02x %02x\n",
//		       retBuf[24], retBuf[25], retBuf[26], retBuf[27],
//		       retBuf[28], retBuf[29], retBuf[30], retBuf[31]);
//
//		// generateResponse in kl.py
//		bufferID = (retBuf[0] << 8) | retBuf[1];
//		respType = (retBuf[3] & 0xF0);
//		printk("bufferID = %02x\n", bufferID);
//		printk("respType = %02x\n", respType);
//
//		if (bufferID == 0xF0F0) {
//			printk
//			    ("generateResponse: console not paired (synchronized)");
//		} else {
//			if (respType == RESPONSE_DATA_WRITTEN) {
//				printk("RESPONSE_DATA_WRITTEN\n");
//			} else if (respType == RESPONSE_GET_CONFIG) {
//				printk("RESPONSE_GET_CONFIG\n");
//			} else if (respType == RESPONSE_GET_CURRENT) {
//				printk("RESPONSE_GET_CURRENT\n");
//
//				// handleCurrentData in kl.py
//				cs = retBuf[6] | (retBuf[5] << 8);
//				printk("handleCurrentData: cs = %02x\n", cs);
//
//				// wird wohl eine Art Index für die Daten sein
//				haddr = 0xffffff;
//
//				// setFrame in kl.py
//				setFramebuf[0] = 0xd5;
//				setFramebuf[1] = 11 >> 8;
//				setFramebuf[2] = 11;
//
//				setFramebuf[3] = retBuf[0];
//				setFramebuf[4] = retBuf[1];
//				setFramebuf[5] = LOGGER_1;
//				setFramebuf[6] = ACTION_GET_HISTORY & 0xF;
//				setFramebuf[7] = (cs >> 8) & 0xFF;
//				setFramebuf[8] = (cs >> 0) & 0xFF;
//				setFramebuf[9] = 0x80;	// TODO: not known what this means
//				setFramebuf[10] = 8 & 0xFF;	// Annahme 8
//				setFramebuf[11] = (haddr >> 16) & 0xFF;
//				setFramebuf[12] = (haddr >> 8) & 0xFF;
//				setFramebuf[13] = (haddr >> 0) & 0xFF;
//
//				ret =
//				    usb_control_msg(hw->dev,
//						    usb_sndctrlpipe(hw->dev,
//								    0),
//						    USB_REQ_SET_CONFIGURATION,
//						    USB_TYPE_CLASS |
//						    USB_RECIP_INTERFACE
//						    | USB_DIR_OUT,
//						    0x3d5, 0, setFramebuf,
//						    0x111, KL_USB_CTRL_TIMEOUT);
//				if (ret < 0) {
//					printk("Error in setFrame Nr: %d\n",
//					       ret);
//				}
//				//  setTX in kl.py
//				ret =
//				    usb_control_msg(hw->dev,
//						    usb_sndctrlpipe(hw->dev, 0),
//						    USB_REQ_SET_CONFIGURATION,
//						    USB_TYPE_CLASS |
//						    USB_RECIP_INTERFACE |
//						    USB_DIR_OUT, 0x3d1, 0,
//						    setTXbuf, 0x15, KL_USB_CTRL_TIMEOUT);
//				if (ret < 0) {
//					printk("Error in setTX Nr: %d\n", ret);
//				}
//
//			} else if (respType == RESPONSE_GET_HISTORY) {
//				printk("RESPONSE_GET_HISTORY\n");
//
//				// handleHistoryData in kl.py
//				cs = retBuf[6] | (retBuf[5] << 8);
//				printk("handleHistoryData: cs = %02x\n", cs);
//
//				printk("Year   : %04d\n",
//				       (retBuf[176] >> 4) * 10 +
//				       (retBuf[176] & 0xF) * 1 + 2000);
//				resultBuf[2] = (retBuf[176] >> 4) * 10 +
//				    (retBuf[176] & 0xF) * 1;
//				printk("Month  : %02d\n",
//				       (retBuf[176 + 1] >> 4) * 10 +
//				       (retBuf[176 + 1] & 0xF) * 1);
//				resultBuf[1] = (retBuf[176 + 1] >> 4) * 10 +
//				    (retBuf[176 + 1] & 0xF) * 1;
//				printk("Days   : %02d\n",
//				       (retBuf[176 + 2] >> 4) * 10 +
//				       (retBuf[176 + 2] & 0xF) * 1);
//				resultBuf[0] = (retBuf[176 + 2] >> 4) * 10 +
//				    (retBuf[176 + 2] & 0xF) * 1;
//				printk("Hours  : %02d\n",
//				       (retBuf[176 + 3] >> 4) * 10 +
//				       (retBuf[176 + 3] & 0xF) * 1);
//				resultBuf[3] = (retBuf[176 + 3] >> 4) * 10 +
//				    (retBuf[176 + 3] & 0xF) * 1;
//				printk("Minutes: %02d\n",
//				       (retBuf[176 + 4] >> 4) * 10 +
//				       (retBuf[176 + 4] & 0xF) * 1);
//				resultBuf[4] = (retBuf[176 + 4] >> 4) * 10 +
//				    (retBuf[176 + 4] & 0xF) * 1;
//
//				//Humidity 161
//				printk("Humidity : %02d\n",
//				       (retBuf[161] >> 4) * 10 +
//				       (retBuf[161 + 0] & 0xF) * 1);
//				resultBuf[5] = (retBuf[161] >> 4) * 10 +
//				    (retBuf[161 + 0] & 0xF) * 1;
//				//Temp 174
//				printk("Temp (d Grad) : %03d\n",
//				       (retBuf[174] & 0xF) * 100 +
//				       (retBuf[174 + 1] >> 4) * 10 +
//				       (retBuf[174 + 1] & 0xF) * 1
//				       - TEMPERATURE_OFFSET);
//				resultBuf[6] = (retBuf[174] & 0xF) * 10 +
//				    (retBuf[174 + 1] >> 4) * 1;
//				resultBuf[7] = (retBuf[174 + 1] & 0xF) * 1;
//				atomic_set(&bytes_available, 8);
//
//				// bytes_to_addr
//				// index_to_addr
//				latestIndex =
//				    (((((retBuf[7] << 8) | retBuf[8]) << 8) |
//				      retBuf[9]) - 0x070000) / 32;
//				thisIndex =
//				    (((((retBuf[10] << 8) | retBuf[11]) << 8)
//				      | retBuf[12]) - 0x070000) / 32;
//				printk("latestIndex = %d\n", latestIndex);
//				printk("thisIndex = %d\n", thisIndex);
//
//				// buildACKFrame(buf, ACTION_GET_HISTORY, cs, nextIndex)
//				// setzt den neuen Index für die historischen Daten
//				//index_to_addr
//				//haddr =  32 * (latestIndex - 2*thisIndex)  + 0x070000;
//				klimaloggRecord = klimaloggRecord + 6;
//				haddr = 32 * (klimaloggRecord) + 0x070000;
//				printk("klimaloggRecord (postInc) = %d\n", klimaloggRecord);
//
//				// setFrame in kl.py
//				setFramebuf[0] = 0xd5;
//				setFramebuf[1] = 11 >> 8;
//				setFramebuf[2] = 11;
//				setFramebuf[3] = retBuf[0];
//				setFramebuf[4] = retBuf[1];
//				setFramebuf[5] = LOGGER_1;
//				setFramebuf[6] = ACTION_GET_HISTORY & 0xF;
//				setFramebuf[7] = (cs >> 8) & 0xFF;
//				setFramebuf[8] = (cs >> 0) & 0xFF;
//				setFramebuf[9] = 0x80;	// TODO: not known what this means
//				setFramebuf[10] = 8 & 0xFF;	// Annahme 8
//				setFramebuf[11] = (haddr >> 16) & 0xFF;
//				setFramebuf[12] = (haddr >> 8) & 0xFF;
//				setFramebuf[13] = (haddr >> 0) & 0xFF;
//				ret =
//				    usb_control_msg(hw->dev,
//						    usb_sndctrlpipe(hw->dev,
//						    		    0),
//						    USB_REQ_SET_CONFIGURATION,
//						    USB_TYPE_CLASS |
//						    USB_RECIP_INTERFACE
//						    | USB_DIR_OUT,
//						    0x3d5, 0, setFramebuf,
//						    0x111, KL_USB_CTRL_TIMEOUT);
//				if (ret < 0) {
//					printk("Error in setFrame Nr: %d\n",
//					       ret);
//				}
//				//  setTX in kl.py
//				ret =
//				    usb_control_msg(hw->dev,
//						    usb_sndctrlpipe(hw->dev, 0),
//						    USB_REQ_SET_CONFIGURATION,
//						    USB_TYPE_CLASS |
//						    USB_RECIP_INTERFACE |
//						    USB_DIR_OUT, 0x3d1, 0,
//						    setTXbuf, 0x15, KL_USB_CTRL_TIMEOUT);
//				if (ret < 0) {
//					printk("Error in setTX Nr: %d\n", ret);
//				}
//
//			} else if (respType == RESPONSE_REQUEST) {
//				printk("RESPONSE_REQUEST\n");
//			}
//		}
//		printk("\n");
//	}

//	ssleep(1);
//
//	to_copy = min((size_t) atomic_read(&bytes_available), count);
//	not_copied = copy_to_user(buffer, resultBuf, to_copy);
//	atomic_sub(to_copy - not_copied, &bytes_available);
//	count = to_copy - not_copied;

//	printk("to_copy: %d\n", (int)to_copy);
//	printk("not_copied: %d\n", (int)not_copied);
//	printk("Return at the end is: %lu\n", count);

read_out:
	mutex_unlock(&disconnect_mutex);	//TODO mutex destroy?
	kfree(resultBuf);
	kfree(setTXbuf);
	kfree(setFramebuf);
	kfree(rawdata);
	kfree(data);
	kfree(retBuf);
	return count;
}

static struct file_operations kl_fops = { .owner   = THIS_MODULE,
					  .write   = kl_write,
					  .read    = kl_read,
					  .open    = kl_open,
					  .release = kl_release };

static struct usb_class_driver kl_class = { .name = "kl%d",
		                            .fops = &kl_fops,
					    .minor_base = ML_MINOR_BASE };


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

static int kl_probe(struct usb_interface *intf,
		const struct usb_device_id *id)
{
	/* called when a USB device is connected to the computer. */

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

		if (((ep_desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK)
		     == USB_DIR_IN)
		    && ((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
		    	== USB_ENDPOINT_XFER_INT)) {
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

static void kl_disconnect(struct usb_interface *intf)
{
	/* called when unplugging a USB device. */
	struct kl_usb *hw;
	int minor;

	mutex_lock(&disconnect_mutex); /* Not interruptible */

	hw = usb_get_intfdata(intf);
	minor = hw->minor;

	release_hw(hw);

	/* TODO is this not already done in release_hw() ??? */
//	usb_set_intfdata(intf, NULL);


//	down(&hw->sem); /* Not interruptible */

	/* Give back our minor. */
	usb_deregister_dev(intf, &kl_class);


	mutex_unlock(&disconnect_mutex);

	DBG_INFO("USB KlimaLogg Pro /dev/kl%d now disconnected",
			minor - ML_MINOR_BASE);
}

static struct usb_driver kl_driver = { .name 	   = "klimalogg_driver",
				       .id_table   = kl_table,
				       .probe 	   = kl_probe,
				       .disconnect = kl_disconnect, };

static int __init usb_kl_init(void)
{
	/* called on module loading */
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

static void __exit usb_kl_exit(void)
{
	/* called on module unloading */
	usb_deregister(&kl_driver);
	DBG_INFO("module deregistered");
}

module_init(usb_kl_init);
module_exit(usb_kl_exit);
