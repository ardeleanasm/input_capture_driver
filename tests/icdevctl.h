#ifndef ICDEVCTL_H_
#define ICDEVCTL_H_

#define _IOCTL_MAGIC 'K'
#define IOCICDEVGPIORP _IOW(_IOCTL_MAGIC,1,uint16_t) /* register pin */
#define IOCICDEVGPIOUP _IOW(_IOCTL_MAGIC,2,uint16_t) /* unregister ping */
#define IOCICDEVGPIOEN _IO(_IOCTL_MAGIC,3)       /* start */
#define IOCICDEVGPIODS _IO(_IOCTL_MAGIC,4)       /* stop  */

#endif