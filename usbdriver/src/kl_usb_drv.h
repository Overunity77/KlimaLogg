/*
 * kl_usb_drv.h
 *
 *  Created on: Sep 9, 2015
 *      Author: D. Reimann
 */


#ifndef __KL_USB_DRV_H__
#define __KL_USB_DRV_H__

#define KL_VENDOR_ID	0x6666
#define KL_PRODUCT_ID	0x5555




/* Logger ID's */
#define LOGGER_1			0
#define LOGGER_2			1
#define LOGGER_3			2
#define LOGGER_4			3
#define LOGGER_5			4
#define LOGGER_6			5
#define LOGGER_7			6
#define LOGGER_8			7
#define LOGGER_9			8
#define LOGGER_10			9

/* Response types */
#define RESPONSE_DATA_WRITTEN		0x10
#define RESPONSE_GET_CONFIG		0x20
#define RESPONSE_GET_CURRENT		0x30
#define RESPONSE_GET_HISTORY		0x40
#define RESPONSE_REQUEST		0x50
#define RESPONSE_REQ_READ_HISTORY	0x50
#define RESPONSE_REQ_FIRST_CONFIG	0x51
#define RESPONSE_REQ_SET_CONFIG		0x52
#define RESPONSE_REQ_SET_TIME		0x53

/* Action types */
#define ACTION_GET_HISTORY		0x00
#define ACTION_REQ_SET_TIME		0x01
#define ACTION_REQ_SET_CONFIG		0x02
#define ACTION_GET_CONFIG		0x03
#define ACTION_GET_CURRENT		0x04
#define ACTION_SEND_CONFIG		0x20
#define ACTION_SEND_TIME		0x60

/* RF Frequency */
#define KL_FREQUENCY_EU			868300000	/* 868.3 MHz */
#define KL_FREQ_VAL			910478540	/* (KL_FREQUENCY_EU / 16000000.0 * 16777216.0) */


#define USB_INT				0
#define USB_HID_FEATURE_REPORT		0x03

#define KL_MAX_RECORD			51200
#define KL_COMM_INT			8
#define KL_USB_CTRL_TIMEOUT		5 	/* 5ms timeout writing/reading regs */
#define KL_USB_CTRL_BUFSIZE 		64
#define KL_CTRL_READ_REQUEST_TYPE	0x21
#define KL_CTRL_SET_REPORT_REQUEST	0x09

/* KlimaLogg pro Message Types */
#define KL_MSG_SET_RX			0xd0	// SET_RX
#define KL_LEN_SET_RX			0x15	// (Length:  21)
#define KL_MSG_SET_TX			0xd1	// SET_TX
#define KL_LEN_SET_TX			0x15	// (Length:  21)
#define KL_MSG_SET_FRAME		0xd5	// SET_FRAME
#define KL_LEN_SET_FRAME		0x111	// (Length: 273)
#define KL_MSG_GET_FRAME		0xd6	// GET_FRAME
#define KL_LEN_GET_FRAME		0x111	// (Length: 273)
#define KL_MSG_SET_STATE		0xd7	// SET_STATE
#define KL_LEN_SET_STATE		0x15	// (Length:  21)
#define KL_MSG_SET_PREAMBLE_PATTERN	0xd8	// SET_PREAMBLE_PATTERN
#define KL_LEN_SET_PREAMBLE_PATTERN	0x15	// (Length:  21)
#define KL_MSG_EXECUTE			0xd9	// EXECUTE
#define KL_LEN_EXECUTE			0x0f	// (Length:  15)
#define KL_MSG_READ_CONFIG_FLASH_IN	0xdc	// READ_CONFIG_FLASH_IN
#define KL_LEN_READ_CONFIG_FLASH_IN	0x15	// (Length:  21)
#define KL_MSG_READ_CONFIG_FLASH_OUT	0xdd	// READ_CONFIG_FLASH_OUT
#define KL_LEN_READ_CONFIG_FLASH_OUT	0x0f	// (Length:  15)
#define KL_MSG_GET_STATE		0xde	// GET_STATE
#define KL_LEN_GET_STATE		0x0a	// (Length:  10)
#define KL_MSG_WRITE_REG		0xf0	// WRITE_REG
#define KL_LEN_WRITE_REG		0x05	// (Length:   5)


/* AX5051 register addresses */
#define AX5051REGISTER_REVISION		0x00
#define AX5051REGISTER_SCRATCH		0x01
#define AX5051REGISTER_POWERMODE	0x02
#define AX5051REGISTER_XTALOSC		0x03
#define AX5051REGISTER_FIFOCTRL		0x04
#define AX5051REGISTER_FIFODATA		0x05
#define AX5051REGISTER_IRQMASK		0x06
#define AX5051REGISTER_IFMODE		0x08
#define AX5051REGISTER_PINCFG1		0x0C
#define AX5051REGISTER_PINCFG2		0x0D
#define AX5051REGISTER_MODULATION	0x10
#define AX5051REGISTER_ENCODING		0x11
#define AX5051REGISTER_FRAMING		0x12
#define AX5051REGISTER_CRCINIT3		0x14
#define AX5051REGISTER_CRCINIT2		0x15
#define AX5051REGISTER_CRCINIT1		0x16
#define AX5051REGISTER_CRCINIT0		0x17
#define AX5051REGISTER_FREQ3		0x20
#define AX5051REGISTER_FREQ2		0x21
#define AX5051REGISTER_FREQ1		0x22
#define AX5051REGISTER_FREQ0		0x23
#define AX5051REGISTER_FSKDEV2		0x25
#define AX5051REGISTER_FSKDEV1		0x26
#define AX5051REGISTER_FSKDEV0		0x27
#define AX5051REGISTER_IFFREQHI		0x28
#define AX5051REGISTER_IFFREQLO		0x29
#define AX5051REGISTER_PLLLOOP		0x2C
#define AX5051REGISTER_PLLRANGING	0x2D
#define AX5051REGISTER_PLLRNGCLK	0x2E
#define AX5051REGISTER_TXPWR		0x30
#define AX5051REGISTER_TXRATEHI		0x31
#define AX5051REGISTER_TXRATEMID	0x32
#define AX5051REGISTER_TXRATELO		0x33
#define AX5051REGISTER_MODMISC		0x34
#define AX5051REGISTER_FIFOCONTROL2	0x37
#define AX5051REGISTER_ADCMISC		0x38
#define AX5051REGISTER_AGCTARGET	0x39
#define AX5051REGISTER_AGCATTACK	0x3A
#define AX5051REGISTER_AGCDECAY		0x3B
#define AX5051REGISTER_AGCCOUNTER	0x3C
#define AX5051REGISTER_CICDEC		0x3F
#define AX5051REGISTER_DATARATEHI	0x40
#define AX5051REGISTER_DATARATELO	0x41
#define AX5051REGISTER_TMGGAINHI	0x42
#define AX5051REGISTER_TMGGAINLO	0x43
#define AX5051REGISTER_PHASEGAIN	0x44
#define AX5051REGISTER_FREQGAIN		0x45
#define AX5051REGISTER_FREQGAIN2	0x46
#define AX5051REGISTER_AMPLGAIN		0x47
#define AX5051REGISTER_TRKFREQHI	0x4C
#define AX5051REGISTER_TRKFREQLO	0x4D
#define AX5051REGISTER_XTALCAP		0x4F
#define AX5051REGISTER_SPAREOUT		0x60
#define AX5051REGISTER_TESTOBS		0x68
#define AX5051REGISTER_APEOVER		0x70
#define AX5051REGISTER_TMMUX		0x71
#define AX5051REGISTER_PLLVCOI		0x72
#define AX5051REGISTER_PLLCPEN		0x73
#define AX5051REGISTER_PLLRNGMISC	0x74
#define AX5051REGISTER_AGCMANUAL	0x78
#define AX5051REGISTER_ADCDCLEVEL	0x79
#define AX5051REGISTER_RFMISC		0x7A
#define AX5051REGISTER_TXDRIVER		0x7B
#define AX5051REGISTER_REF		0x7C
#define AX5051REGISTER_RXMISC		0x7D


struct usb_ctrl_buf {
	struct usb_ctrlrequest	ctrlrequest;
	int			pipe;
	void*			buf;
	__u16			buflen;
};

struct usb_read_config_flash {
	__u16 addr;
	__u16 buflen;
	__u8  *readBuf;
};

struct usb_rf_setup_buffers {
	__u8 *buf_execute;
	__u8 *buf_preamble_first;
	__u8 *buf_setstate_first;
	__u8 *buf_setRx_first;
	__u8 *buf_preamble_second;
	__u8 *buf_setstate_second;
	__u8 *buf_setRx_second;
};

/*
 * URB error codes
 * Used to represent a list of values and their respective symbolic names
 */
struct klusb_symbolic_list {
	const int num;
	const char *name;
};

static struct klusb_symbolic_list urb_errlist[] = {
	{-ENOMEM,    "No memory for allocation of internal structures"},
	{-ENOSPC,    "The host controller's bandwidth is already consumed"},
	{-ENOENT,    "URB was canceled by unlink_urb"},
	{-EXDEV,     "ISO transfer only partially completed"},
	{-EAGAIN,    "Too match scheduled for the future"},
	{-ENXIO,     "URB already queued"},
	{-EFBIG,     "Too much ISO frames requested"},
	{-ENOSR,     "Buffer error (overrun)"},
	{-EPIPE,     "Specified endpoint is stalled (device not responding)"},
	{-EOVERFLOW, "Babble (bad cable?)"},
	{-EPROTO,    "Bit-stuff error (bad cable?)"},
	{-EILSEQ,    "CRC/Timeout"},
	{-ETIMEDOUT, "NAK (device does not respond)"},
	{-ESHUTDOWN, "Device unplugged"},
	{-1, NULL}
};

struct klusb_ax5015_register_list {
	const __u8 addr;
	int        value;
	__u8       *buf;
};


static struct klusb_ax5015_register_list ax5051_reglist[] = {
	{ AX5051REGISTER_ADCDCLEVEL	, 0x10, NULL },
	{ AX5051REGISTER_ADCMISC	, 0x01, NULL },
	{ AX5051REGISTER_AGCATTACK	, 0x11, NULL },
	{ AX5051REGISTER_AGCDECAY	, 0x0e, NULL },
	{ AX5051REGISTER_AGCMANUAL	, 0x00, NULL },
	{ AX5051REGISTER_AGCTARGET	, 0x0e, NULL },
	{ AX5051REGISTER_AMPLGAIN	, 0x06, NULL },
	{ AX5051REGISTER_APEOVER	, 0x00, NULL },
	{ AX5051REGISTER_CICDEC		, 0x3f, NULL },
	{ AX5051REGISTER_CRCINIT0	, 0xff, NULL },
	{ AX5051REGISTER_CRCINIT1	, 0xff, NULL },
	{ AX5051REGISTER_CRCINIT2	, 0xff, NULL },
	{ AX5051REGISTER_CRCINIT3	, 0xff, NULL },
	{ AX5051REGISTER_DATARATEHI	, 0x19, NULL },
	{ AX5051REGISTER_DATARATELO	, 0x66, NULL },
	{ AX5051REGISTER_ENCODING	, 0x07, NULL },
	{ AX5051REGISTER_FRAMING	, 0x84, NULL }, //# 1000:0100 ##?hdlc? |1000 010 0
	{ AX5051REGISTER_FREQ0		, 0x01, NULL },
	{ AX5051REGISTER_FREQ1		, 0x00, NULL },
	{ AX5051REGISTER_FREQ2		, 0x90, NULL },
	{ AX5051REGISTER_FREQ3		, 0x38, NULL },
	{ AX5051REGISTER_FREQGAIN	, 0x04, NULL },
	{ AX5051REGISTER_FREQGAIN2	, 0x0a, NULL },
	{ AX5051REGISTER_FSKDEV0	, 0x27, NULL },
	{ AX5051REGISTER_FSKDEV1	, 0x31, NULL },
	{ AX5051REGISTER_FSKDEV2	, 0x00, NULL },
	{ AX5051REGISTER_IFFREQHI	, 0x20, NULL },
	{ AX5051REGISTER_IFFREQLO	, 0x00, NULL },
	{ AX5051REGISTER_IFMODE		, 0x00, NULL },
	{ AX5051REGISTER_MODMISC	, 0x03, NULL },
	{ AX5051REGISTER_MODULATION	, 0x41, NULL }, // fsk
	{ AX5051REGISTER_PHASEGAIN	, 0x03, NULL },
	{ AX5051REGISTER_PLLCPEN	, 0x01, NULL },
	{ AX5051REGISTER_PLLLOOP	, 0x1d, NULL },
	{ AX5051REGISTER_PLLRANGING	, 0x08, NULL },
	{ AX5051REGISTER_PLLRNGCLK	, 0x03, NULL },
	{ AX5051REGISTER_PLLVCOI	, 0x01, NULL },
	{ AX5051REGISTER_REF		, 0x23, NULL },
	{ AX5051REGISTER_RFMISC		, 0xb0, NULL },
	{ AX5051REGISTER_RXMISC		, 0x35, NULL },
	{ AX5051REGISTER_SPAREOUT	, 0x00, NULL },
	{ AX5051REGISTER_TESTOBS	, 0x00, NULL },
	{ AX5051REGISTER_TMGGAINHI	, 0x01, NULL },
	{ AX5051REGISTER_TMGGAINLO	, 0x96, NULL },
	{ AX5051REGISTER_TMMUX		, 0x00, NULL },
	{ AX5051REGISTER_TXDRIVER	, 0x88, NULL },
	{ AX5051REGISTER_TXPWR		, 0x03, NULL },
	{ AX5051REGISTER_TXRATEHI	, 0x00, NULL },
	{ AX5051REGISTER_TXRATELO	, 0xec, NULL },
	{ AX5051REGISTER_TXRATEMID	, 0x51, NULL },
	{ AX5051REGISTER_REVISION	, -1  , NULL }, // not used
	{ AX5051REGISTER_SCRATCH	, -1  , NULL }, // not used
	{ AX5051REGISTER_POWERMODE	, -1  , NULL }, // not used
	{ AX5051REGISTER_XTALOSC	, -1  , NULL }, // not used
	{ AX5051REGISTER_FIFOCTRL	, -1  , NULL }, // not used
	{ AX5051REGISTER_FIFODATA	, -1  , NULL }, // not used
	{ AX5051REGISTER_IRQMASK	, -1  , NULL }, // not used
	{ AX5051REGISTER_PINCFG1	, -1  , NULL }, // not used
	{ AX5051REGISTER_PINCFG2	, -1  , NULL }, // not used
	{ AX5051REGISTER_FIFOCONTROL2	, -1  , NULL }, // not used
	{ AX5051REGISTER_AGCCOUNTER	, -1  , NULL }, // not used
	{ AX5051REGISTER_TRKFREQHI	, -1  , NULL }, // not used
	{ AX5051REGISTER_TRKFREQLO	, -1  , NULL }, // not used
	{ AX5051REGISTER_XTALCAP	, -1  , NULL }, // not used
	{ AX5051REGISTER_PLLRNGMISC	, -1  , NULL }  // not used

};

static inline const char *
symbolic(struct klusb_symbolic_list list[], const int num)
{
	int i;
	for (i = 0; list[i].name != NULL; i++)
		if (list[i].num == num)
			return list[i].name;
	return "<unknown USB Error>";
}

struct kl_usb {
	struct usb_device	*dev;		/* our device */
	struct usb_interface	*intf;		/* used interface */

	/* usb rx interrupt handling */
	struct usb_endpoint_descriptor  *rx_int_endpoint_desc;
	struct urb		*rx_int_urb;
	int			rx_int_in_pipe;
	char			*rx_int_buffer;
	int			rx_int_running;

	struct semaphore	sem; /* Locks this structure */
	atomic_t		logger_state;

	/* control pipe background handling */
	struct usb_ctrl_buf	usb_ctrl_buff[KL_USB_CTRL_BUFSIZE];
	int			ctrl_in_idx, ctrl_out_idx, ctrl_cnt;
	struct urb		*ctrl_urb;
	struct usb_ctrlrequest	ctrl_write;
	struct usb_ctrlrequest	ctrl_read;
	int			ctrl_packet_size;
	int			ctrl_in_pipe, ctrl_out_pipe;
	spinlock_t		ctrl_lock; /* lock for ctrl */

	struct usb_rf_setup_buffers rf_setup_buffers;

	unsigned char 		minor;
	int			open_count;
	char 			serial_number[8];
	unsigned int		transceiver_id;
	unsigned char           logger_id;	/* LOGGER_1 .. LOGGER_10 */
	int 			history_record_nr;
	int			last_sent_history_record_nr;
	int			nextSleepMs;
	bool			config_changed;

};

#endif /* __KL_USB_DRV_H__ */
