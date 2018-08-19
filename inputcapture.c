/*
  input capture driver
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
#include <linux/gpio.h>
#include <linux/poll.h>
#ifndef ARM_CPU /*if is not running on ARM CPU then is ok to use get_cycles*/
#include <linux/timex.h>
#else
#include <linux/ktime.h> /*make use of getnstimeofday()*/
#endif

#define DRIVER_AUTHOR "23ars <ardeleanasm@gmail.com>"
#define DRIVER_DESC "Input Capture driver"
#define DEVICE_NAME "ic"
#define DEVICE_CLASS_NAME "ic_class"
#define DEVICE_PROCESS "ic%d"

#define VERSION_MAJOR_NUMBER 0x01u
#define VERSION_MINOR_NUMBER 0x00u
#define VERSION_PATCH_NUMBER 0x01u


#define icdev_free_device() \
  kfree(icdev_Ptr);	   \
  icdev_Ptr = NULL

#define icdev_unregister_region() unregister_chrdev_region(icdev_no,1)

#define icdev_destroy_class() \
  class_destroy(icdev_class_Ptr);		\
  icdev_class_Ptr = NULL

#define icdev_destroy_device() \
  device_destroy(icdev_class_Ptr,MKDEV(MAJOR(icdev_no),0));	\
  cdev_del(&(icdev_Ptr->cdev))


#define _IOCTL_MAGIC 'K'
#define IOCICUP _IOW(_IOCTL_MAGIC,1,u16) /* register pin */
#define IOCICDW _IOW(_IOCTL_MAGIC,2,u16) /* unregister ping */
#define IOCICFE _IO(_IOCTL_MAGIC,3) /*Capture timer value on every falling edge*/
#define IOCICRE _IO(_IOCTL_MAGIC,4) /*Capture timer value on every rising edge*/
#define IOCICRF _IO(_IOCTL_MAGIC,5) /*Capture timer value on rising and falling*/

struct icdev_data{
  u64 data;
  bool is_data_updated;
};


struct ic_device {
  struct cdev cdev;
  struct device *device_Ptr;
  struct mutex io_mutex;
  u8 is_open;
};

static struct ic_device *icdev_Ptr = NULL;
static dev_t icdev_no;
static struct class *icdev_class_Ptr = NULL;
static u16 ioctl_read_value = 0x00u;
static u32 icdev_detect_level = IRQF_TRIGGER_NONE;

static struct icdev_data icdev_value=
  {
    .data = 0x00ull,
    .is_data_updated = false
  };




volatile int icdev_irq_no = -1;

static int icdev_open(struct inode *, struct file *);
static int icdev_release(struct inode *, struct file *);
static ssize_t icdev_read(struct file *, char __user *, size_t, loff_t *);
static unsigned int icdev_poll(struct file *, poll_table *);
static long icdev_ioctl(struct file *, unsigned int, unsigned long);
static irq_handler_t icdev_irq_handler(int, void *);

static DEFINE_SPINLOCK(event_rwlock);
static DECLARE_WAIT_QUEUE_HEAD(icdev_waitq);
				
struct file_operations fops=
  {
    .open = icdev_open,
    .release = icdev_release,
    .read = icdev_read,
    .poll = icdev_poll,
    .unlocked_ioctl = icdev_ioctl
  };


static int icdev_open(struct inode *inode, struct file *file)
{
  if (mutex_trylock(&icdev_Ptr->io_mutex) == 0) {
    printk(KERN_ERR "Device Busy!\n");
    return -EBUSY;
  }

  if (icdev_Ptr->is_open == 1) {
    printk(KERN_ERR "%s:Device already open!\n",DEVICE_NAME);
    mutex_unlock(&icdev_Ptr->io_mutex);
    return -EBUSY;
  }
  icdev_Ptr->is_open = 1;
  mutex_unlock(&icdev_Ptr->io_mutex);
  
  return 0;
}


static int icdev_release(struct inode *inode, struct file *file)
{
  if (!mutex_is_locked(&icdev_Ptr->io_mutex))
    mutex_lock(&icdev_Ptr->io_mutex);
  icdev_Ptr->is_open = 0;
  mutex_unlock(&icdev_Ptr->io_mutex);
  
  return 0;
}

static ssize_t icdev_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
  
  unsigned long flags;
  u64 buffer_value = 0x00ull;

  spin_lock_irqsave(&event_rwlock, flags);
  buffer_value = icdev_value.data;
  spin_unlock_irqrestore(&event_rwlock, flags);

  if (copy_to_user(buffer,&buffer_value,sizeof(u64)) != 0) {
    return -EINVAL;
  }
  if (buffer_value == 0ull)
    return 0;
  return sizeof(u64);
}

static unsigned int icdev_poll(struct file *file, poll_table *wait)
{
  unsigned long flags;
  unsigned int reval_mask = 0x00u;
  poll_wait(file,&icdev_waitq,wait);
  spin_lock_irqsave(&event_rwlock,flags);
  if (icdev_value.is_data_updated != false ) {
    reval_mask |= (POLLIN | POLLRDNORM);
  }
  icdev_value.is_data_updated = false;
  spin_unlock_irqrestore(&event_rwlock,flags);
  return reval_mask;
}

static long icdev_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
  u16 __user *ioctl_value_Ptr;
  long error_code = 0x00L;
  ioctl_read_value = 0x00ul;

  ioctl_value_Ptr = (u16 __user *)ioctl_param;
  if (copy_from_user(&ioctl_read_value,ioctl_value_Ptr,sizeof(u16)) != 0) {
    return -EINVAL;
  }

  switch(ioctl_num){
  case IOCICUP:/* register pin */
    pr_err("\tIOCICUP:Gpio Pin %d",ioctl_read_value);
    if (gpio_is_valid(ioctl_read_value)) {
      gpio_request(ioctl_read_value,"sysfs");/*TODO: Check if error -> !=0*/
      gpio_direction_input(ioctl_read_value);
      gpio_export(ioctl_read_value,false);
      /*request interrupt*/
      icdev_irq_no = gpio_to_irq(ioctl_read_value);
      if (request_irq(icdev_irq_no,(irq_handler_t)icdev_irq_handler,icdev_detect_level,
		      DEVICE_NAME, NULL)){
	icdev_irq_no = -1;
	pr_err("Request irq error");
      } 
    } else {
      pr_err("INVALID GPIO");
    }
    break;
  case IOCICDW:/* unregister pin */
    if (icdev_irq_no != -1) {
      free_irq(icdev_irq_no, NULL);
    }
    icdev_detect_level = IRQF_TRIGGER_NONE;
    gpio_unexport(ioctl_read_value);
    gpio_free(ioctl_read_value);
    break;
  case IOCICFE: /*Capture timer value on every falling edge*/
    icdev_detect_level = IRQF_TRIGGER_FALLING;
    break;
  case IOCICRE: /*Capture timer value on every rising edge*/
    icdev_detect_level = IRQF_TRIGGER_RISING;
    break;
  case IOCICRF:
    icdev_detect_level = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
    break;
  default:
    break;
  }
  return error_code;
}




static irq_handler_t icdev_irq_handler(int irq, void *dev_id)
{

#ifdef ARM_CPU
  struct timespec64 t;
#endif
  int value = 0x00;
  value = gpio_get_value(ioctl_read_value);
  pr_err("\tInterrupt:Read Value %d", value);

  spin_lock(&event_rwlock);
#ifndef ARM_CPU
  icdev_value.data = get_cycles(); /* get_cycles return clock counter value except for arm and 486*/
#else
  ktime_get_real_ts64(&t); /*alias getnstimeofday is deprecated for new code*/
  icdev_value.data = t.tv_sec*1000000000ull + t.tv_nsec;/*transform everything to nsec*/
#endif
  pr_devel("Value read than 0, timer value %llu",icdev_value.data);
  wake_up_interruptible(&icdev_waitq);
  icdev_value.is_data_updated = false;
  spin_unlock(&event_rwlock);
  return (irq_handler_t) IRQ_HANDLED;
}


static s8 alloc_device(void)
{
  icdev_Ptr = kmalloc(sizeof(*icdev_Ptr),GFP_KERNEL);
  if (icdev_Ptr == NULL) {
    printk(KERN_WARNING "%s:Failed to alloc memory for ic_device\n",DEVICE_NAME);
    return -1;
  }
  memset(icdev_Ptr,0,sizeof(struct ic_device));
  if (alloc_chrdev_region(&icdev_no,0,1,DEVICE_NAME) < 0) {
    printk(KERN_WARNING "%s:Could not register\n",DEVICE_NAME);
    icdev_free_device();
    return -1;
  }
  return 0;
}


static s8 init_class(void)
{
  icdev_class_Ptr = class_create(THIS_MODULE,DEVICE_CLASS_NAME);
  if (IS_ERR(icdev_class_Ptr)) {
    printk(KERN_WARNING "%s:Could not create class\n",DEVICE_NAME);
    icdev_unregister_region();
    icdev_free_device();
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
    icdev_destroy_class();
    icdev_unregister_region();
    icdev_free_device();
    return -1;
  }

  icdev_Ptr->device_Ptr = device_create(icdev_class_Ptr,NULL,MKDEV(MAJOR(icdev_no),0),NULL,DEVICE_PROCESS,0);
  if (IS_ERR(icdev_Ptr->device_Ptr)) {
    printk(KERN_WARNING "%s:Could not create device\n",DEVICE_NAME);
    icdev_destroy_device();
    icdev_destroy_class();
    icdev_unregister_region();
    icdev_free_device();
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
  init_error = alloc_device();
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

  if (icdev_class_Ptr != NULL) {
    icdev_destroy_device();
    icdev_destroy_class();
  }

  icdev_unregister_region();
  
  if (icdev_Ptr != NULL) {
    icdev_free_device();
  }

  printk(KERN_INFO "Driver %s unloaded.",DEVICE_NAME);
 
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("device")

module_init(ic_init);
module_exit(ic_exit); 
