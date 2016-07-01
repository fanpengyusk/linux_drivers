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

#define globalfifo_SIZE 0x1000 /*全局内存最大 4KB*/
#define MEM_CLEAR 0x1 /*清零全局内存*/
static int globalfifo_major;
static struct class *cdev_class;
static int have_data,full_data;

#define DEBUG_GLOBAFIFO

#ifdef DEBUG_GLOBAFIFO
	#define gdebug(format, args...) printk(format, ##args)
#else
	#define gdebug(format, args...) 
#endif



struct globalfifo_dev
{
	struct cdev cdev; /*cdev 结构体*/
	unsigned char mem[globalfifo_SIZE]; /*全局内存*/
	unsigned int current_len; /*mem 有效数据长度*/
	struct semaphore sem; 	/* 并发控制用的信号量 */
	wait_queue_head_t outq, inq;	/* 读取和写入队列 */
	struct fasync_struct *async_queue;/*异步结构体指针*/
};

struct globalfifo_dev *globalfifo_devp; /*设备结构体指针*/

static int globalfifo_fasync(int fd,struct file *filp,int mode)
{
	struct globalfifo_dev *dev =filp->private_data;
	return fasync_helper(fd,filp,mode,&dev->async_queue);
}

int globalfifo_open(struct inode *inode, struct file *filp)
{
	/*将设备结构体指针赋值给文件私有数据指针*/
	filp->private_data = container_of(inode->i_cdev, struct globalfifo_dev, cdev);
	return 0;
}

int globalfifo_release(struct inode *inode, struct file *filp)
{
	globalfifo_fasync(-1,filp,0);/*将文件从异步通知列表里删除*/
	return 0;
}

static long globalfifo_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
{
	struct globalfifo_dev *dev = filp->private_data;/*获得设备结构体指针*/
	switch(cmd)
	{
		case MEM_CLEAR:
			if (down_interruptible(&dev->sem))	/*获取信号量*/
				return -ERESTARTSYS;
				
			memset(dev->mem, 0, globalfifo_SIZE);
			dev->current_len = 0;
			
			up(&dev->sem);	/*释放信号量*/
			printk(KERN_INFO "globalfifo is set to zero\n");
			break;
		default:
			return -EINVAL;
	}
	return 0;
}
static ssize_t globalfifo_read(struct file *filp, char __user *buf,size_t size,loff_t *ppos)
{
	unsigned long pos = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalfifo_dev *dev = filp->private_data; /*获得设备结构体指针*/
	/*分析和获取有效的写长度*/
	if (pos >= globalfifo_SIZE)
		return count ? -ENXIO: 0;
	if (count > globalfifo_SIZE - pos)
		count = globalfifo_SIZE - pos;
	
	if (down_interruptible(&dev->sem))	/*获取信号量*/
		return -ERESTARTSYS;
		
	while (!have_data)
	{
		gdebug("not have data");
		up(&dev->sem);		/* 释放信号量 */
		if (filp->f_flags & O_NONBLOCK)		//若是非阻塞操作
			return -ERESTARTSYS;
			
		if (wait_event_interruptible(dev->inq, have_data))
			return -ERESTARTSYS;		//信号，通知fs层做相应的处理
		
		if (down_interruptible(&dev->sem))	//否则循环，但首先获取锁
			return -ERESTARTSYS;
	}		
	/*内核空间→用户空间*/
	if (copy_to_user(buf, (void*)(dev->mem + pos), count))
	{
		ret = - EFAULT;
	}
	else
	{
		ret = count;
		*ppos += count;
		dev->current_len -= count;
//		printk(KERN_INFO "read %u bytes from %lu\n", count, pos);
	}
	
	up(&dev->sem);	/*释放信号量*/
	
	have_data = false;
	
	
//	wake_up_interruptible(&dev->outq);	//唤醒所有的写入者
	
	return ret;
}
static ssize_t globalfifo_write(struct file *filp, const char __user *buf,size_t size, loff_t *ppos)
{
	unsigned long pos = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalfifo_dev *dev = filp->private_data; /*获得设备结构体指针*/
	
	if (pos >= globalfifo_SIZE)
		return count ? - ENXIO: 0;
	if (count > globalfifo_SIZE - pos)
		count = globalfifo_SIZE - pos;
	
	if (down_interruptible(&dev->sem))	/*获取信号量*/
		return -ERESTARTSYS;
	
	while (full_data)
	{
		up(&dev->sem);		/* 释放锁 */
		if (filp->f_flags & O_NONBLOCK)		//若是非阻塞操作
			return -ERESTARTSYS;
			
		if (wait_event_interruptible(dev->inq, full_data))
			return -ERESTARTSYS;		//信号，通知fs层做相应的处理
		
		if (down_interruptible(&dev->sem))	//否则循环，但首先获取锁
			return -ERESTARTSYS;
	}		
	
	if (copy_from_user(dev->mem + pos, buf, count))
		ret = - EFAULT;
	else
	{
		ret = count;
		*ppos += count;
		dev->current_len += count;
		
		have_data = true;
		wake_up_interruptible(&dev->inq);	//唤醒所有的读取者
//		printk(KERN_INFO "write %u bytes to %lu\n", count, pos);
	}
	
	up(&dev->sem);	/*释放信号量*/
				

		kill_fasync(&dev->async_queue,SIGIO,POLL_IN);	//产生SIGIO信号
		gdebug("create async signal!\n");
	
	return ret;
}

static unsigned int globalfifo_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	struct globalfifo_dev *dev = filp->private_data; /*获得设备结构体指针*/
	
	if (down_interruptible(&dev->sem))	/*获取信号量*/
		return -ERESTARTSYS;
		
	poll_wait(filp, &dev->inq, wait);//添加写等待队列头
	poll_wait(filp, &dev->outq, wait);//添加读等待队列头
	
	if(dev->current_len != 0)/*fifo 非空*/
	{
		mask |= POLLIN | POLLRDNORM; /*标示数据可获得*/
	}
	
	if (dev->current_len != globalfifo_SIZE)/*fifo 非满*/
	{
		mask |= POLLOUT | POLLWRNORM; /*标示数据可写入*/
	}
	
	up(&dev->sem);
	return mask;
}

static loff_t globalfifo_lseek(struct file *filp, loff_t offset, int whence)
{ 
    loff_t newpos;

    switch(whence) {
      case SEEK_SET: 
        newpos = offset;
        break;

      case SEEK_CUR: 
        newpos = filp->f_pos + offset;
        break;

      case SEEK_END: 
        newpos = 5*sizeof(int)-1 + offset;
        break;

      default: 
        return -EINVAL;
    }
    if ((newpos<0) || (newpos>5*sizeof(int)))
    	return -EINVAL;
    	
    filp->f_pos = newpos;
    return newpos;

}

static const struct file_operations globalfifo_fops =
{
	.owner = THIS_MODULE,
	.read = globalfifo_read,
	.write = globalfifo_write,
	.llseek = globalfifo_lseek,
	.unlocked_ioctl = globalfifo_ioctl,
	.poll = globalfifo_poll,
	.open = globalfifo_open,
	.release = globalfifo_release,
};

int globalfifo_init(void)
{
	int result, err;
	dev_t devno;
		struct device *glomem_dev = NULL;
	result = alloc_chrdev_region(&devno, 0, 1, "x4412-globalfifo");
	if(result < 0)
		return result;
	globalfifo_major = MAJOR(devno);
	globalfifo_devp = kmalloc(sizeof(struct globalfifo_dev),GFP_KERNEL);
	if (!globalfifo_devp)
	{
		result = -ENOMEM;
		goto fail_kmalloc;
	}
	memset(globalfifo_devp, 0, sizeof(struct globalfifo_dev));
	cdev_init(&globalfifo_devp->cdev, &globalfifo_fops);
	globalfifo_devp->cdev.owner = THIS_MODULE;
	globalfifo_devp->cdev.ops = &globalfifo_fops;
	err = cdev_add(&globalfifo_devp->cdev, devno, 1);
	if (err)
	{
		goto fail_cdev_add;
		printk(KERN_NOTICE "Error %d adding cdev%d", err, 0);
	}
	
	cdev_class = class_create(THIS_MODULE,"x4412-globalmem");	//创建设备类
	if(IS_ERR(cdev_class))
	{
		result = PTR_ERR(cdev_class);
		goto fail_class_create;
	}
	
	/* 注册设备
	 *	在/sys/class/x4412-globalmem_class/x4412-globalmem
	 *	/dev/x4412-globalmem
	 */
	glomem_dev = device_create(cdev_class, NULL, devno, NULL, "x4412-globalmem");	//注册设备
	if (IS_ERR(glomem_dev))
	{
		result = PTR_ERR(glomem_dev);
		printk(KERN_NOTICE "Error to x4412-globalmem device_create");
		goto fail_device_create;
	}
	
	init_waitqueue_head(&globalfifo_devp->inq);	//初始化写入等待队列头
	init_waitqueue_head(&globalfifo_devp->outq);	//初始化读取等待队列头
	
	sema_init(&globalfifo_devp->sem, 1);	/* 初始化信号量 */
	
	return 0;
	
	fail_device_create:
	class_destroy(cdev_class);
	
	fail_class_create:
	cdev_del(&globalfifo_devp->cdev);
	
	fail_cdev_add:
	kfree(globalfifo_devp);
			
	fail_kmalloc:
	unregister_chrdev_region(devno, 1);
	return result;
}
void globalfifo_exit(void)
{
	device_destroy(cdev_class,MKDEV(globalfifo_major,0));
	class_destroy(cdev_class);
	cdev_del(&globalfifo_devp->cdev); /*注销 cdev*/
	kfree(globalfifo_devp); /*释放设备结构体内存*/
	unregister_chrdev_region(MKDEV(globalfifo_major, 0), 1); /*释放设备号*/
}

module_init(globalfifo_init);
module_exit(globalfifo_exit);
MODULE_AUTHOR("taxuewuhen");
MODULE_LICENSE("GPL");

