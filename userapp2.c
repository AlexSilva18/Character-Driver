#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <ctype.h>
#include "mymodule2.h"

#define DEVICE "/dev/testDevice"
char *get_index(char*);
char *get_input(int);

int main () {
  /* int ret, fd, input, key, i; */
  /* char * return; */
  /* char ch, write_buf[2048], read_buf[2048]; */
  /* int keys[200]; */

  int indexes[200];
  char *keys[30];
  char encrypt_device[20] = "/dev/cryptEncrypt";
  char decrypt_device[20] = "/dev/cryptDecrypt";
  int i, fd, device_fd;
  char device_index[4];
  const char *currChar;
  /* char index[4]; */
  char *index;
  
  i = 0;
  /* char ch, write_buf[100],temp_buf[100], read_buf[100]; */
  char ch, *write_buf,temp_buf[100], read_buf[100];
  int rc, ret, input, rtc;
  /* int index; */
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
      printf("enter 3 - CHANGE DEVICE KEY\n");
      printf("enter 4 - ENCRYPT MESSAGE\n");
      printf("enter 5 - DECRYPT MESSAGE\n");
      printf("enter 6 - DISPLAY ALL DEVICE INDEXES/KEYS\n");
      printf("enter 7 - END APPLICATION\n\n");
      continue;

/*
###############################################################
*/
    case 1:
      printf("enter key: ");
      scanf("%s", temp_buf);
      write(fd, temp_buf, sizeof(temp_buf));
      
      printf("User Create file\n");
      ret = ioctl(fd, IOCTL_CREATE_DEVICE, 0);
      
      if (ret == -1){
	printf("Unable to create file\n");
      }
      // add items to list
      /* indexes[] += index; */
      /* keys[] += write_buf; */
      printf("Device has been created!\ndevice index: %d\n\n", ret);
      continue;

/*
###############################################################
*/
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
      printf("Device has been destroyed! \n\n");
      continue;

/*
###############################################################
*/
    case 3:
      printf("User change key.\n");

      write_buf = get_input(0);
      printf("write_buf: %s \n", write_buf);
      
      index = get_index(write_buf);
      printf("index: %s\n", index);
      
      strcat(encrypt_device, (index + '\0'));
      printf("encrypt_device: %s\n", encrypt_device);

      int priv = chmod(encrypt_device, 0666);
      device_fd = open(encrypt_device, O_RDWR);
  
      if(device_fd == -1){
      	printf("device %s either DNE or is locked\n", encrypt_device);
      	exit(-1);
      }

      rc = write(fd, write_buf, sizeof(write_buf));
	       
      if (rc < 0) {
      	perror("Failed to write message to the device");
      	//return errno;
      	continue;
      }
      /* rc = ioctl(fd, IOCTL_CHANGE_KEY, 0); */
      /* if (rc < 0) { */
      /* 	printf("Unable to change key\n"); */
      /* 	continue; */
      /* } */
      
      printf("Key has been updated!\n\n");
      continue;

/*
###############################################################
*/
    case 4:
      printf("User encrypt device.\n");

      write_buf = get_input(1);
      printf("write_buf: %s \n", write_buf);
      
      index = get_index(write_buf);
      printf("index: %s\n", index);
      
      strcat(encrypt_device, index);
      printf("encrypt_device: %s\n", encrypt_device);
      
      device_fd = open(encrypt_device, O_RDWR);
      
      if(device_fd == -1){
      	printf("device %s either DNE or is locked\n", encrypt_device);
      	exit(-1);
      }

      rc = write(device_fd, write_buf, sizeof(write_buf));
      
      if (rc < 0) {
	perror("Failed to write message to the device");
	//	return errno;
	continue;
      }
      printf("writing successful\n");
      /* ret = ioctl(fd, IOCTL_ENCRYPT, 0); */
      /* if (ret < 0) { */
      /* 	printf("Unable to encrypt text\n"); */
      /* 	continue; */
      /* } */
      if(read(fd, read_buf, sizeof(read_buf)) < 0){
	printf("Error reading from device\n");
	continue;
      }

      //read(device_fd, read_buf,sizeof(read_buf));
      printf("Encrypted text: %s\n\n", read_buf);
      close(device_fd);
      continue;

/*
###############################################################
*/
    case 5:
      printf("User decrypt device.\n");

      printf("User encrypt device.\n");

      write_buf = get_input(2);
      printf("write_buf: %s \n", write_buf);
      
      index = get_index(write_buf);
      printf("index: %s\n", index);
      
      strcat(decrypt_device, (index + '\0'));
      printf("decrypt_device: %s\n", decrypt_device);
      
      /* device_fd = open(encrypt_device, O_RDWR); */
      
      /* if(device_fd == -1){ */
      /* 	printf("device %s either DNE or is locked\n", DEVICE); */
      /* 	exit(-1); */
      /* } */

      rc = write(fd, write_buf, sizeof(write_buf));
      if (rc < 0) {
	perror("Failed to write message to the device");
	/* return errno; */
	continue;
      }
      ret = ioctl(fd, IOCTL_DECRYPT, 0);
      
      continue;

/*
###############################################################
*/
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
      
/*
###############################################################
*/

    case 7:
        return 0;
	
    default:
        printf("Input not recognized...\n");
	break;
        //continue;
    }
  }
  free(write_buf);
  free(index);
  close(fd);
  return 0;

}

char *get_index(char* input){
  const char *currChar;
  char *index;
  char temp_index[4];
  
  int i = 0;
  for(currChar = input; *currChar != '\0'; currChar+=1){
    if (*currChar == '#'){
      temp_index[i] = '\0';
      break;
    }
    temp_index[i] = *currChar;
    i++;
  }
  index = temp_index;
  return index;
}

char *get_input(int flag){
  char *write_buf,temp_buf[100];

  if (flag == 0){
    printf("Enter device index: ");
    scanf("%s", write_buf);
    strcat(write_buf, "#");
    printf("Enter new key: ");
    scanf("%s", temp_buf);
    strcat(write_buf, (temp_buf + '\0'));
  }
  if (flag == 1){
    printf("Enter device index: ");
    scanf("%s", write_buf);
    strcat(write_buf, "#");
    printf("Enter text to be encrypted:\n");
    scanf("%s", temp_buf);
    strcat(write_buf, (temp_buf + '\0'));
    
  }
  if (flag == 2){
    printf("Enter device index: ");
    scanf("%s", write_buf);
    strcat(write_buf, "#");
    printf("Enter text to be decrypted:\n");
    scanf("%s", temp_buf);
    strcat(write_buf, (temp_buf + '\0'));
  }
  return write_buf;
}
