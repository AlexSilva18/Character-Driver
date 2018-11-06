#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "mymodule.h"

#define DEVICE "/dev/testDevice"

int main (){
  int i, fd;
  char ch, write_buf[100], read_buf[100];
  int rc, input;
  
  fd = open(DEVICE, O_RDWR);

  if(fd == -1){
    printf("file %s either DNE or is locked\n", DEVICE);
    exit(-1);
  }

  printf("r = read from device\nw = write to device\nenter: ");
  //scanf("%c", &ch);
  scanf("%d", &input);

  //switch(ch){
  switch(input){
    //case 'w':
  case 0:
    //printf("enter data: ");
    //scanf("%s", write_buf);
    printf("User Create file\n");
      rc = ioctl(fd, IOCTL_GET_CREATE, 0);
      printf("rc is: %d\n", rc);
      /* write(fd, write_buf, sizeof(write_buf)); */
    break;
  case 1:
    printf("User destroy file\n");
    rc = ioctl(fd, IOCTL_DESTROY, 0);
    break;
    
    
    case 'r':
      read(fd, read_buf, sizeof(read_buf));
      printf("device: %s\n", read_buf);
      break;

    default:
      printf("input not recognized\n");
      break;
  }

  close(fd);
  return 0;
  
}
