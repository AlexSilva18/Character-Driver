#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include "mymodule2.h"

#define DEVICE "/dev/testDevice"

int main () {
  /* int ret, fd, input, key, i; */
  /* char * return; */
  /* char ch, write_buf[2048], read_buf[2048]; */
  /* int keys[200]; */

  int indexes[200];
  char *keys[30];
  int i, fd;
  i = 0;
  char ch, write_buf[100], read_buf[100];
  int rc, ret, index, input;

  printf("Starting userspace program...\n");
  
  fd = open(DEVICE, O_RDWR);
  
  if(fd == -1){
    printf("file %s either DNE or is locked\n", DEVICE);
    exit(-1);
  }

  printf("(enter 0 to display all options)\n");
  for (;;) {
    // get user input
    printf("Enter option: ");
    scanf("%d", &input);
    if (input < 0 || input > 7) {
      printf("Invalid input. Try again.\n");
      continue;
    }

    switch(input) {
    case 0:
      printf("enter 0 - SHOW ALL OPTIONS\n");
      printf("enter 1 - CREATE DEVICE\n");
      printf("enter 2 - DESTROY DEVICE\n");
      printf("enter 3 - ENCRYPT MESSAGE\n");
      printf("enter 4 - DECRYPT MESSAGE\n");
      printf("enter 5 - CHANGE DEVICE KEY\n");
      printf("enter 6 - DISPLAY ALL DEVICE INDEXES\n");
      printf("enter 7 - END APPLICATION\n");
      printf("\n");
      continue;

    case 1:
      printf("enter key: ");
      scanf("%s", write_buf);
      write(fd, write_buf, sizeof(write_buf));
      
      printf("User Create file\n");
      index = ioctl(fd, IOCTL_CREATE_DEVICE, 0);
      
      if (index == -1){
	printf("Unable to create file\n");
      }
      // add items to list
      /* indexes[] += index; */
      /* keys[] += write_buf; */
      printf("index is: %d\n", index);
      continue;
      
    case 2:
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
      // remove items to list
      /* indexes[] -= index; */
      /* keys[] -= write_buf; */
      continue;
      
    case 3:
      printf("User change key.\n");
      printf("Enter <key> <dev index>: ");
      scanf("%s", write_buf);
      
      /* printf("Enter <dev index>: "); */
      /* scanf("%s", write_buf); */
      /* strcat(write_buf, "#"); */
      /* printf("Enter <key> "); */
      /* scanf("%s", temp_buff); */
      /* strcat(write_buf, (temp_buff + '\0')); */
      strcat(write_buf, "\0");
      rc = write(fd, write_buf, sizeof(write_buf));

      if (rc < 0) {
	perror("Failed to write message to the device");
	//return errno;
	continue;
      }
      rc = ioctl(fd, IOCTL_CHANGE_KEY, 0);
      if (rc < 0) {
	printf("Unable to change key\n");
	continue;
      }
      continue;
	
    case 4:
      printf("User encrypt device.\n");
      printf("Enter <dev index> <plain text> to be encrypted: ");
      scanf("%s", write_buf);

      rc = write(fd, write_buf, sizeof(write_buf));
      if (rc < 0) {
	perror("Failed to write message to the device");
	//	return errno;
	continue;
      }
      ret = ioctl(fd, IOCTL_ENCRYPT, 0);
      continue;
    case 5:
      printf("User decrypt device.\n");
      printf("Enter <dev index> <cipher text> to be decrypted: ");
      scanf("%s", write_buf);

      rc = write(fd, write_buf, sizeof(write_buf));
      if (rc < 0) {
	perror("Failed to write message to the device");
	/* return errno; */
	continue;
      }
      ret = ioctl(fd, IOCTL_DECRYPT, 0);
      continue;

    case 6:
      printf("Indexes: ");
      for (int j = 0; j <= i; j++){
	printf("%d \t", indexes[j]);
      }
      printf("\nKeys: ");
      for (int j = 0; j <= i; j++){
	printf("%s \t", keys[j]);
      }
      printf("\n\n");

      continue;
      
    case 7:
        return 0;
	
    default:
        printf("Input not recognized...\n");
        continue;
    }
  }

  close(fd);
  return 0;

}
