/*
 * kl_usb_drv.h
 *
 *  Created on: Sep 9, 2015
 *      Author: D. Reimann
 */


#ifndef __KL_USB_DRV_H__
#define __KL_USB_DRV_H__


/* RF Frequency */
#define KL_FREQUENCY_EU		868300000	/* 868.3 MHz */
#define KL_FREQ_VAL		910478540	/* (KL_FREQUENCY_EU / 16000000.0 * 16777216.0) */

/* fifo registers */
#define KLUSB_NUM_FIFOS	3	/* maximum number of fifos */
#define KLUSB_INT_RX	0	/* index of the INT IN interrupt fifo */
#define KLUSB_B1_TX		0	/* index for B1 transmit bulk/int */
#define KLUSB_B1_RX		1	/* index for B1 receive bulk/int */
#define KLUSB_B2_TX		2
#define KLUSB_B2_RX		3
#define KLUSB_D_TX		4
#define KLUSB_D_RX		5
#define KLUSB_PCM_TX		6
#define KLUSB_PCM_RX		7

#define ISO_BUFFER_SIZE		128
#define USB_FIFO_BUFFER_SIZE	128

#define USB_INT		0
#define USB_BULK	1
#define USB_ISOC	2

#define USB_HID_FEATURE_REPORT	0x03

#define KL_USB_CTRL_TIMEOUT	5 	/* 5ms timeout writing/reading regs */

/* KlimaLogg pro Message Type Array Index */
#define KL_GET_FRAME			 0
#define KL_SET_RX			 1
#define KL_SET_TX			 2
#define KL_SET_FRAME			 3
#define KL_SET_STATE			 4
#define KL_SET_PREAMBLE_PATTERN		 5
#define KL_EXECUTE			 6
#define KL_READ_CONFIG_FLASH_IN		 7
#define KL_READ_CONFIG_FLASH_OUT	 8
#define KL_GET_STATE			 9
#define KL_WRITE_REG			10


#define KL_MSG_GET_FRAME		0x00	// GET_FRAME
#define KL_LEN_GET_FRAME		0x111	// (Length: 273)
#define KL_MSG_SET_RX			0xd0	// SET_RX
#define KL_LEN_SET_RX			0x15	// (Length:  21)
#define KL_MSG_SET_TX			0xd1	// SET_TX
#define KL_LEN_SET_TX			0x15	// (Length:  21)
#define KL_MSG_SET_FRAME		0xd5	// SET_FRAME
#define KL_LEN_SET_FRAME		0x111	// (Length: 273)
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


/* AX5051 register access by Control-URSs */
#define write_reg_atomic(a, b, c)					\
	usb_control_msg((a)->dev, (a)->ctrl_out_pipe, 0, 0x40, (c), (b), \
			0, 0, HFC_CTRL_TIMEOUT)
#define read_reg_atomic(a, b, c)					\
	usb_control_msg((a)->dev, (a)->ctrl_in_pipe, 1, 0xC0, 0, (b), (c), \
			1, HFC_CTRL_TIMEOUT)

#define KL_CTRL_BUFSIZE 16
#define KL_USB_CTRL_BUFSIZE 64

struct ctrl_buf {
	__u8 ax5051_reg;	/* register number */
	__u8 reg_val;		/* value to be written (or read) */
};

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


//static struct klusb_ax5015_register_list ax5051_reglist[] = {
//	{ AX5051REGISTER_REVISION	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_SCRATCH	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_POWERMODE	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_XTALOSC	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_FIFOCTRL	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_FIFODATA	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_IRQMASK	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_IFMODE		, 0x00, {0x00} },
//	{ AX5051REGISTER_PINCFG1	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_PINCFG2	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_MODULATION	, 0x41, {0x00} }, // fsk
//	{ AX5051REGISTER_ENCODING	, 0x07, {0x00} },
//	{ AX5051REGISTER_FRAMING	, 0x84, {0x00} }, //# 1000:0100 ##?hdlc? |1000 010 0
//	{ AX5051REGISTER_CRCINIT3	, 0xff, {0x00} },
//	{ AX5051REGISTER_CRCINIT2	, 0xff, {0x00} },
//	{ AX5051REGISTER_CRCINIT1	, 0xff, {0x00} },
//	{ AX5051REGISTER_CRCINIT0	, 0xff, {0x00} },
//	{ AX5051REGISTER_FREQ3		, 0x38, {0x00} },
//	{ AX5051REGISTER_FREQ2		, 0x90, {0x00} },
//	{ AX5051REGISTER_FREQ1		, 0x00, {0x00} },
//	{ AX5051REGISTER_FREQ0		, 0x01, {0x00} },
//	{ AX5051REGISTER_FSKDEV2	, 0x00, {0x00} },
//	{ AX5051REGISTER_FSKDEV1	, 0x31, {0x00} },
//	{ AX5051REGISTER_FSKDEV0	, 0x27, {0x00} },
//	{ AX5051REGISTER_IFFREQHI	, 0x20, {0x00} },
//	{ AX5051REGISTER_IFFREQLO	, 0x00, {0x00} },
//	{ AX5051REGISTER_PLLLOOP	, 0x1d, {0x00} },
//	{ AX5051REGISTER_PLLRANGING	, 0x08, {0x00} },
//	{ AX5051REGISTER_PLLRNGCLK	, 0x03, {0x00} },
//	{ AX5051REGISTER_TXPWR		, 0x03, {0x00} },
//	{ AX5051REGISTER_TXRATEHI	, 0x00, {0x00} },
//	{ AX5051REGISTER_TXRATEMID	, 0x51, {0x00} },
//	{ AX5051REGISTER_TXRATELO	, 0xec, {0x00} },
//	{ AX5051REGISTER_MODMISC	, 0x03, {0x00} },
//	{ AX5051REGISTER_FIFOCONTROL2	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_ADCMISC	, 0x01, {0x00} },
//	{ AX5051REGISTER_AGCTARGET	, 0x0e, {0x00} },
//	{ AX5051REGISTER_AGCATTACK	, 0x11, {0x00} },
//	{ AX5051REGISTER_AGCDECAY	, 0x0e, {0x00} },
//	{ AX5051REGISTER_AGCCOUNTER	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_CICDEC		, 0x3f, {0x00} },
//	{ AX5051REGISTER_DATARATEHI	, 0x19, {0x00} },
//	{ AX5051REGISTER_DATARATELO	, 0x66, {0x00} },
//	{ AX5051REGISTER_TMGGAINHI	, 0x01, {0x00} },
//	{ AX5051REGISTER_TMGGAINLO	, 0x96, {0x00} },
//	{ AX5051REGISTER_PHASEGAIN	, 0x03, {0x00} },
//	{ AX5051REGISTER_FREQGAIN	, 0x04, {0x00} },
//	{ AX5051REGISTER_FREQGAIN2	, 0x0a, {0x00} },
//	{ AX5051REGISTER_AMPLGAIN	, 0x06, {0x00} },
//	{ AX5051REGISTER_TRKFREQHI	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_TRKFREQLO	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_XTALCAP	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_SPAREOUT	, 0x00, {0x00} },
//	{ AX5051REGISTER_TESTOBS	, 0x00, {0x00} },
//	{ AX5051REGISTER_APEOVER	, 0x00, {0x00} },
//	{ AX5051REGISTER_TMMUX		, 0x00, {0x00} },
//	{ AX5051REGISTER_PLLVCOI	, 0x01, {0x00} },
//	{ AX5051REGISTER_PLLCPEN	, 0x01, {0x00} },
//	{ AX5051REGISTER_PLLRNGMISC	, -1  , {0x00} }, // not used
//	{ AX5051REGISTER_AGCMANUAL	, 0x00, {0x00} },
//	{ AX5051REGISTER_ADCDCLEVEL	, 0x10, {0x00} },
//	{ AX5051REGISTER_RFMISC		, 0xb0, {0x00} },
//	{ AX5051REGISTER_TXDRIVER	, 0x88, {0x00} },
//	{ AX5051REGISTER_REF		, 0x23, {0x00} },
//	{ AX5051REGISTER_RXMISC		, 0x35, {0x00} }
//};

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


struct kl_usb;
struct usb_fifo;

/* structure defining input+output fifos (interrupt/bulk mode) */
//struct iso_urb {
//	struct urb *urb;
//	__u8 buffer[ISO_BUFFER_SIZE];	/* buffer rx/tx USB URB data */
//	struct usb_fifo *owner_fifo;	/* pointer to owner fifo */
//	__u8 indx; /* Fifos's ISO double buffer 0 or 1 ? */
//#ifdef ISO_FRAME_START_DEBUG
//	int start_frames[ISO_FRAME_START_RING_COUNT];
//	__u8 iso_frm_strt_pos; /* index in start_frame[] */
//#endif
//};

struct usb_fifo {
	int fifonum;		/* fifo index attached to this structure */
	int active;		/* fifo is currently active */
	struct kl_usb *hw;	/* pointer to main structure */
	int pipe;		/* address of endpoint */
	__u8 usb_packet_maxlen;	/* maximum length for usb transfer */
	unsigned int max_size;	/* maximum size of receive/send packet */
	__u8 intervall;		/* interrupt interval */
	struct urb *urb;	/* transfer structure for usb routines */
	__u8 buffer[USB_FIFO_BUFFER_SIZE]; /* buffer USB INT OUT URB data */
	int bit_line;		/* how much bits are in the fifo? */

	__u8 usb_transfer_mode; /* switched between ISO and INT */
//	struct iso_urb	iso[2]; /* two urbs to have one always
//				   one pending */

//	struct dchannel *dch;	/* link to hfcsusb_t->dch */
//	struct bchannel *bch;	/* link to hfcsusb_t->bch */
//	struct dchannel *ech;	/* link to hfcsusb_t->ech, TODO: E-CHANNEL */
	int last_urblen;	/* remember length of last packet */
	__u8 stop_gracefull;	/* stops URB retransmission */
};


struct kl_usb {
	struct list_head	list;
//	struct dchannel		dch;
//	struct bchannel		bch[2];
//	struct dchannel		ech; /* TODO : wait for struct echannel ;) */

	struct usb_device	*dev;		/* our device */
	struct usb_interface	*intf;		/* used interface */
//	int			if_used;	/* used interface number */
//	int			alt_used;	/* used alternate config */
//	int			cfg_used;	/* configuration index used */
//	int			vend_idx;	/* index in hfcsusb_idtab */
	int			packet_size;
//	int			iso_packet_size;
	struct usb_fifo		fifos[KLUSB_NUM_FIFOS];

	struct semaphore sem; /* Locks this structure */

	/* control pipe background handling */
//	struct ctrl_buf		ctrl_buff[KL_CTRL_BUFSIZE]; // TODO remove
	struct usb_ctrl_buf	usb_ctrl_buff[KL_USB_CTRL_BUFSIZE];
	int			ctrl_in_idx, ctrl_out_idx, ctrl_cnt;
	struct urb		*ctrl_urb;
	struct usb_ctrlrequest	ctrl_write;
	struct usb_ctrlrequest	ctrl_read;
	int			ctrl_paksize;
	int			ctrl_in_pipe, ctrl_out_pipe;
	spinlock_t		ctrl_lock; /* lock for ctrl */
	spinlock_t		lock;

	unsigned int		transceiver_id;
	struct usb_rf_setup_buffers rf_setup_buffers;
//	__u8			threshold_mask;
//	__u8			led_state;

//	__u8			protocol;
	int			nt_timer;
	int			open;
	__u8			timers;
	__u8			initdone;
//	char			name[MISDN_MAX_IDLEN];

	unsigned char minor;
	char serial_number[8];
	int open_count; /* Open count for this port */

};


struct usb_kl
{
	/* One structure for each connected device */
	struct usb_device *udev;
	struct usb_interface *interface;
	unsigned char minor;
	char serial_number[8];

	int open_count; /* Open count for this port */
	struct semaphore sem; /* Locks this structure */
	spinlock_t cmd_spinlock; /* locks dev->command */

	char *int_in_buffer;
	struct usb_endpoint_descriptor *int_in_endpoint;
	struct urb *int_in_urb;
	int int_in_running;

	char *ctrl_buffer; /* 8 byte buffer for ctrl msg */
	struct urb *ctrl_urb;
	struct usb_ctrlrequest *ctrl_dr; /* Setup packet information */
	int correction_required;


	char *ctrl_read_buffer;
	char *ctrl_read_buffer2;
	struct urb *ctrl_read_urb;
	struct urb *ctrl_read_urb2;
	struct usb_ctrlrequest *ctrl_read_dr;
	struct usb_ctrlrequest *ctrl_read_dr2;
	unsigned int ctrl_read_transfer_count; /* bytes transfered */

	__u8 command;/* Last issued command */
};
#endif /* __KL_USB_DRV_H__ */
