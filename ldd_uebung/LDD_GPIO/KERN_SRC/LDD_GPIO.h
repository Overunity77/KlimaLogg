
#define DRIVER_NAME "LDD_GPIO"
#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s:"fmt,DRIVER_NAME, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s:"fmt,DRIVER_NAME,##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s:"fmt,DRIVER_NAME, ##args)
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/fs.h>
#include<linux/init.h>
#include<linux/kdev_t.h>
#include<linux/module.h>
#include<linux/types.h>
#include<linux/uaccess.h>
#include<linux/gpio.h>
#include<linux/string.h>
#include<linux/slab.h>
#include<linux/interrupt.h>
#include<linux/jiffies.h>

static irqreturn_t irq_handler(int, void *);
static irqreturn_t irq_Tasklet_handler(int, void *);
static irqreturn_t irq_WorkItem_handler(int, void *);
static irqreturn_t irq_ThreadedInterupt_TopHandler(int, void *);
static irqreturn_t irq_ThreadedInterupt_BottomHandler(int, void *);


#define TASKLET
//#define WORKITEM
//#define THREADED
