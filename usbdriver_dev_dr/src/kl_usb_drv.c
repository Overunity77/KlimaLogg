#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>			/* kmalloc() */
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

//static LIST_HEAD(KLlist);
//static DEFINE_RWLOCK(KLlock);

static int klusb_cnt;

//struct usb_kl
//{
//	/* One structure for each connected device */
//	struct usb_device *dev;
//	struct usb_interface *interface;
//	unsigned char minor;
//	char serial_number[8];
//
//	int open_count; /* Open count for this port */
//	struct semaphore sem; /* Locks this structure */
//	spinlock_t cmd_spinlock; /* locks hw->command */
//
//	char *int_in_buffer;
//	struct usb_endpoint_descriptor *int_in_endpoint;
//	struct urb *int_in_urb;
//	int int_in_running;
//
//	char *ctrl_buffer; /* 8 byte buffer for ctrl msg */
//	struct urb *ctrl_urb;
//	struct usb_ctrlrequest *ctrl_dr; /* Setup packet information */
//	int correction_required;
//
//	char *ctrl_read_buffer;
//	char *ctrl_read_buffer2;
//	struct urb *ctrl_read_urb;
//	struct urb *ctrl_read_urb2;
//	struct usb_ctrlrequest *ctrl_read_dr;
//	struct usb_ctrlrequest *ctrl_read_dr2;
//	unsigned int ctrl_read_transfer_count; /* bytes transfered */
//
//	__u8 command;/* Last issued command */
//};

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
		return 1;
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

//static int write_reg(struct kl_usb *hw, struct klusb_ax5015_register_list *reg)
//{
//	int ret = 0;
//	reg->buf = kcalloc(KL_LEN_WRITE_REG, 1, GFP_KERNEL);
//
//	if (!reg->buf)
//		return -ENOMEM;
//
//	reg->buf[0] = 0xf0;
//	reg->buf[1] = reg->addr & 0x7f;
//	reg->buf[2] = 0x01;
//	reg->buf[3] = reg->value;
//	reg->buf[4] = 0x00;
//
//	ret =
//	    usb_control_msg(hw->dev,
//			    usb_sndctrlpipe(hw->dev, 0),
//			    USB_REQ_SET_CONFIGURATION,
//			    USB_TYPE_CLASS |
//			    USB_RECIP_INTERFACE |
//			    USB_DIR_OUT, 0x3f0, 0, reg->buf, 5, KL_USB_CTRL_TIMEOUT);
//	if (ret < 0) {
//		DBG_ERR("ret = %d", ret);
//	}
//	kl_debug_data(__FUNCTION__, KL_LEN_WRITE_REG, reg->buf);
//
//	kfree(reg->buf);
//
//	return 0;
//}



/* control completion routine handling background control cmds */
static void ctrl_complete(struct urb *urb)
{
	struct kl_usb *hw = (struct kl_usb *) urb->context;
	int ret;

//	/* The following flags are used internally by usbcore and HCDs */
//	#define URB_DIR_IN		0x0200	/* Transfer from device to host */
//	#define URB_DIR_OUT		0
//	#define URB_DIR_MASK		URB_DIR_IN

//	DBG_INFO("transfer flags: 0x%x, actual length: %d", urb->transfer_flags, urb->actual_length);
//	DBG_INFO("ctrl_complete buffer:");

	kl_debug_data(__FUNCTION__, urb->actual_length, urb->transfer_buffer);
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


static void kl_abort_transfers(struct kl_usb *hw)
{

//	if (!hw)
//	{
//		DBG_ERR("hw is NULL");
//		return;
//	}
//
//	if (!hw->dev)
//	{
//		DBG_ERR("dev is NULL");
//		return;
//	}
//
//	if (hw->dev->state == USB_STATE_NOTATTACHED)
//	{
//		DBG_ERR("dev not attached");
//		return;
//	}
//
//	/* Shutdown transfer */
//	if (hw->int_in_running)
//	{
//		hw->int_in_running = 0;
//		mb();
//		if (hw->int_in_urb)
//			usb_kill_urb(hw->int_in_urb);
//	}
//
//	if (hw->ctrl_urb)
//		usb_kill_urb(hw->ctrl_urb);
//
//
//
//
////	if (hw->ctrl_read_urb)
////		usb_kill_urb(hw->ctrl_read_urb);
////
////	if (hw->ctrl_read_urb2)
////		usb_kill_urb(hw->ctrl_read_urb2);
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

static void kl_ctrl_callback(struct urb *urb)
{
//	struct kl_usb *hw = urb->context;
//	hw->correction_required = 0; /* TODO: do we need race protection? */
//	kl_debug_data(__FUNCTION__, urb->actual_length, urb->transfer_buffer);
}

static void kl_ctrl_read_callback2(struct urb *urb)
{
//	struct kl_usb *hw = urb->context;
//	DBG_INFO(
//			"urb->status: %d, urb->transfer_flags: 0x%x, urb->transfer_dma: 0x%x, urb->error_count: %d",
//			urb->status, urb->transfer_flags,
//			(unsigned int )urb->transfer_dma, urb->error_count);
//	kl_debug_data(__FUNCTION__, urb->actual_length, urb->transfer_buffer);
//	hw->ctrl_read_transfer_count = urb->actual_length;
}

static void kl_ctrl_read_callback(struct urb *urb)
{
//	struct kl_usb *hw = urb->context;
//	int ret = 0;
//
//	DBG_INFO(
//			"urb->status: %d, urb->transfer_flags: 0x%x, urb->transfer_dma: 0x%x, urb->error_count: %d",
//			urb->status, urb->transfer_flags,
//			(unsigned int )urb->transfer_dma, urb->error_count);
//	kl_debug_data(__FUNCTION__, urb->actual_length, urb->transfer_buffer);
//
//	hw->ctrl_read_dr2->bRequestType = 0xa1;
//	hw->ctrl_read_dr2->bRequest = 0x01;
//	hw->ctrl_read_dr2->wValue = cpu_to_le16(0x03dc); /* convert to little-endian */
//	hw->ctrl_read_dr2->wIndex = cpu_to_le16(0);
//	hw->ctrl_read_dr2->wLength = cpu_to_le16(KL_CTRL_READ_BUFFER2_SIZE);
//
//	usb_fill_control_urb(hw->ctrl_read_urb2, hw->dev,
//			usb_sndctrlpipe(hw->dev, 0),
//			(unsigned char *) hw->ctrl_read_dr2,
//			hw->ctrl_read_buffer2,
//			KL_CTRL_READ_BUFFER2_SIZE, kl_ctrl_read_callback2, hw);
//
//	DBG_INFO("going to submit control urb");
//
//	ret = usb_submit_urb(hw->ctrl_read_urb2, GFP_ATOMIC);
//	if (ret)
//	{
//		DBG_ERR("submitting get_report control URB failed (%d)", ret);
//	}
}

static void kl_int_in_callback(struct urb *urb)
{
//	struct kl_usb *hw = urb->context;
//	int retval;
//
//	kl_debug_data(__FUNCTION__, urb->actual_length, urb->transfer_buffer);
//
//	DBG_INFO(
//			"urb->status = %d (-2 = -ENOENT: The urb was stopped by a call to usb_kill_urb)",
//			urb->status);
//
//	if (urb->status)
//	{
//		if (urb->status == -ENOENT || urb->status == -ECONNRESET
//				|| urb->status == -ESHUTDOWN)
//		{
//			return;
//		} else
//		{
//			DBG_ERR("non-zero urb status (%d)", urb->status);
//			goto resubmit;
//			/* Maybe we can recover. */
//		}
//	}
//
//	if (urb->actual_length > 0)
//	{
//		spin_lock(&hw->cmd_spinlock);
//
//		if (hw->int_in_buffer[0] & ML_MAX_UP && hw->command & ML_UP)
//		{
//			hw->command &= ~ML_UP;
//			hw->correction_required = 1;
//		} else if (hw->int_in_buffer[0] & ML_MAX_DOWN
//				&& hw->command & ML_DOWN)
//		{
//			hw->command &= ~ML_DOWN;
//			hw->correction_required = 1;
//		}
//
//		if (hw->int_in_buffer[1] & ML_MAX_LEFT
//				&& hw->command & ML_LEFT)
//		{
//			hw->command &= ~ML_LEFT;
//			hw->correction_required = 1;
//		} else if (hw->int_in_buffer[1] & ML_MAX_RIGHT
//				&& hw->command & ML_RIGHT)
//		{
//			hw->command &= ~ML_RIGHT;
//			hw->correction_required = 1;
//		}
//
//		if (hw->correction_required)
//		{
//			hw->ctrl_buffer[0] = hw->command;
//			spin_unlock(&hw->cmd_spinlock);
//			retval = usb_submit_urb(hw->ctrl_urb, GFP_ATOMIC);
//			if (retval)
//			{
//				DBG_ERR(
//						"submitting correction control URB failed (%d)",
//						retval);
//			}
//		} else
//		{
//			spin_unlock(&hw->cmd_spinlock);
//		}
//	}
//
//	resubmit:
//	/* Resubmit if we're still running. */
//	if (hw->int_in_running && hw->dev)
//	{
//		retval = usb_submit_urb(hw->int_in_urb, GFP_ATOMIC);
//		if (retval)
//		{
//			DBG_ERR("resubmitting urb failed (%d)", retval);
//			hw->int_in_running = 0;
//		}
//	}
}



static int kl_release(struct inode *inode, struct file *file)
{
//	/* close syscall */
//	struct kl_usb *hw = NULL;
//	int retval = 0;
//
//	DBG_INFO("Release driver");
//	hw = file->private_data;
//
//	if (!hw)
//	{
//		DBG_ERR("hw is NULL");
//		retval = -ENODEV;
//		goto exit;
//	}
//
//	/* Lock our device */
//	if (down_interruptible(&hw->sem))
//	{
//		retval = -ERESTARTSYS;
//		goto exit;
//	}
//
//	if (hw->open_count <= 0)
//	{
//		DBG_ERR("device not opened");
//		retval = -ENODEV;
//		goto unlock_exit;
//	}
//
//	if (!hw->dev)
//	{
//		DBG_DEBUG("device unplugged before the file was released");
//		up(&hw->sem); /* Unlock here as kl_delete frees hw. */
//		kl_delete(hw);
//		goto exit;
//	}
//
//	if (hw->open_count > 1)
//		DBG_DEBUG("open_count = %d", hw->open_count);
//
//	kl_abort_transfers(hw);
//	--hw->open_count;
//
//	unlock_exit: up(&hw->sem);
//
//	exit: return retval;
}


/* receive completion routine for all interrupt rx fifos */
static void rx_int_complete(struct urb *urb)
{
	int len, status, i;
	__u8 *buf, maxlen, fifon;
	struct usb_fifo *fifo = (struct usb_fifo *) urb->context;
	struct kl_usb *hw = fifo->hw;
	static __u8 eof[8];

	spin_lock(&hw->lock);
	if (fifo->stop_gracefull) {
		fifo->stop_gracefull = 0;
		fifo->active = 0;
		spin_unlock(&hw->lock);
		return;
	}
	spin_unlock(&hw->lock);

	fifon = fifo->fifonum;
	if ((!fifo->active) || (urb->status)) {
		DBG_ERR("RX-Fifo %i is going down (%i)", fifon, urb->status);

		fifo->urb->interval = 0; /* cancel automatic rescheduling */
		return;
	}
	len = urb->actual_length;
	buf = fifo->buffer;
	maxlen = fifo->usb_packet_maxlen;

	if (urb->status) {
		DBG_INFO("urb->status = %d", urb->status);
		if (urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN) {
			return;
		} else {
			DBG_ERR("non-zero urb status (%d)", urb->status);
			goto resubmit; /* Maybe we can recover. */
		}
	}


	/* USB data log for RX INT in */
	DBG_INFO("RX INT len(%d", len);
	kl_debug_data(__FUNCTION__, len + 16, buf);

	fifo->last_urblen = urb->actual_length;

resubmit:
	status = 0;
// TODO re-enable:	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status) {
		DBG_ERR("error resubmitting USB Int urb");
	}
}


static void stop_int_gracefull(struct usb_fifo *fifo)
{
	struct kl_usb *hw = fifo->hw;
	int timeout;
	u_long flags;

	spin_lock_irqsave(&hw->lock, flags);
	DBG_INFO("stopping interrupt gracefull for fifo %i", fifo->fifonum);

	if (fifo->urb) {
		usb_kill_urb(fifo->urb);
		usb_free_urb(fifo->urb);
	}

//	fifo->stop_gracefull = 1;

	spin_unlock_irqrestore(&hw->lock, flags);

//	timeout = 3;
//	while (fifo->stop_gracefull && timeout--)
//		schedule_timeout_interruptible((HZ / 1000) * 3);
//	if (fifo->stop_gracefull)
//		DBG_ERR("ERROR stopping interrupt gracefull for fifo %i", fifo->fifonum);
}

/* start the interrupt transfer for the given fifo */
static void start_int_fifo(struct usb_fifo *fifo)
{
	struct kl_usb *hw = fifo->hw;
	int errcode;

	DBG_INFO("starting interrupt transfer: INT IN fifo:%d", fifo->fifonum);

	if (!fifo->urb)
	{
		fifo->urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!fifo->urb)
			return;
	}
	usb_fill_int_urb(fifo->urb, fifo->hw->dev, fifo->pipe,
			 fifo->buffer, fifo->usb_packet_maxlen,
			 (usb_complete_t)rx_int_complete, fifo, fifo->intervall);
	fifo->active = 1;
	fifo->stop_gracefull = 0;
	errcode = usb_submit_urb(fifo->urb, GFP_KERNEL);
	if (errcode)
	{
		DBG_ERR("submit URB: status:%i", errcode);
		fifo->active = 0;
	}
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
	struct usb_fifo *fifo;
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

//	    def doRFSetup(self):
//	        self.hid.execute(5)
//	        self.hid.setPreamblePattern(0xaa)
//	        self.hid.setState(0)
//	        time.sleep(1)
//	        self.hid.setRX()
//
//	        self.hid.setPreamblePattern(0xaa)
//	        self.hid.setState(0x1e)
//	        time.sleep(1)
//	        self.hid.setRX()
//	        self.setSleep(0.075, 0.005)



//	write_usb_ctrl(hw,
//		       kl_msg_type[KL_READ_CONFIG_FLASH_OUT].msg_type,
//		       kl_msg_type[KL_READ_CONFIG_FLASH_OUT].length,
//		       hw->frequency_corr.writeData);
//
//
//
//
//	read_usb_ctrl(hw,
//		      kl_msg_type[KL_READ_CONFIG_FLASH_IN].msg_type,
//		      kl_msg_type[KL_READ_CONFIG_FLASH_IN].length,
//		      hw->frequency_corr.readBuf);


//	/* do Chip reset */
//	write_reg(hw, HFCUSB_CIRM, 8);
//
//	/* aux = output, reset off */
//	write_reg(hw, HFCUSB_CIRM, 0x10);
//
//	/* set USB_SIZE to match the wMaxPacketSize for INT or BULK transfers */
//	write_reg(hw, HFCUSB_USB_SIZE, (hw->packet_size / 8) |
//		  ((hw->packet_size / 8) << 4));
//
//	/* set USB_SIZE_I to match the the wMaxPacketSize for ISO transfers */
//	write_reg(hw, HFCUSB_USB_SIZE_I, hw->iso_packet_size);
//
//	/* enable PCM/GCI master mode */
//	write_reg(hw, HFCUSB_MST_MODE1, 0);	/* set default values */
//	write_reg(hw, HFCUSB_MST_MODE0, 1);	/* enable master mode */
//
//	/* init the fifos */
//	write_reg(hw, HFCUSB_F_THRES,
//		  (HFCUSB_TX_THRESHOLD / 8) | ((HFCUSB_RX_THRESHOLD / 8) << 4));
//
//	fifo = hw->fifos;
//	for (i = 0; i < HFCUSB_NUM_FIFOS; i++) {
//		write_reg(hw, HFCUSB_FIFO, i);	/* select the desired fifo */
//		fifo[i].max_size =
//			(i <= HFCUSB_B2_RX) ? MAX_BCH_SIZE : MAX_DFRAME_LEN;
//		fifo[i].last_urblen = 0;
//
//		/* set 2 bit for D- & E-channel */
//		write_reg(hw, HFCUSB_HDLC_PAR, ((i <= HFCUSB_B2_RX) ? 0 : 2));
//
//		/* enable all fifos */
//		if (i == HFCUSB_D_TX)
//			write_reg(hw, HFCUSB_CON_HDLC,
//				  (hw->protocol == ISDN_P_NT_S0) ? 0x08 : 0x09);
//		else
//			write_reg(hw, HFCUSB_CON_HDLC, 0x08);
//		write_reg(hw, HFCUSB_INC_RES_F, 2); /* reset the fifo */
//	}
//
//	write_reg(hw, HFCUSB_SCTRL_R, 0); /* disable both B receivers */
//	handle_led(hw, LED_POWER_ON);
}

/* Hardware Initialization */
static int setup_klusb(struct kl_usb *hw)
{
	u_char b;
	int ret;

	DBG_INFO("%s",__func__);


//	/* check the chip id */
//	if (read_reg_atomic(hw, HFCUSB_CHIP_ID, &b) != 1) {
//		printk(KERN_DEBUG "%s: %s: cannot read chip id\n",
//		       hw->name, __func__);
//		return 1;
//	}
//	if (b != HFCUSB_CHIPID) {
//		printk(KERN_DEBUG "%s: %s: Invalid chip id 0x%02x\n",
//		       hw->name, __func__, b);
//		return 1;
//	}

//	/* first set the needed config, interface and alternate */
//	(void) usb_set_interface(hw->dev, hw->if_used, hw->alt_used);

	// TODO re-enable
//	start_int_fifo(hw->fifos + KLUSB_INT_RX);

	/* init the background machinery for control requests */
	hw->ctrl_urb->dev = hw->dev;
	hw->ctrl_urb->complete = (usb_complete_t)ctrl_complete;
	hw->ctrl_urb->context = hw;





//	hw->ctrl_write.bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT;
//	hw->ctrl_write.bRequest = USB_REQ_SET_CONFIGURATION;
//	hw->ctrl_write.wValue = cpu_to_le16(0x03dd); /* convert to little-endian */
//	hw->ctrl_write.wIndex = cpu_to_le16(0);
//	hw->ctrl_write.wLength = cpu_to_le16(KL_CTRL_READ_BUFFER_SIZE);
//
//	usb_fill_control_urb(hw->ctrl_urb,
//			     hw->dev,
//			     hw->ctrl_out_pipe,
//			     (u_char *)&hw->ctrl_write,
//			     hw->fifos[1].buffer,
//			     KL_CTRL_READ_BUFFER_SIZE,
//			     (usb_complete_t)ctrl_complete,
//			     hw);
//
//
//
//	kl_debug_data(__FUNCTION__, KL_CTRL_READ_BUFFER_SIZE, hw->fifos[1].buffer);
//
//	DBG_INFO("going to submit control write urb");
//
//	ret = usb_submit_urb(hw->ctrl_urb, GFP_ATOMIC);
//	if (ret)
//	{
//		DBG_ERR("submitting write control URB failed (%d)", ret);
//	}



//	/* init the background machinery for control requests */
//	hw->ctrl_read.bRequestType = 0xc0;
//	hw->ctrl_read.bRequest = 1;
//	hw->ctrl_read.wLength = cpu_to_le16(1);
//	hw->ctrl_write.bRequestType = 0x40;
//	hw->ctrl_write.bRequest = 0;
//	hw->ctrl_write.wLength = 0;
//	usb_fill_control_urb(hw->ctrl_urb, hw->dev, hw->ctrl_out_pipe,
//			     (u_char *)&hw->ctrl_write, NULL, 0,
//			     (usb_complete_t)ctrl_complete, hw);

	ret = initTranseiver(hw);
	return ret;
}

static void release_hw(struct kl_usb *hw)
{
	DBG_INFO("%s", __func__);


	/* rx endpoint using USB INT IN method */
	stop_int_gracefull(hw->fifos + KLUSB_INT_RX);

	/* rx/tx control endpoint */
	if (hw->ctrl_urb) {
		usb_kill_urb(hw->ctrl_urb);
		usb_free_urb(hw->ctrl_urb);
		hw->ctrl_urb = NULL;
	}

	if (hw->intf)
		usb_set_intfdata(hw->intf, NULL);
//	list_del(&hw->list);
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
	int err;

	DBG_INFO("Open device");
	subminor = iminor(inode);

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

	/* lock this device */
//	if (down_interruptible(&hw->sem))
//	{
//		DBG_ERR("sem down failed");
//		retval = -ERESTARTSYS;
//		goto exit;
//	}

	/* Increment our usage count for the device. */
	++hw->open_count;
	if (hw->open_count > 1)
		DBG_DEBUG("open_count = %d", hw->open_count);

//	/* Initialize interrupt URB. */
//	usb_fill_int_urb(hw->int_in_urb, hw->dev,
//			usb_rcvintpipe(hw->dev,
//					hw->int_in_endpoint->bEndpointAddress),
//			hw->int_in_buffer,
//			le16_to_cpu(hw->int_in_endpoint->wMaxPacketSize),
//			kl_int_in_callback, hw,
//			hw->int_in_endpoint->bInterval);
//
//	hw->int_in_running = 1;
//	mb();
//
//	retval = usb_submit_urb(hw->int_in_urb, GFP_KERNEL);
//	if (retval)
//	{
//		DBG_ERR("submitting int urb failed (%d)", retval);
//		hw->int_in_running = 0;
//		--hw->open_count;
//		goto unlock_exit;
//	}


	/* Save our object in the file's private structure. */
	file->private_data = hw;

unlock_exit:
//	up(&hw->sem);

exit:
	mutex_unlock(&disconnect_mutex);
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

	printk("Count is: %lu\n", count);

	if (!retBuf || !data || !rawdata || !setFramebuf || !setTXbuf || !resultBuf) {
		printk("no memory");
		count = -ENOMEM;
		goto read_out;
	}




	DBG_INFO("read command, read by \"%s\" (pid %i), size=%lu",
			current->comm, current->pid, (unsigned long ) count);

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
	ret =
	    usb_control_msg(hw->dev, usb_rcvctrlpipe(hw->dev, 0), USB_REQ_CLEAR_FEATURE,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
			    0x3de, 0, data, 10, KL_USB_CTRL_TIMEOUT);

	if (ret < 0) {
		printk("Error read Nr: %d\n", ret);
		count = -EIO;
		goto read_out;
	}

	printk("read nach= %02x %02x %02x %02x\n", data[0], data[1], data[2],
	       data[3]);
	/* getFrame in kl.py */
	if (data[1] == 0x16) {
		printk("Success!\n");
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
		nbytes = (rawdata[1] << 8 | rawdata[2]) & 0x1ff;
		printk("nbytes = %d\n", nbytes);

		for (i = 0; i < nbytes; i++) {
			retBuf[i] = rawdata[i + 3];
		}
		printk("getFrame = %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       rawdata[0], rawdata[1], rawdata[2], rawdata[3],
		       rawdata[4], rawdata[5], rawdata[6], rawdata[7]);

		printk("getFrame = %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       rawdata[8], rawdata[9], rawdata[10], rawdata[11],
		       rawdata[12], rawdata[13], rawdata[14], rawdata[15]);

		printk("getFrame = %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       rawdata[16], rawdata[17], rawdata[18], rawdata[19],
		       rawdata[20], rawdata[21], rawdata[22], rawdata[23]);

		printk("getFrame = %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       rawdata[24], rawdata[25], rawdata[26], rawdata[27],
		       rawdata[28], rawdata[29], rawdata[30], rawdata[31]);

		printk("retBuf   = %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       retBuf[0], retBuf[1], retBuf[2], retBuf[3],
		       retBuf[4], retBuf[5], retBuf[6], retBuf[7]);

		printk("retBuf   = %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       retBuf[8], retBuf[9], retBuf[10], retBuf[11],
		       retBuf[12], retBuf[13], retBuf[14], retBuf[15]);

		printk("retBuf   = %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       retBuf[16], retBuf[17], retBuf[18], retBuf[19],
		       retBuf[20], retBuf[21], retBuf[22], retBuf[23]);

		printk("retBuf   = %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       retBuf[24], retBuf[25], retBuf[26], retBuf[27],
		       retBuf[28], retBuf[29], retBuf[30], retBuf[31]);

		// generateResponse in kl.py
		bufferID = (retBuf[0] << 8) | retBuf[1];
		respType = (retBuf[3] & 0xF0);
		printk("bufferID = %02x\n", bufferID);
		printk("respType = %02x\n", respType);

		if (bufferID == 0xF0F0) {
			printk
			    ("generateResponse: console not paired (synchronized)");
		} else {
			if (respType == RESPONSE_DATA_WRITTEN) {
				printk("RESPONSE_DATA_WRITTEN\n");
			} else if (respType == RESPONSE_GET_CONFIG) {
				printk("RESPONSE_GET_CONFIG\n");
			} else if (respType == RESPONSE_GET_CURRENT) {
				printk("RESPONSE_GET_CURRENT\n");

				// handleCurrentData in kl.py
				cs = retBuf[6] | (retBuf[5] << 8);
				printk("handleCurrentData: cs = %02x\n", cs);

				// wird wohl eine Art Index für die Daten sein
				haddr = 0xffffff;

				// setFrame in kl.py
				setFramebuf[0] = 0xd5;
				setFramebuf[1] = 11 >> 8;
				setFramebuf[2] = 11;

				setFramebuf[3] = retBuf[0];
				setFramebuf[4] = retBuf[1];
				setFramebuf[5] = LOGGER_1;
				setFramebuf[6] = ACTION_GET_HISTORY & 0xF;
				setFramebuf[7] = (cs >> 8) & 0xFF;
				setFramebuf[8] = (cs >> 0) & 0xFF;
				setFramebuf[9] = 0x80;	// TODO: not known what this means
				setFramebuf[10] = 8 & 0xFF;	// Annahme 8
				setFramebuf[11] = (haddr >> 16) & 0xFF;
				setFramebuf[12] = (haddr >> 8) & 0xFF;
				setFramebuf[13] = (haddr >> 0) & 0xFF;

				ret =
				    usb_control_msg(hw->dev,
						    usb_sndctrlpipe(hw->dev,
								    0),
						    USB_REQ_SET_CONFIGURATION,
						    USB_TYPE_CLASS |
						    USB_RECIP_INTERFACE
						    | USB_DIR_OUT,
						    0x3d5, 0, setFramebuf,
						    0x111, KL_USB_CTRL_TIMEOUT);
				if (ret < 0) {
					printk("Error in setFrame Nr: %d\n",
					       ret);
				}
				//  setTX in kl.py
				ret =
				    usb_control_msg(hw->dev,
						    usb_sndctrlpipe(hw->dev, 0),
						    USB_REQ_SET_CONFIGURATION,
						    USB_TYPE_CLASS |
						    USB_RECIP_INTERFACE |
						    USB_DIR_OUT, 0x3d1, 0,
						    setTXbuf, 0x15, KL_USB_CTRL_TIMEOUT);
				if (ret < 0) {
					printk("Error in setTX Nr: %d\n", ret);
				}

			} else if (respType == RESPONSE_GET_HISTORY) {
				printk("RESPONSE_GET_HISTORY\n");

				// handleHistoryData in kl.py
				cs = retBuf[6] | (retBuf[5] << 8);
				printk("handleHistoryData: cs = %02x\n", cs);

				printk("Year   : %04d\n",
				       (retBuf[176] >> 4) * 10 +
				       (retBuf[176] & 0xF) * 1 + 2000);
				resultBuf[2] = (retBuf[176] >> 4) * 10 +
				    (retBuf[176] & 0xF) * 1;
				printk("Month  : %02d\n",
				       (retBuf[176 + 1] >> 4) * 10 +
				       (retBuf[176 + 1] & 0xF) * 1);
				resultBuf[1] = (retBuf[176 + 1] >> 4) * 10 +
				    (retBuf[176 + 1] & 0xF) * 1;
				printk("Days   : %02d\n",
				       (retBuf[176 + 2] >> 4) * 10 +
				       (retBuf[176 + 2] & 0xF) * 1);
				resultBuf[0] = (retBuf[176 + 2] >> 4) * 10 +
				    (retBuf[176 + 2] & 0xF) * 1;
				printk("Hours  : %02d\n",
				       (retBuf[176 + 3] >> 4) * 10 +
				       (retBuf[176 + 3] & 0xF) * 1);
				resultBuf[3] = (retBuf[176 + 3] >> 4) * 10 +
				    (retBuf[176 + 3] & 0xF) * 1;
				printk("Minutes: %02d\n",
				       (retBuf[176 + 4] >> 4) * 10 +
				       (retBuf[176 + 4] & 0xF) * 1);
				resultBuf[4] = (retBuf[176 + 4] >> 4) * 10 +
				    (retBuf[176 + 4] & 0xF) * 1;

				//Humidity 161
				printk("Humidity : %02d\n",
				       (retBuf[161] >> 4) * 10 +
				       (retBuf[161 + 0] & 0xF) * 1);
				resultBuf[5] = (retBuf[161] >> 4) * 10 +
				    (retBuf[161 + 0] & 0xF) * 1;
				//Temp 174
				printk("Temp (d Grad) : %03d\n",
				       (retBuf[174] & 0xF) * 100 +
				       (retBuf[174 + 1] >> 4) * 10 +
				       (retBuf[174 + 1] & 0xF) * 1
				       - TEMPERATURE_OFFSET);
				resultBuf[6] = (retBuf[174] & 0xF) * 10 +
				    (retBuf[174 + 1] >> 4) * 1;
				resultBuf[7] = (retBuf[174 + 1] & 0xF) * 1;
				atomic_set(&bytes_available, 8);

				// bytes_to_addr
				// index_to_addr
				latestIndex =
				    (((((retBuf[7] << 8) | retBuf[8]) << 8) |
				      retBuf[9]) - 0x070000) / 32;
				thisIndex =
				    (((((retBuf[10] << 8) | retBuf[11]) << 8)
				      | retBuf[12]) - 0x070000) / 32;
				printk("latestIndex = %d\n", latestIndex);
				printk("thisIndex = %d\n", thisIndex);

				// buildACKFrame(buf, ACTION_GET_HISTORY, cs, nextIndex)
				// setzt den neuen Index für die historischen Daten
				//index_to_addr
				//haddr =  32 * (latestIndex - 2*thisIndex)  + 0x070000;
				klimaloggRecord = klimaloggRecord + 6;
				haddr = 32 * (klimaloggRecord) + 0x070000;
				printk("klimaloggRecord (postInc) = %d\n", klimaloggRecord);

				// setFrame in kl.py
				setFramebuf[0] = 0xd5;
				setFramebuf[1] = 11 >> 8;
				setFramebuf[2] = 11;
				setFramebuf[3] = retBuf[0];
				setFramebuf[4] = retBuf[1];
				setFramebuf[5] = LOGGER_1;
				setFramebuf[6] = ACTION_GET_HISTORY & 0xF;
				setFramebuf[7] = (cs >> 8) & 0xFF;
				setFramebuf[8] = (cs >> 0) & 0xFF;
				setFramebuf[9] = 0x80;	// TODO: not known what this means
				setFramebuf[10] = 8 & 0xFF;	// Annahme 8
				setFramebuf[11] = (haddr >> 16) & 0xFF;
				setFramebuf[12] = (haddr >> 8) & 0xFF;
				setFramebuf[13] = (haddr >> 0) & 0xFF;
				ret =
				    usb_control_msg(hw->dev,
						    usb_sndctrlpipe(hw->dev,
						    		    0),
						    USB_REQ_SET_CONFIGURATION,
						    USB_TYPE_CLASS |
						    USB_RECIP_INTERFACE
						    | USB_DIR_OUT,
						    0x3d5, 0, setFramebuf,
						    0x111, KL_USB_CTRL_TIMEOUT);
				if (ret < 0) {
					printk("Error in setFrame Nr: %d\n",
					       ret);
				}
				//  setTX in kl.py
				ret =
				    usb_control_msg(hw->dev,
						    usb_sndctrlpipe(hw->dev, 0),
						    USB_REQ_SET_CONFIGURATION,
						    USB_TYPE_CLASS |
						    USB_RECIP_INTERFACE |
						    USB_DIR_OUT, 0x3d1, 0,
						    setTXbuf, 0x15, KL_USB_CTRL_TIMEOUT);
				if (ret < 0) {
					printk("Error in setTX Nr: %d\n", ret);
				}

			} else if (respType == RESPONSE_REQUEST) {
				printk("RESPONSE_REQUEST\n");
			}
		}
		printk("\n");
	}

	ssleep(1);

	to_copy = min((size_t) atomic_read(&bytes_available), count);
	not_copied = copy_to_user(buffer, resultBuf, to_copy);
	atomic_sub(to_copy - not_copied, &bytes_available);
	count = to_copy - not_copied;

	printk("to_copy: %d\n", (int)to_copy);
	printk("not_copied: %d\n", (int)not_copied);
	printk("Return at the end is: %lu\n", count);

read_out:
	mutex_unlock(&disconnect_mutex);
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
					  .release = kl_release, };

static struct usb_class_driver kl_class = { .name = "kl%d",
		                            .fops = &kl_fops,
					    .minor_base = ML_MINOR_BASE };


static int setup_instance(struct kl_usb *hw)
{
	u_long	flags;
	int	err = 0;
	int	i;

	DBG_INFO("%s", __func__);

//	sema_init(&hw->sem, 1);

	spin_lock_init(&hw->ctrl_lock);
	spin_lock_init(&hw->lock);

	err = setup_klusb(hw);
	if (err)
		goto out;

//	snprintf(hw->name, MISDN_MAX_IDLEN - 1, "%s.%d", DRIVER_NAME,
//		 hfcsusb_cnt + 1);
//	printk(KERN_INFO "%s: registered as '%s'\n",
//	       DRIVER_NAME, hw->name);
//
//	err = mISDN_register_device(&hw->dch.dev, parent, hw->name);
//	if (err)
//		goto out;

	klusb_cnt++;
//	write_lock_irqsave(&KLlock, flags);
//	list_add_tail(&hw->list, &KLlist);
//	write_unlock_irqrestore(&KLlock, flags);
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
	struct usb_host_interface	*iface = intf->cur_altsetting;
	struct usb_host_endpoint	*ep;

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


	ep = iface->endpoint;


	/* Set up interrupt endpoint information. */
	if ( (iface->desc.bNumEndpoints == 1) &&
	     ((ep->desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
	       == USB_ENDPOINT_XFER_INT) )
	{
		struct usb_fifo *f;
		f = &hw->fifos[KLUSB_INT_RX];

		f->pipe = usb_rcvintpipe(dev,
					 ep->desc.bEndpointAddress);
		f->usb_transfer_mode = USB_INT;
		int_end_packet_size = le16_to_cpu(ep->desc.wMaxPacketSize);
		DBG_INFO("int_end_packet_size = %d", int_end_packet_size);
		if (f->pipe)
		{
			f->fifonum = KLUSB_INT_RX;
			f->hw = hw;
			f->usb_packet_maxlen = int_end_packet_size;
			f->intervall = ep->desc.bInterval;
		}
	}
	else
	{
		DBG_ERR("could not find interrupt in endpoint");
		retval = -ENODEV;
		goto error;
	}


	hw->dev = dev; /* save device */
	hw->ctrl_paksize = dev->descriptor.bMaxPacketSize0; /* control size */
	hw->packet_size = int_end_packet_size;

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
	struct kl_usb *next;
	int minor;
	int cnt = 0;

	mutex_lock(&disconnect_mutex); /* Not interruptible */

	hw = usb_get_intfdata(intf);
	minor = hw->minor;

	release_hw(hw);

//	list_for_each_entry_safe(hw, next, &KLlist, list)
//		cnt++;
//	if (!cnt)
//		klusb_cnt = 0;

	klusb_cnt = 0;

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
