#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/device.h>

static dev_t devno;
static struct cdev x4412_cdev;	//第一步，分配设备描述结构

int x4412_cdev_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int x4412_cdev_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long x4412_cdev_ioctl(struct file *file,unsigned int cmd, unsigned long arg)
{
	return 0;
}

static ssize_t x4412_cdev_read(struct file *filp, char __user *buf,size_t size,loff_t *ppos)
{
	int read_sizes = 0;
	char *p;
	char Message[]="This is x4412/ibox devboard.";
	p = Message;
	while(size && *p)
	{
		if(put_user(*(p++),buf++))
		return -EINVAL;
		size--;
		read_sizes++;
	}
	return read_sizes;
}

static ssize_t x4412_cdev_write(struct file *filp, const char __user *buf,size_t size, loff_t *ppos)
{
	int i;
	char str;
	for(i=0;i<size;i++)
	{
		if(!get_user(str,buf++))
		printk("%c",str);
	}
	printk("\r\n");
	return size;
}

static const struct file_operations x4412_fops =
{
	.owner = THIS_MODULE,
	.read = x4412_cdev_read,
	.write = x4412_cdev_write,
	.unlocked_ioctl = x4412_cdev_ioctl,
	.open = x4412_cdev_open,
	.release = x4412_cdev_release,
};		//第五步，实现设备操作

int x4412_cdev_init(void)
{
	int result,err;
	int x4412_cdev_major;


	result = alloc_chrdev_region(&devno, 0, 1, "x4412-cdev");	//第二步,分配设备号
	if(result < 0)
	{
		printk("register x4412-cdev error!\r\n");
		return result;
	}
	
	cdev_init(&x4412_cdev, &x4412_fops);	//第三步,初始化x4412_cdev,建立cdev和file_operation之间的连接
	
	err = cdev_add(&x4412_cdev, devno, 1);		//第四步,注册cdev，（向系统添加一个cdev）
	if (err)
		printk(KERN_NOTICE "Error %d adding x4412_cdev", err);
	
	return 0;
}





void x4412_cdev_exit(void)
{
	cdev_del(&x4412_cdev);	//第六步，注销cdev(向系统删除一个cdev)
	unregister_chrdev_region(devno, 1);		//第七步，释放申请的设备号
}

MODULE_AUTHOR("taxuewuhen");
MODULE_LICENSE("GPL");
module_init(x4412_cdev_init);
module_exit(x4412_cdev_exit);
