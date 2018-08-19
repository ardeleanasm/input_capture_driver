#ifndef ICDEVCTL_H_
#define ICDEVCTL_H_

#define _IOCTL_MAGIC 'K'
#define IOCICUP _IOW(_IOCTL_MAGIC,1,u16) /* register pin */
#define IOCICDW _IOW(_IOCTL_MAGIC,2,u16) /* unregister ping */
#define IOCICFE _IO(_IOCTL_MAGIC,3) /*Capture timer value on every falling edge*/
#define IOCICRE _IO(_IOCTL_MAGIC,4) /*Capture timer value on every rising edge*/
#define IOCICRF _IO(_IOCTL_MAGIC,5) /*Capture timer value on rising and falling*/

#define ICEBIRQN 0x01
#define ICEBGPIO 0x02


#endif
