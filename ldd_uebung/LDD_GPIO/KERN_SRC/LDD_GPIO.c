/*
 ===============================================================================
 Driver Name		:		LDD_GPIO
 Author			:		CHRISTIAN BINDER
 License			:		GPL
 Description		:		LINUX DEVICE DRIVER PROJECT
 ===============================================================================
 */

#include"LDD_GPIO.h"

#define LDD_GPIO_N_MINORS 2
#define LDD_GPIO_FIRST_MINOR 0
#define LDD_GPIO_NODE_NAME "LDD_GPIO"
#define LDD_GPIO_BUFF_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CHRISTIAN BINDER");

int LDD_GPIO_major = 0;
int LED_1 = 69;
int LED_2 = 68;
int BUTTON_T1 = 7;
int BUTTON_T2 = 60;
int BUTTON_T3 = 48;
int BUTTON_T4 = 51;
int BUTTON_T5 = 115;

const int debounce = 150;

dev_t LDD_GPIO_device_num;

struct class *LDD_GPIO_class;

typedef struct privatedata {
	int nMinor;

	char buff[LDD_GPIO_BUFF_SIZE];

	struct cdev cdev;

	struct device *LDD_GPIO_device;

	int irq;

	unsigned long jiffiesFromLastIRQ;

} LDD_GPIO_private;

LDD_GPIO_private devices[LDD_GPIO_N_MINORS];

//definitions for workItems
static struct workqueue_struct *my_wq;

typedef struct {
	struct work_struct my_work;
	int x;
} my_work_t;

my_work_t *work;

void toggle_LED(int gpionr) {
	bool value = (bool) gpio_get_value(gpionr);
	gpio_set_value(gpionr, !value);
}

static int LDD_GPIO_open(struct inode *inode, struct file *filp) {
	LDD_GPIO_private *priv = container_of(inode->i_cdev ,
			LDD_GPIO_private ,cdev);
	filp->private_data = priv;

	PINFO("In char driver open() function\n");

	return 0;
}

static int LDD_GPIO_release(struct inode *inode, struct file *filp) {
	LDD_GPIO_private *priv;

	priv = filp->private_data;

	PINFO("In char driver release() function\n");

	return 0;
}

static ssize_t LDD_GPIO_read(struct file *filp, char __user *ubuff,
		size_t count, loff_t *offp) {
	int n = 0;
	char* buffer;
	int value1;
	int value2;
	int value3;
	int value4;
	int value5;

	LDD_GPIO_private *priv;

	priv = filp->private_data;

	if (priv->nMinor == 1) {
		int size = 7;
		if (*offp == size) {
			return 0;
		}

		buffer = kzalloc(size, GFP_KERNEL);
		value1 = gpio_get_value(BUTTON_T1);
		value2 = gpio_get_value(BUTTON_T2);
		value3 = gpio_get_value(BUTTON_T3);
		value4 = gpio_get_value(BUTTON_T4);
		value5 = gpio_get_value(BUTTON_T5);
		snprintf(buffer, size, "%i%i%i%i%i\n", value1, value2, value3, value4,
				value5);
		copy_to_user(ubuff, buffer, size);
		kzfree(buffer);
		*offp = size;
		return size;
	}
	PINFO("In char driver read() function\n");
	return n;
}

static ssize_t LDD_GPIO_write(struct file *filp, const char __user *ubuff,
		size_t count, loff_t *offp) {
	int n = count;
	LDD_GPIO_private *priv;

	priv = filp->private_data;
	if (priv->nMinor == 0) {
		PINFO("Minor==0\n");
		if (count > 1) {
			if (strncmp(ubuff, "1", 1) == 0) {
				PINFO("LED 1\n");
				toggle_LED(LED_1);
			} else if (strncmp(ubuff, "2", 1) == 0) {
				PINFO("LED 2\n");
				toggle_LED(LED_2);
			} else {
				PERR("No valid Data, ignoring command..\n");
			}
		}

	} else {
		PINFO("Minor= %i \n",priv->nMinor);
		return n;
	}
	PINFO("In char driver write() function\n");
	//gpio_set_value(LED_1,1);
	return n;
}

static const struct file_operations LDD_GPIO_fops = { .owner = THIS_MODULE,
		.open = LDD_GPIO_open, .release = LDD_GPIO_release, .read =
				LDD_GPIO_read, .write = LDD_GPIO_write, };

static int __init LDD_GPIO_init(void) {
	irq_handler_t tmp;
	int i;
	int res;

	res = alloc_chrdev_region(&LDD_GPIO_device_num, LDD_GPIO_FIRST_MINOR,
			LDD_GPIO_N_MINORS, DRIVER_NAME);
	if (res) {
		PERR("register device no failed\n");
		return -1;
	}
	LDD_GPIO_major = MAJOR(LDD_GPIO_device_num);

	LDD_GPIO_class = class_create(THIS_MODULE , DRIVER_NAME);
	if (!LDD_GPIO_class) {
		PERR("class creation failed\n");
		return -1;
	}

	for (i = 0; i < LDD_GPIO_N_MINORS; i++) {
		LDD_GPIO_device_num = MKDEV(LDD_GPIO_major ,LDD_GPIO_FIRST_MINOR+i);
		cdev_init(&devices[i].cdev, &LDD_GPIO_fops);
		cdev_add(&devices[i].cdev, LDD_GPIO_device_num, 1);

		devices[i].LDD_GPIO_device = device_create(LDD_GPIO_class, NULL,
				LDD_GPIO_device_num, NULL, LDD_GPIO_NODE_NAME"%d",
				LDD_GPIO_FIRST_MINOR + i);
		if (!devices[i].LDD_GPIO_device) {
			class_destroy(LDD_GPIO_class);
			PERR("device creation failed\n");
			return -1;
		}

		devices[i].nMinor = LDD_GPIO_FIRST_MINOR + i;
	}

	if (gpio_request(LED_1, "LDD_LED1") != 0) {
		PERR("was not able to register LDD_LED1\n");
		return -EFAULT;
	}
	if (gpio_request(LED_2, "LDD_LED2") != 0) {
		PERR("was not able to register LDD_LED2\n");
		return -EFAULT;
	}
	if (gpio_request(BUTTON_T1, "LDD_BUTTON_T1") != 0) {
		PERR("was not able to register LDD_BUTTON_T1\n");
		return -EFAULT;
	}
	if (gpio_request(BUTTON_T2, "LDD_BUTTON_T2") != 0) {
		PERR("was not able to register LDD_BUTTON_T2\n");
		return -EFAULT;
	}
	if (gpio_request(BUTTON_T3, "LDD_BUTTON_T3") != 0) {
		PERR("was not able to register LDD_BUTTON_T3\n");
		return -EFAULT;
	}
	if (gpio_request(BUTTON_T4, "LDD_BUTTON_T4") != 0) {
		PERR("was not able to register LDD_BUTTON_T4\n");
		return -EFAULT;
	}
	if (gpio_request(BUTTON_T5, "LDD_BUTTON_T5") != 0) {
		PERR("was not able to register LDD_BUTTON_T5\n");
		return -EFAULT;
	}

	//configure LED as output
	gpio_direction_output(LED_1, 0);
	gpio_direction_output(LED_2, 0);

	//configure Bttons as input
	gpio_direction_input(BUTTON_T1);
	gpio_direction_input(BUTTON_T2);
	gpio_direction_input(BUTTON_T3);
	gpio_direction_input(BUTTON_T4);
	gpio_direction_input(BUTTON_T5);

	//set init value
	gpio_set_value(LED_1, 0);
	gpio_set_value(LED_2, 0);

	//register irq
	devices[0].irq = gpio_to_irq(BUTTON_T1);
	devices[1].irq = gpio_to_irq(BUTTON_T2);

#ifdef TASKLET
	//handles interrupt with Tasklet in Bottom
	tmp = irq_Tasklet_handler;
#elif defined WORKITEM
	//handles interrupt with workItem
	tmp = irq_WorkItem_handler;
#else
	//handles interrupt in top
	tmp = irq_handler;
#endif

#ifdef THREADED
	request_threaded_irq(devices[0].irq,irq_ThreadedInterupt_TopHandler,irq_ThreadedInterupt_BottomHandler,IRQF_TRIGGER_RISING, "test", &devices[0]);
	request_threaded_irq(devices[1].irq,irq_ThreadedInterupt_TopHandler,irq_ThreadedInterupt_BottomHandler,IRQF_TRIGGER_RISING, "test", &devices[1]);
#else
	request_irq(devices[0].irq, tmp, IRQF_TRIGGER_RISING, "test", &devices[0]);
	request_irq(devices[1].irq, tmp, IRQF_TRIGGER_RISING, "test", &devices[1]);
#endif



	//create workQueue
	my_wq = create_workqueue("my_queue");
	PINFO("INIT\n");

	return 0;
}

static void __exit LDD_GPIO_exit(void) {
	int i;

	//freeing registered irqa
	free_irq(devices[0].irq, &devices[0]);
	free_irq(devices[1].irq, &devices[1]);

	PINFO("EXIT\n");

	for (i = 0; i < LDD_GPIO_N_MINORS; i++) {
		LDD_GPIO_device_num = MKDEV(LDD_GPIO_major ,LDD_GPIO_FIRST_MINOR+i);

		cdev_del(&devices[i].cdev);

		device_destroy(LDD_GPIO_class, LDD_GPIO_device_num);

	}

	class_destroy(LDD_GPIO_class);

	unregister_chrdev_region(LDD_GPIO_device_num, LDD_GPIO_N_MINORS);

	//free gpios

	gpio_free(LED_1);
	gpio_free(LED_2);
	gpio_free(BUTTON_T1);
	gpio_free(BUTTON_T2);
	gpio_free(BUTTON_T3);
	gpio_free(BUTTON_T4);
	gpio_free(BUTTON_T5);

	//free up work Queue
	flush_workqueue(my_wq);
	destroy_workqueue(my_wq);
}

static irqreturn_t irq_handler(int irq, void *devid) {
	unsigned long deltaT;
	unsigned long tmp;
	LDD_GPIO_private *priv;

	PINFO("TOP BEGIN\n");
	deltaT = msecs_to_jiffies(debounce);
	priv = devid;
	tmp = get_jiffies_64();
	if (deltaT > (tmp - priv->jiffiesFromLastIRQ)) {
		priv->jiffiesFromLastIRQ = tmp;
		return IRQ_HANDLED;
	}
	priv->jiffiesFromLastIRQ = tmp;
	PINFO("In char driver ISR IRQ:%i device Minor:%i\n",irq,priv->nMinor);
	if (priv->nMinor == 0) {
		toggle_LED(LED_1);
	} else if (priv->nMinor == 1) {
		toggle_LED(LED_2);
	}
	PINFO("TOP END\n");
	return IRQ_HANDLED;
}

static void taskletFunction(unsigned long data) {
	PINFO("BOTTOM\n");
	toggle_LED(*((unsigned long*) data));
	PINFO("Tasklet\n");
}

char my_tasklet_data[] = "my_tasklet_function was called";

unsigned long data = 0;
DECLARE_TASKLET(Hallo,&taskletFunction,&data);

static irqreturn_t irq_Tasklet_handler(int irq, void *devid) {
	unsigned long deltaT;
	unsigned long tmp;
	LDD_GPIO_private *priv;
	PINFO("TOP\n");


	deltaT = msecs_to_jiffies(debounce);
	priv = devid;
	tmp = get_jiffies_64();
	if (deltaT > (tmp - priv->jiffiesFromLastIRQ)) {
		priv->jiffiesFromLastIRQ = tmp;
		return IRQ_HANDLED;
	}
	priv->jiffiesFromLastIRQ = tmp;

	if (priv->nMinor == 0) {
		data = LED_1;
	} else if (priv->nMinor == 1) {
		data = LED_2;
	}
	tasklet_schedule(&Hallo);
	return IRQ_HANDLED;
}

static void my_wq_function(struct work_struct *work) {
	PINFO("BOTTOM\n");
	my_work_t *my_work = (my_work_t *) work;

	toggle_LED(my_work->x);
	printk("my_work.x %d\n", my_work->x);

	kfree((void *) work);

	return;
}

static irqreturn_t irq_WorkItem_handler(int irq, void *devid) {
	unsigned long deltaT;
	unsigned long tmp;
	LDD_GPIO_private *priv;
	PINFO("TOP\n");

	deltaT = msecs_to_jiffies(debounce);
	priv = devid;
	tmp = get_jiffies_64();
	if (deltaT > (tmp - priv->jiffiesFromLastIRQ)) {
		priv->jiffiesFromLastIRQ = tmp;
		return IRQ_HANDLED;
	}
	priv->jiffiesFromLastIRQ = tmp;

	work = (my_work_t*) kmalloc(sizeof(my_work_t), GFP_KERNEL);
	if (work) {
		INIT_WORK((struct work_struct*)work,my_wq_function);

		if (priv->nMinor == 0) {
			work->x = LED_1;
		} else if (priv->nMinor == 1) {
			work->x = LED_2;
		}
		queue_work(my_wq,(struct work_struct*)work);
	}
	return IRQ_HANDLED;
}

static irqreturn_t irq_ThreadedInterupt_TopHandler(int irq, void *devid){
	unsigned long deltaT;
	unsigned long tmp;
	LDD_GPIO_private *priv;
	PINFO("TOP\n");

	deltaT = msecs_to_jiffies(debounce);
	priv = devid;
	tmp = get_jiffies_64();
	if (deltaT > (tmp - priv->jiffiesFromLastIRQ)) {
		priv->jiffiesFromLastIRQ = tmp;
		return IRQ_HANDLED;
	}
	return IRQ_WAKE_THREAD;
}

static irqreturn_t irq_ThreadedInterupt_BottomHandler(int irq, void *devid){
	LDD_GPIO_private *priv;
	PINFO("BOTTOM\n");
	PINFO("Threaded\n");

	priv = devid;
	if (priv->nMinor == 0) {
		toggle_LED(LED_1);
	} else if (priv->nMinor == 1) {
		toggle_LED(LED_2);
	}
	return IRQ_HANDLED;
}

module_init(LDD_GPIO_init);
module_exit(LDD_GPIO_exit);

