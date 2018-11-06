#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "mymodule.h"

#define DEVICE "/dev/testDevice"

int main (){
  int keys[200];
  int i, fd;
  i = 0;
  char ch, write_buf[100], read_buf[100];
  int rc, key, input;
  
  fd = open(DEVICE, O_RDWR);

  if(fd == -1){
    printf("file %s either DNE or is locked\n", DEVICE);
    exit(-1);
  }

  //printf("r = read from device\nw = write to device\nenter: ");
  //scanf("%c", &ch);
  printf("(enter 5 to display all options)\n");
  for(;;){
    printf("enter option: ");
    scanf("%d", &input);
    if (input < 0 || input > 6){
      printf("invalid input\n");
      continue;
    }
    //switch(ch){
    switch(input){
      //case 'w':
    case 0:
      //printf("enter data: ");
      //scanf("%s", write_buf);
      printf("User Create file\n");
      key = ioctl(fd, IOCTL_CREATE_DEVICE, 0);
      if (key == -1){
	printf("Unable to create file\n");
      }
      printf("key is: %d\n", key);
      /* if (i > 10){ */
      /* 	keys = realloc(keys, sizeof(key)*10); */
      /* } */

      keys[i] = key;
      ++i;
      /* write(fd, write_buf, sizeof(write_buf)); */
      continue;
    case 1:
      printf("User destroy file\n");
      printf("enter device to destroy: ");
      scanf("%s", write_buf);
      write(fd, write_buf, sizeof(write_buf));
      rc = ioctl(fd, IOCTL_DESTROY_DEVICE, 0);
      if (rc == -1){
	printf("Unable to destroy device\n\n");
	continue;
      }
      --i;
      continue;

    case 4:
      printf("keys:\n");
      for (int j = 0; j <= i; j++){
	printf("%d ", keys[j]);
      }
      printf("\n\n");
      continue;
    case 5:
      printf("enter 0 - CREATE DEVICE\n");
      printf("enter 1 - DESTROY DEVICE\n");
      printf("enter 5 - SHOW ALL OPTIONS\n");
      printf("enter 6 - END APPLICATION\n");
      printf("\n");
      continue;
    case 6:
      //free(keys);
      return 0;
      
      /* case 'r': */
      /*   read(fd, read_buf, sizeof(read_buf)); */
      /*   printf("device: %s\n", read_buf); */
      /*   break; */
      
    default:
      printf("input not recognized\n");
      continue;
    }
  }

  close(fd);
  return 0;
  
}
