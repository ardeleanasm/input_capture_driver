/*
  input capture driver
*/


/*
 * General Info
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/interrupt.h>

#define DRIVER_AUTHOR "23ars <ardeleanasm@gmail.com>"
#define DRIVER_DESC "Input Capture driver"
#define DEVICE_NAME "ic"
#define DEVICE_CLASS_NAME "ic_class"
#define DEVICE_PROCESS "ic%d"

#define VERSION_MAJOR_NUMBER 0x01u
#define VERSION_MINOR_NUMBER 0x00u
#define VERSION_PATCH_NUMBER 0x00u

struct ic_device {
  struct cdev cdev;
  struct device *device_Ptr;
  struct mutex io_mutex;
  u8 is_open;
};



static struct ic_device *icdev_Ptr=NULL;
static dev_t icdev_no;
static struct class *icdev_class_Ptr=NULL;

static int icdev_open(struct inode *, struct file *);
static int icdev_release(struct inode *, struct file *);
static ssize_t icdev_read(struct file *, char __user *, size_t, loff_t *);

static irq_handler_t irq_input_handler(int, void *, struct pt_regs *);

struct file_operations fops=
  {
    .open=icdev_open,
    .release=icdev_release,
    .read=icdev_read
  };


static int icdev_open(struct inode *inode, struct file *file)
{
  return 0;
}


static int icdev_release(struct inode *inode, struct file *file)
{
  return 0;
}

static ssize_t icdev_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
  return 0;
}

static irq_handler_t irq_event_handler(int irq, void *dev_id, struct pt_regs *regs)
{
  return (irq_handler_t) IRQ_HANDLED;
}








/*
 * Driver Initialization
 */
static int __init ic_init(void)
{

  icdev_Ptr=kmalloc(sizeof(struct ic_device),GFP_KERNEL);
  if (icdev_Ptr == NULL) {
    printk(KERN_WARNING "%s:Failed to alloc memory for ic_device\n",DEVICE_NAME);
    goto failed_allocation;
  }
  memset(icdev_Ptr,0,sizeof(struct ic_device));
  if (alloc_chrdev_region(&icdev_no,0,1,DEVICE_NAME) < 0) {
    printk(KERN_WARNING "%s:Could not register\n",DEVICE_NAME);
    goto failed_registration;
  }

  icdev_class_Ptr=class_create(THIS_MODULE,DEVICE_CLASS_NAME);
  if (IS_ERR(icdev_class_Ptr)) {
    printk(KERN_WARNING "%s:Could not create class\n",DEVICE_NAME);
    goto failed_class_creation;
  }
  
  cdev_init(&icdev_Ptr->cdev,&fops);
  icdev_Ptr->cdev.owner = THIS_MODULE;
  if (cdev_add(&(icdev_Ptr->cdev),icdev_no,1) != 0) {
    printk(KERN_WARNING "%s:Could not add device\n",DEVICE_NAME);
    goto failed_adding_device;
  }

  icdev_Ptr->device_Ptr = device_create(icdev_class_Ptr,NULL,MKDEV(MAJOR(icdev_no),0),NULL,DEVICE_PROCESS,0);
  if (IS_ERR(icdev_Ptr->device_Ptr)) {
    printk(KERN_WARNING "%s:Could not create device\n",DEVICE_NAME);
    goto failed_device_creation;
  }

  mutex_init(&(icdev_Ptr->io_mutex));
  printk(KERN_INFO "%s:Registered device with (%d,%d)\n",DEVICE_NAME,MAJOR(icdev_no),MINOR(icdev_no));
  printk(KERN_INFO "Driver %s loaded. Version %2x %2x %2x",DEVICE_NAME,
	 VERSION_MAJOR_NUMBER,VERSION_MINOR_NUMBER,VERSION_PATCH_NUMBER);
  return 0;
  
 failed_device_creation:
  device_destroy(icdev_class_Ptr,MKDEV(MAJOR(icdev_no),0));
  cdev_del(&(icdev_Ptr->cdev));
  
 failed_adding_device:
  class_destroy(icdev_class_Ptr);
  icdev_class_Ptr = NULL;
  
 failed_class_creation:
  unregister_chrdev_region(icdev_no,1);
  
 failed_registration:
  kfree(icdev_Ptr);
  icdev_Ptr = NULL;
  
 failed_allocation:
  return -EBUSY;
  
}

static void __exit ic_exit(void)
{
  printk(KERN_INFO "%s:Unregister...:(",DEVICE_NAME);
  if (icdev_Ptr != NULL) {
    device_destroy(icdev_class_Ptr,MKDEV(MAJOR(icdev_no),0));
    cdev_del(&(icdev_Ptr->cdev));
    kfree(icdev_Ptr);
    icdev_Ptr = NULL;
  }
  unregister_chrdev_region(icdev_no,1);
  if (icdev_class_Ptr != NULL) {
    class_destroy(icdev_class_Ptr);
    icdev_class_Ptr = NULL;
  }
  printk(KERN_INFO "Driver %s unloaded.",DEVICE_NAME);
  
  
  
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("device")

module_init(ic_init);
module_exit(ic_exit);
