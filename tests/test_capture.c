#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "icdevctl.h"

#define GROUP 0
#define READ_PIN 20

int main(int argc, char **argv)
{
  int fd;
  uint32_t read_value;
  int err=0;
  
  const uint16_t gpio_number=32*GROUP+READ_PIN;
  int i=0;
  fd=open("/dev/ic0",O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Open failed!%s",strerror(errno));
    return -1;
  }
  
  //register pin
  if(ioctl(fd,IOCICDEVGPIORP,&gpio_number)!=0){
    fprintf(stderr,"IOCICDEVGPIORP:%s\n",strerror(errno));
    goto error;
  }


  //start
  if(ioctl(fd,IOCICDEVGPIOEN,&gpio_number)!=0){
    fprintf(stderr,"IOCICDEVGPIOEN:%s\n",strerror(errno));
    goto error;
  }

  while(i<10)
  {
    sleep(1);
    printf("Sleep...%d \n",i);
    i++;
  }

  if ((err = read(fd,&read_value,sizeof(uint32_t))) < 0){
    fprintf(stderr, "Read failed! %s",strerror(errno));
    close(fd);
    return -1;
  }

  printf("Read %d bytes. Value %x\n",err,read_value);

  //stop
  if(ioctl(fd,IOCICDEVGPIODS,&gpio_number)!=0){
    fprintf(stderr,"IOCICDEVGPIODS:%s\n",strerror(errno));
    goto error;
  }
  //unregister pin
  if(ioctl(fd,IOCICDEVGPIOUP,&gpio_number)!=0){
    fprintf(stderr,"IOCICDEVGPIOUP:%s\n",strerror(errno));
    goto error;
  }

  close(fd);
  return 0;
  error:
    close(fd);
    return errno;
}
