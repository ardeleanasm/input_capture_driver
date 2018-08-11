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


#define FREE_DEVICE() \
  kfree(icdev_Ptr);	   \
  icdev_Ptr = NULL

#define UNREGISTER_REGION() unregister_chrdev_region(icdev_no,1)

#define DESTROY_CLASS() \
  class_destroy(icdev_class_Ptr);		\
  icdev_class_Ptr = NULL

#define DESTROY_DEVICE() \
  device_destroy(icdev_class_Ptr,MKDEV(MAJOR(icdev_no),0));	\
  cdev_del(&(icdev_Ptr->cdev))


#define _IOCTL_MAGIC 'K'
#define IOCICDEVGPIORP _IOW(_IOCTL_MAGIC,1,u16) /* register pin */
#define IOCICDEVGPIOUP _IOW(_IOCTL_MAGIC,2,u16) /* unregister ping */
#define IOCICDEVGPIOEN _IO(IOCTL_MAGIC,3)       /* start */
#define IOCICDEVGPIODS _IO(IOCTL_MAGIC,4)       /* stop  */

struct ic_device {
  struct cdev cdev;
  struct device *device_Ptr;
  struct mutex io_mutex;
  u8 is_open;
};


static struct ic_device *icdev_Ptr=NULL;
static dev_t icdev_no;
static struct class *icdev_class_Ptr=NULL;

static u16 gpio_no;
static u32 icdev_value;
static int icdev_open(struct inode *, struct file *);
static int icdev_release(struct inode *, struct file *);
static ssize_t icdev_read(struct file *, char __user *, size_t, loff_t *);
static long icdev_ioctl(struct file *, unsigned int, unsigned long);


static irq_handler_t irq_input_handler(int, void *, struct pt_regs *);

struct file_operations fops=
  {
    .open=icdev_open,
    .release=icdev_release,
    .read=icdev_read,
    .unlocked_ioctl=icdev_ioctl
  };


static int icdev_open(struct inode *inode, struct file *file)
{
  if (mutex_trylock(&icdev_Ptr->io_mutex) == 0) {
    printk(KERN_ERR "Device Busy!\n");
    return -EBUSY;
  }

  if (icdev_Ptr->is_open == 1) {
    printk(KERN_ERR "%s:Device already open!\n",DEVICE_NAME);
    return -EBUSY;
  }

  /*
   * TODO:gpio & interrupt initialization
   */

  icdev_Ptr->is_open = 1;
  mutex_unlock(&icdev_Ptr->io_mutex);
  
  
  return 0;
}


static int icdev_release(struct inode *inode, struct file *file)
{

  mutex_lock(&icdev_Ptr->io_mutex);
  /*
   * TODO: gpio & interrupt clean
   */ 


  icdev_Ptr->is_open=0;
  mutex_unlock(&icdev_Ptr->io_mutex);
  
  return 0;
}

static ssize_t icdev_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
  if (mutex_trylock(&icdev_Ptr->io_mutex) == 0) {
    printk(KERN_ERR "Device Busy!\n");
    return -EBUSY;
  }
  icdev_value=0xAAULL;/*TODO: Remove with actual calculation*/
  if (copy_to_user(buffer,&icdev_value,sizeof(u32)) != 0) {
    mutex_unlock(&icdev_Ptr->io_mutex);
    return -EINVAL;
  }

  mutex_unlock(&icdev_Ptr->io_mutex);
  return sizeof(u32);
}

static long icdev_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
  return 0L;
}


static irq_handler_t irq_input_handler(int irq, void *dev_id, struct pt_regs *regs)
{
  return (irq_handler_t) IRQ_HANDLED;
}





static s8 alloc_device(void)
{
  icdev_Ptr=kmalloc(sizeof(struct ic_device),GFP_KERNEL);
  if (icdev_Ptr == NULL) {
    printk(KERN_WARNING "%s:Failed to alloc memory for ic_device\n",DEVICE_NAME);
    return -1;
  }
  memset(icdev_Ptr,0,sizeof(struct ic_device));
  if (alloc_chrdev_region(&icdev_no,0,1,DEVICE_NAME) < 0) {
    printk(KERN_WARNING "%s:Could not register\n",DEVICE_NAME);
    FREE_DEVICE();
    return -1;
  }
  return 0;
}


static s8 init_class(void)
{
  icdev_class_Ptr=class_create(THIS_MODULE,DEVICE_CLASS_NAME);
  if (IS_ERR(icdev_class_Ptr)) {
    printk(KERN_WARNING "%s:Could not create class\n",DEVICE_NAME);
    UNREGISTER_REGION();
    FREE_DEVICE();
    return -1;
  }
  return 0;
}

static s8 init_device_registration(void)
{
  cdev_init(&icdev_Ptr->cdev,&fops);
  icdev_Ptr->cdev.owner = THIS_MODULE;
  if (cdev_add(&(icdev_Ptr->cdev),icdev_no,1) != 0) {
    printk(KERN_WARNING "%s:Could not add device\n",DEVICE_NAME);
    DESTROY_CLASS();
    UNREGISTER_REGION();
    FREE_DEVICE();
    return -1;
  }

  icdev_Ptr->device_Ptr = device_create(icdev_class_Ptr,NULL,MKDEV(MAJOR(icdev_no),0),NULL,DEVICE_PROCESS,0);
  if (IS_ERR(icdev_Ptr->device_Ptr)) {
    printk(KERN_WARNING "%s:Could not create device\n",DEVICE_NAME);
    DESTROY_DEVICE();
    DESTROY_CLASS();
    UNREGISTER_REGION();
    FREE_DEVICE();
    return -1;
  }
  return 0;

}

/*
 * Driver Initialization
 */
static int __init ic_init(void)
{
  s8 init_error = 0;
  init_error=alloc_device();
  if (init_error == -1)
    goto ic_init_error;

  init_error = init_class();
  if (init_error == -1)
    goto ic_init_error;
 
  init_error = init_device_registration();
  if (init_error == -1)
    goto ic_init_error;

 
  mutex_init(&(icdev_Ptr->io_mutex));
  printk(KERN_INFO "%s:Registered device with (%d,%d)\n",DEVICE_NAME,MAJOR(icdev_no),MINOR(icdev_no));
  printk(KERN_INFO "Driver %s loaded. Version %2x %2x %2x",DEVICE_NAME,
	 VERSION_MAJOR_NUMBER,VERSION_MINOR_NUMBER,VERSION_PATCH_NUMBER);
  return 0;
  
  
  ic_init_error:
  return -EBUSY;
  
}

static void __exit ic_exit(void)
{
  printk(KERN_INFO "%s:Unregister...:(",DEVICE_NAME);
  if (icdev_Ptr != NULL) {
    DESTROY_DEVICE();
    FREE_DEVICE();
  }
  unregister_chrdev_region(icdev_no,1);
  if (icdev_class_Ptr != NULL) {
    DESTROY_CLASS();
  }
  printk(KERN_INFO "Driver %s unloaded.",DEVICE_NAME);
  
  
  
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("device")

module_init(ic_init);
module_exit(ic_exit);
