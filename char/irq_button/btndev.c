#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <mach/gpio.h>

#define btn_SIZE 0x1000 /*全局内存最大 4KB*/


struct pin_desc
{
	unsigned int pin;
	unsigned int key_val;
}pin_desc;

struct btn_dev
{
	struct cdev cdev; /*cdev 结构体*/
//	struct semaphore sem; 	/* 并发控制用的信号量 */
	wait_queue_head_t btnq;	/* 等待队列 */
};

static struct pin_desc pins_desc[4] =
{
	{EXYNOS4_GPX1(0),0x01},//LEFT
	{EXYNOS4_GPX1(3),0x02},//RIGHT
	{EXYNOS4_GPX1(2),0x03},//UP
	{EXYNOS4_GPX1(1),0x04},//DOWN
};

struct btn_dev *btn_devp; /*设备结构体指针*/
static unsigned char key_val;
static int ev_press = 0;
static int btn_major;
static struct class *cdev_class;

static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	struct pin_desc *pindesc = (struct pin_desc *)dev_id;
	unsigned int pinval;
//	printk("eint occured..\r\n");
	pinval = gpio_get_value(pindesc->pin);
	if(pinval)
	{
		key_val = 0x80 | (pindesc->key_val);/* 松开 */
	}
	else
	{
		key_val = pindesc->key_val;/* 按下 */
	}
	
	ev_press = 1; /* 表示中断已经发生 */
	wake_up_interruptible(&btn_devp->btnq); /* 唤醒休眠的进程 */
	return IRQ_HANDLED;
}



int btn_open(struct inode *inode, struct file *filp)
{
	/*将设备结构体指针赋值给文件私有数据指针*/
	int ret, irq;
	filp->private_data = container_of(inode->i_cdev, struct btn_dev, cdev);
	
	irq = gpio_to_irq(pins_desc[0].pin);
	ret = request_irq(irq, buttons_irq,IRQ_TYPE_EDGE_BOTH, "LEFT", &pins_desc[0]);
	if(ret)
	{
		printk("request irq_eint8 error!\r\n");
		if (ret == -EINVAL)
		{
			printk(" pointer invalid or interrupt number invalid");
		}else if (ret == -EBUSY)
		{
			printk(" this interrupt is busy");
		}
	}
		
	
	irq = gpio_to_irq(pins_desc[1].pin);
	ret = request_irq(irq, buttons_irq,IRQ_TYPE_EDGE_BOTH, "RIGHT",&pins_desc[1]);
	if(ret)
	{
		printk("request irq_eint8 error!\r\n");
		if (ret == -EINVAL)
		{
			printk(" pointer invalid or interrupt number invalid\n");
		}else if (ret == -EBUSY)
		{
			printk(" this interrupt is busy\n");
		}
	}
	
	irq = gpio_to_irq(pins_desc[2].pin);
	ret = request_irq(irq, buttons_irq, IRQ_TYPE_EDGE_BOTH, "UP", &pins_desc[2]);
	if(ret)
	{
		printk("request irq_eint8 error!\r\n");
		if (ret == -EINVAL)
		{
			printk(" pointer invalid or interrupt number invalid\n");
		}else if (ret == -EBUSY)
		{
			printk(" this interrupt is busy\n");
		}
	}
	
	irq = gpio_to_irq(pins_desc[3].pin);
	ret = request_irq(irq, buttons_irq, IRQ_TYPE_EDGE_BOTH, "DOWN", &pins_desc[3]);
	if(ret)
	{
		printk("request irq_eint8 error!\r\n");
		if (ret == -EINVAL)
		{
			printk(" pointer invalid or interrupt number invalid\n");
		}else if (ret == -EBUSY)
		{
			printk(" this interrupt is busy\n");
		}
	}
	
	return 0;
}

int btn_release(struct inode *inode, struct file *filp)
{
	int irq;
	
	irq = gpio_to_irq(pins_desc[0].pin);
	free_irq(irq, &pins_desc[0]);
	
	irq = gpio_to_irq(pins_desc[1].pin);
	free_irq(irq,&pins_desc[1]);
	
	irq = gpio_to_irq(pins_desc[2].pin);
	free_irq(irq,&pins_desc[2]);
	
	irq = gpio_to_irq(pins_desc[3].pin);
	free_irq(irq, &pins_desc[3]);
	
	ev_press = 0;
	
	return 0;
}

static long btn_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
{
	struct btn_dev *dev = filp->private_data;/*获得设备结构体指针*/
	switch(cmd)
	{

	}
	return 0;
}
static ssize_t btn_read(struct file *filp, char __user *buf,size_t size,loff_t *ppos)
{
	int ret = 0;
	struct btn_dev *dev = filp->private_data; /*获得设备结构体指针*/
			
	while (!ev_press)
	{
		if (filp->f_flags & O_NONBLOCK)		//若是非阻塞操作
			return -ERESTARTSYS;
		if (wait_event_interruptible(dev->btnq, ev_press))
			return -ERESTARTSYS;		//信号，通知fs层做相应的处理
		
	}		
	/*内核空间→用户空间*/
	if (copy_to_user(buf, &key_val, 1))
	{
		ret = - EFAULT;
		return ret;
	}
	else
	{
		ev_press = 0;
	}
	
	return 1;
	
}


static unsigned int btn_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	struct btn_dev *dev = filp->private_data; /*获得设备结构体指针*/
	
		
	poll_wait(filp, &dev->btnq, wait);//添加写等待队列头
	

	return mask;
}


static const struct file_operations btn_fops =
{
	.owner = THIS_MODULE,
	.read = btn_read,
	.unlocked_ioctl = btn_ioctl,
	.poll = btn_poll,
	.open = btn_open,
	.release = btn_release,
};


int btn_init(void)
{
	int result, err;
	dev_t devno;
	struct device *glomem_dev = NULL;
	result = alloc_chrdev_region(&devno, 0, 1, "x4412-btn");
	if(result < 0)
		return result;
	btn_major = MAJOR(devno);
	btn_devp = kmalloc(sizeof(struct btn_dev),GFP_KERNEL);
	if (!btn_devp)
	{
		result = -ENOMEM;
		goto fail_malloc;
	}
	memset(btn_devp, 0, sizeof(struct btn_dev));
	
	cdev_init(&btn_devp->cdev, &btn_fops);
	btn_devp->cdev.owner = THIS_MODULE;
	btn_devp->cdev.ops = &btn_fops;
	err = cdev_add(&btn_devp->cdev, devno, 1);
	if (err)
	{
		goto fail_cdev_add;
		printk(KERN_NOTICE "Error %d adding cdev%d", err, 0);
	}
	
	cdev_class = class_create(THIS_MODULE,"x4412-btn");	//创建设备类
	if(IS_ERR(cdev_class))
	{
		result = PTR_ERR(cdev_class);
		goto fail_class_create;
	}
	
	/* 注册设备
	 *	在/sys/class/x4412-btn_class/x4412-btn
	 *	/dev/x4412-btn
	 */
	glomem_dev = device_create(cdev_class, NULL, devno, NULL, "x4412-btn");	//注册设备
	if (IS_ERR(glomem_dev))
	{
		result = PTR_ERR(glomem_dev);
		printk(KERN_NOTICE "Error to x4412-btn device_create");
		goto fail_device_create;
	}
	
	init_waitqueue_head(&btn_devp->btnq);	//初始化等待队列头

	
	return 0;
	
	fail_device_create:
	class_destroy(cdev_class);
	
	fail_class_create:
	cdev_del(&btn_devp->cdev);
	
	fail_cdev_add:
	kfree(btn_devp);
			
	fail_malloc:
	unregister_chrdev_region(devno, 1);
	return result;
}

void btn_exit(void)
{
	device_destroy(cdev_class,MKDEV(btn_major,0));
	class_destroy(cdev_class);
	cdev_del(&btn_devp->cdev); /*注销 cdev*/
	kfree(btn_devp); /*释放设备结构体内存*/
	unregister_chrdev_region(MKDEV(btn_major, 0), 1); /*释放设备号*/
}

MODULE_AUTHOR("taxuewuhen");
MODULE_LICENSE("GPL");
module_init(btn_init);
module_exit(btn_exit);
