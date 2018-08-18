#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
#include "icdevctl.h"

#define GROUP 0
#define READ_PIN 20

static volatile int running = 1;


void signal_handler(int signo)
{
	running = 0;
}

int main(int argc, char **argv)
{
  int fd;
  fd_set read_fd;
  uint64_t read_value;
  int val=0;
  const uint16_t gpio_number=32*GROUP+READ_PIN;


  signal(SIGTERM, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGINT, signal_handler);
  fd=open("/dev/ic0",O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Open failed!%s",strerror(errno));
    return -1;
  }

  if(ioctl(fd,IOCICRE)!=0){
    fprintf(stderr,"IOCICRE(Rising Edge):%s\n",strerror(errno));
    goto error;
  }

  if(ioctl(fd,  IOCICUP,&gpio_number)!=0){
    fprintf(stderr,"IOCICUP(UP error):%s\n",strerror(errno));
    goto error;
  }

  while(running) {
    FD_ZERO(&read_fd);
    FD_SET(fd, &read_fd);

    val = select(fd, &read_fd, NULL, NULL, NULL);
    /* From this line, the process has been notified already */
    if (val == -1) {
      fprintf(stderr, "Select error %s", strerror(errno));
      goto unregister_error;
    }

    if (FD_ISSET(fd, &read_fd)) {
      val = read(fd, &read_value, sizeof(uint64_t));
      if (val < 0 ){
	fprintf(stderr,"Read error %s\n",strerror(errno));
	goto unregister_error;
      }
      else{
	printf("Read %llu\n",read_value);
      }
    }
  }

 unregister_error:
  if(ioctl(fd,  IOCICDW,&gpio_number)!=0){
    fprintf(stderr,"IOCICDW(Down error):%s\n",strerror(errno));
  }
 error:  
  close(fd);
  return errno;


}
