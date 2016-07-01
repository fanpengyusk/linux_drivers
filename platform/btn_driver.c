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




struct resource btn_resource[] =
{
	{
		.start = IRQ_EINT0,
		.end   = IRQ_EINT0,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = IRQ_EINT1,
		.end   = IRQ_EINT1,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = IRQ_EINT2,
		.end   = IRQ_EINT2,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = IRQ_EINT3,
		.end   = IRQ_EINT3,
		.flags = IORESOURCE_IRQ,
	},
};

struct gpio_button gpio_btns[] = 
{
	{
		.code = KEY_UP'
		.desc = "UP",
		.gpio = EXYNOS4_GPX1(2),
	},
	{
		.code = KEY_DOWN'
		.desc = "UP",
		.gpio = EXYNOS4_GPX1(1),
	},
	{
		.code = KEY_LEFT'
		.desc = "UP",
		.gpio = EXYNOS4_GPX1(0),
	},
	{
		.code = KEY_RIGHT'
		.desc = "UP",
		.gpio = EXYNOS4_GPX1(4),
	},
	{
		.code = KEY_UP'
		.desc = "UP",
		.gpio = EXYNOS4_GPX1(0),
	},
}; 

void gpio_btn_release(struct device *dev)
{
	printk("enter gpio_btn_release\n");
}

struct platform_device btn_dev =
{
	.name = "x4412btn",
	.resource = btn_resource,
	.num_resources = ARRAY_SIZE(btn_resource),
	.dev = 
	{
		.platform_data = gpio_btns,
		.release = gpio_btn_release,
	},
}



int btn_init(void)
{
	platform_device_register(&btn_dev);
	return 0;
}

void btn_exit(void)
{
	platform_device_unregister(&btn_dev);
}

MODULE_AUTHOR("taxuewuhen");
MODULE_LICENSE("GPL");
module_init(btn_init);
module_exit(btn_exit);
