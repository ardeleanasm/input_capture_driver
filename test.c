#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv)
{
  int fd;
  uint32_t read_value;
  int err=0;
  fd=open("/dev/ic0",O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Open failed!%s",strerror(errno));
    return -1;
  }

  if ((err = read(fd,&read_value,sizeof(uint32_t))) < 0){
    fprintf(stderr, "Read failed! %s",strerror(errno));
    close(fd);
    return -1;
  }

  printf("Read %d bytes. Value %x\n",err,read_value);
  close(fd);
  
  return 0;
}
