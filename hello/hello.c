#include <linux/module.h>
#include <linux/init.h>
static int __devinit hello_x4412_init(void)
{
printk("hello,x4412!\r\n");
return 0;
}
static void hello_x4412_exit(void)
{
printk("Goodbye,x4412!\r\n");
}

module_init(hello_x4412_init);
module_exit(hello_x4412_exit);
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_AUTHOR("www.9tripod.com");
MODULE_ALIAS("a Character driver sample");
MODULE_DESCRIPTION("hello x4412 driver");
