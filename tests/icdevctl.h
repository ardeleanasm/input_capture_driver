#ifndef ICDEVCTL_H_
#define ICDEVCTL_H_

#define _IOCTL_MAGIC 'K'
#define IOCICUP _IOW(_IOCTL_MAGIC,1,uint16_t) /* register pin */
#define IOCICDW _IOW(_IOCTL_MAGIC,2,uint16_t) /* unregister ping */
#define IOCICFE _IO(_IOCTL_MAGIC,3) /*Capture timer value on every falling edge*/
#define IOCICRE _IO(_IOCTL_MAGIC,4) /*Capture timer value on every rising edge*/
#define IOCICRF _IO(_IOCTL_MAGIC,5) /*Capture timer value on rising and falling*/


#define ICDEV_DETECT_RISING_EDGES 0x01u
#define ICDEV_DETECT_FALLING_EDGES 0x02u
#define ICDEV_DETECT_BOTH_EDGES 0x03u

#endif
