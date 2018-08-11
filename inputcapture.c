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


#define DRIVER_AUTHOR "23ars <ardeleanasm@gmail.com>"
#define DRIVER_DESC "Input Capture driver"
#define DEVICE_NAME "ic"
#define DEVICE_CLASS_NAME "ic_class"
#define DEVICE_PROCESS "ic%d"


struct ic_device {
  struct cdev cdev;
  struct device *device_Ptr;
  struct mutex io_mutex;
  u8 is_open;
};



static struct ic_device *icdev_Ptr=NULL;
static dev_t ic_dev_no;




/*
 * Driver Initialization
 */
static int __init ic_init(void)
{

  
  
  return 0;
}

static void __exit ic_exit(void)
{
  printk(KERN_ERR "Exit");
  
  
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("device")

module_init(ic_init);
module_exit(ic_exit);
