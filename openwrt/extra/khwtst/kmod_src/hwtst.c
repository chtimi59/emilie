#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <asm/io.h>

static u32 BASE = 0x0;
static dev_t first;
static struct cdev c_dev;
static struct class *cl;

// --- hwtst API ----

typedef enum {
	NONE,
	WRITE_VALUE
} hwtst_op_t;

typedef struct {
	unsigned long base_addr;
	hwtst_op_t    operation;
	unsigned long value;
} hwtst_write_t;

typedef struct {
	unsigned long value;
} hwtst_read_t;


static int my_open(struct inode *i, struct file *f)
{
	return 0;
}
static int my_close(struct inode *i, struct file *f)
{
	return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	hwtst_read_t r={0};

	if(*off!=0) {
		printk(KERN_INFO "unexpected offset\n");
		return 0;
	}		
	
	// Read value
	printk(KERN_INFO "read 0x%08lX at 0x%08X\n", r.value, BASE);	
	{
		void __iomem *mmio = NULL;
		if ((mmio = ioremap(BASE, 4)) == NULL) {
			printk(KERN_ERR "Mapping test module failed\n");
		} else {
			r.value=ioread32(mmio);
			// printk("read done\n");
		}
		if (mmio) iounmap(mmio);
	}

	// copy from user-space	
	if(len<sizeof(hwtst_read_t)) {
		printk(KERN_INFO "Invalid len %u\n", len);
		return 0;
	}
	if (copy_to_user(buf, &r, len))
		return -EFAULT;
		
	*off = len+1;
	return len;
}
	


static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	hwtst_write_t w = {0,0,0};

	if(len!=sizeof(hwtst_write_t)) {
		printk(KERN_INFO "Invalid len %u\n", len);
		return len;
	}
	
	// copy from user-space
	if (copy_from_user(&w, buf, len))
		return -EFAULT;

	// mmio allocation
	BASE = w.base_addr;		
	printk(KERN_INFO "Base 0x%08X\n", BASE);	

	// do asked operations
	switch(w.operation) {
		case NONE:
			break;
		case WRITE_VALUE:
			printk(KERN_INFO "Write 0x%08lX\n", w.value);
			{
				void __iomem *mmio = NULL;
				if ((mmio = ioremap(BASE, 4)) == NULL) {
					printk(KERN_ERR "Mapping test module failed\n");
				} else {
					iowrite32(w.value,mmio);
					//printk("write done\n");
				}
				if (mmio) iounmap(mmio);
			}
			break;
	}			
	
	return len;
}



// -------------------------- Driver declaration ------------------------
// ----------------------------------------------------------------------

static struct file_operations fops =
{
	.owner   = THIS_MODULE,
	.open    = my_open,
	.release = my_close,
	.read    = my_read,
	.write   = my_write
};

static int __init hwtst_init(void)
{
	int ret;
	struct device *dev_ret;
	
	printk(KERN_INFO "Hello HW Reg Tester !..\n");
	
	if ((ret = alloc_chrdev_region(&first, 0, 1, "hwtst")) < 0)
	{
		return ret;
	}
	if (IS_ERR(cl = class_create(THIS_MODULE, "chardrv")))
	{
		unregister_chrdev_region(first, 1);
		return PTR_ERR(cl);
	}
	if (IS_ERR(dev_ret = device_create(cl, NULL, first, NULL, "hwtst")))
	{
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return PTR_ERR(dev_ret);
	}

	cdev_init(&c_dev, &fops);
	if ((ret = cdev_add(&c_dev, first, 1)) < 0)
	{
		device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return ret;
	}
	return 0;
}

static void __exit hwtst_exit(void)
{
	cdev_del(&c_dev);
	device_destroy(cl, first);
	class_destroy(cl);
	unregister_chrdev_region(first, 1);
	printk(KERN_INFO "Goodbye HW Reg Tester !..\n");
}

module_init(hwtst_init);
module_exit(hwtst_exit);

MODULE_AUTHOR("Jan dOrgeville");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Basic HW Register Tester");
