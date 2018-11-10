/**
 * A character driver for the Linux loadable kernel module (LKM).
 * This module maps to /dev/cryptctl and comes with a helper C program that
 * can be run in Linux userspace to communicate with this LKM.
 * Has semaphore locks to deal with synchronication problems.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include "mymodule2.h"

#define ENCRYPT_CLASS "EncryptClass19"
#define DECRYPT_CLASS "DecryptClass19"

#define CLASS_NAME_MODULE "modClass"


/* #define DEVICE_NAME "cryptctl" // The device will appear at /dev/cryptctl using this value */
/* #define CLASS_NAME "crypt" // The device class - this is a character device driver */

/* #define ENCRYPT_CLASS "EncryptClass18" */
/* #define DECRYPT_CLASS "DecryptClass18" */

/* #define CLASS_NAME_MODULE "modClass" */


struct newDevice {
  char data[256]; // Memory for the string that is passed from userspace
  short size_of_message;
  int numberOpens; // Counts the number of times the device is opened
  struct semaphore sem;
} virtualDevice;

struct idNode {
  char key[100];
  dev_t dev;
  char name[100];
  struct class *cls;
  char index[4];
  char data[256];
};

MODULE_LICENSE("GPL");

//structs for creating our module
struct cdev *mcdev;
struct class *cl;



//int majorNumber;
struct class * cryptctlClass = NULL; // The device-driver class struct pointer
struct device * cryptctlDevice = NULL; // The device-driver device struct pointer

//int device_number = 0;
struct device * encryptDevice = NULL;
struct device * decryptDevice = NULL;

/* // class for creating encrypt/decrypt */
static struct class *crypt_class_encrypt;
static struct class *crypt_class_decrypt;
static struct device* new_encrypt_device = NULL;
static struct device* new_decrypt_device = NULL;



char temp_encrypt_device[17];
char temp_decrypt_device[17];

int currMajNumEnc = 0;
int currMinNumEnc = 0;
int currMajNumDec = 0;
int currMinNumDec = 0;

int numEncDevices = 0; //number of encrypt devices we've created
int numDecDevices = 0;

struct cdev *charDevEncrypt;
struct cdev *charDevDecrypt;

struct cdev encrypt_cdev, decrypt_cdev;
struct idNode encIdNodes[100];
struct idNode decIdNodes[100];

// boolean vars to check if we allocated space for the drivers
int boolAllocEncrypt = 0;
int boolAllocDecrypt = 0;

int retval;
int flag = 0;
int numChars = -1;
int boolCryptCreated = 0;
int device_number = 0;
int currNodeIndEnc = -1;
int currNodeIndDec = -1;

// structs for device destruction
struct idNode destroyEncNode;
struct idNode destroyDecNode;
struct idNode currNode;
struct idNode *tempEnc0;
struct idNode *tempDec0;
dev_t firstDevEnc, firstDevDec;

/*
#####################END OF VARIABLE DECLARATIONS######################################
*/


static int device_open (struct inode *, struct file *);
static ssize_t device_read (struct file *, char *, size_t, loff_t *);
static ssize_t device_write (struct file *, const char *, size_t, loff_t *);
static int device_release (struct inode *, struct file *);
static long device_ioctl (struct file *, unsigned int, unsigned long);
static int driver_uevent(struct device *, struct kobj_uevent_env *);

static ssize_t encrypt_read (struct file *, char *, size_t, loff_t *);
static ssize_t encrypt_write (struct file *, const char *, size_t, loff_t *);

static ssize_t decrypt_read (struct file *, char *, size_t, loff_t *);
static ssize_t decrypt_write (struct file *, const char *, size_t, loff_t *);

/**
 * Devices are represented as file structures in the kernel.
 * The file_operations structure from /linux/fs.h lists the callback functions that you wish
 * to associate with your file operations.
 * Char devices usually implement open, read, write, and release calls.
*/
struct file_operations fops = {
  .open = device_open,
  .read = device_read,
  .write = device_write,
  .release = device_release,
  .unlocked_ioctl = device_ioctl,
};

struct file_operations enc_fops = {
  .open = device_open,
  .read = encrypt_read,
  .write = encrypt_write,
  .release = device_release,
  .unlocked_ioctl = device_ioctl,

};

struct file_operations dec_fops = {
  .open = device_open,
  .read = decrypt_read,
  .write = decrypt_write,
  .release = device_release,
  .unlocked_ioctl = device_ioctl,
};

static int driver_uevent(struct device *dev, struct kobj_uevent_env *env){
  add_uevent_var(env, "DEVMODE=%#o", 0666);
  return 0;
}


/* void vinegere_cipher (int arg, int devIndex, char * text) { */
//void vinegere_cipher (int arg, struct idNode currIdNode, char * text) {
void vinegere_cipher (int arg, char * text, char *inputKey, char *return_value) {
  //char * key = encIdNodes[devIndex].key;
  char * key = inputKey;
  int textLen = strlen(text);
  int keyLen = strlen(key);
  int i, j;
  char newKey[textLen];
  char encryptedMsg[textLen];
  char decryptedMsg[textLen];
  //  char *return_value= NULL;
  
  if (arg == 1) {

    // Loop over original key and repeat characters until same length of text
    if (keyLen != textLen) {
      for (i = 0, j = 0; i < textLen; i++, j++) {
        if (j == keyLen)
          j = 0;
        newKey[i] = key[j];
      }
    }
    newKey[i] = '\0';
    /* strcpy(inputKey, newKey); // Update the key */
    /* strcpy(inputKey, newKey); // Update the message */
  
    // Encryption
    for (i = 0; i < textLen; i++) {
      encryptedMsg[i] = ((text[i] + newKey[i]) % 26) + 'A';
    }
    encryptedMsg[i] = '\0';
  
    
    memcpy(return_value, encryptedMsg, strlen(encryptedMsg)+1);
    //printk(KERN_INFO "return_value %s\n", return_value);
    return;
  } else {
    // Decryption
    for (i = 0; i < textLen; i++) {
      decryptedMsg[i] = (((encryptedMsg[i] - newKey[i]) + 26) % 26) + 'A';
    }
    decryptedMsg[i] = '\0';
    
    memcpy(return_value, encryptedMsg, strlen(decryptedMsg)+1);
    printk(KERN_INFO "return_value %s\n", return_value);
    return;
  }
  return;
}

static ssize_t encrypt_read (struct file * filep, char * buffer, size_t len, loff_t * offset) {
  int ret = 0;
  int i = 0;
  int index_size = 0;
  char index[4];
  char * token = NULL;
  const char *currChar;
  struct idNode newEncNode;
  /* token = strtok(virtualDevice.data, " "); */
  /* sprintf(index, "%s", token); */
  //printk(KERN_INFO "ENCRYPT READ!\n");
  for(currChar = virtualDevice.data; *currChar != '\0'; currChar+=1){  
    if (*currChar == '#' && flag == 0){
      index[i] = '\0';
      index_size = i - 1;
      flag = 1;
      break;
    }
    if (flag == 1)
      index[i] = *currChar;
    else
      token[i] = *currChar;
    i++;
  }
  
  for(i = 0; i < numEncDevices; i++){
    newEncNode = encIdNodes[i];
    if (strncmp(newEncNode.index, index, numChars) == 0){
      printk(KERN_INFO "device found!\n");
      break;
    }
  }
  
  // copy_to_user returns 0 on success
  // For encryption to be read back
  //printk(KERN_INFO "ENC REEAD: %s", newEncNode.data);
  ret = copy_to_user(buffer, newEncNode.data, strlen(newEncNode.data)+1);

  if (ret == 0) {
    // Success
    printk(KERN_INFO "cryptctl: sent %zu characters to the user\n", len);
    return (0);
  } else {
    printk(KERN_INFO "cryptctl: failed to send %d characters to the user\n", ret);
    return -EFAULT;
  }
  return 0;
}

static ssize_t decrypt_read (struct file * filep, char * buffer, size_t len, loff_t * offset) {
  int ret = 0;
  int i = 0;
  int index_size = 0;
  int flag = 0;
  char index[4];
  char * token = NULL;
  const char* currChar;
  struct idNode newDecNode;
  //token = strtok(virtualDevice.data, " ");
  //printk(KERN_INFO "DECRYPT READ!\n");
  for(currChar = virtualDevice.data; *currChar != '\0'; currChar+=1){  
    if (*currChar == '#' && flag == 0){
      index[i] = '\0';
      index_size = i - 1;
      flag = 1;
      break;
    }
    if (flag == 1)
      index[i] = *currChar;
    else
      token[i] = *currChar;
    i++;
  }
    
  for(i = 0; i < numEncDevices; i++){
    newDecNode = decIdNodes[i];
    if (strncmp(newDecNode.index, index, index_size) == 0){
      printk(KERN_INFO "device found!\n");
      break;
    }
  }
  printk(KERN_INFO "DEC REEAD: %s", newDecNode.data);
  ret = copy_to_user(buffer, newDecNode.data, strlen(newDecNode.data)+1);

  if (ret == 0) {
    // Success
    printk(KERN_INFO "cryptctl: sent %zu characters to the user\n", len);
    return (0);
  } else {
    printk(KERN_INFO "cryptctl: failed to send %d characters to the user\n", ret);
    return -EFAULT;
  }
  return 0;

}

static ssize_t encrypt_write (struct file * filep, const char * buffer, size_t len, loff_t * offset) {
  char index[4];
  char data[200];
  int i = 0;
  int index_size = 0;
  //int retval;
  //char * token;
  const char *currChar;
  //char encrypted_text[30];
  struct idNode newEncNode;
;
//printk(KERN_INFO "buffer: %s\n", buffer);
  
  for(currChar = buffer; *currChar != '\0'; currChar+=1){
    if (*currChar == '#' && flag == 0){
      index[i] = '\0';
      index_size = i - 1;
      flag = 1;
      i = 0;
      continue;
    }
    if (flag == 0){
      index[i] = *currChar;
    }
    else{
      data[i] = *currChar;
    }
    i++;
  }
  data[i] = '\0';
  /* printk(KERN_INFO "index: %s\n", index); */
  /* printk(KERN_INFO "data: %s\n", data); */
      
  for(i = 0; i < numEncDevices; i++){
    newEncNode = encIdNodes[i];
    if (strncmp(newEncNode.index, index, index_size) == 0){
      printk(KERN_INFO "device found!\n");
      break;
    }
  }

  vinegere_cipher(1, data, newEncNode.key, newEncNode.data);
  printk("message: %s\n", newEncNode.data);
  
  strcpy(encIdNodes[i].data, newEncNode.data);
  printk(KERN_INFO "cryptctl: received characters from the user\n");
  //newEncNode->data = encrypted_text;
  //retval = copy_to_user(encrypted_text, buffer, len);
  //retval = copy_to_user(buffer, encrypted_text, strlen(encrypted_text)+1);
  //return len;
  return 0;
}

static ssize_t decrypt_write (struct file * filep, const char * buffer, size_t len, loff_t * offset) {
  int i = 0;
  int index_size = 0;
  char index[4];
  //char * token;
  char data[200];
  const char* currChar;
  struct idNode newDecNode;
  /* token = strtok(virtualDevice.data, " "); */
  /* sprintf(index, "%s", token); */
  
    for(currChar = buffer; *currChar != '\0'; currChar+=1){
    if (*currChar == '#' && flag == 0){
      index[i] = '\0';
      index_size = i - 1;
      flag = 1;
      i = 0;
      continue;
    }
    if (flag == 0){
      index[i] = *currChar;
    }
    else{
      data[i] = *currChar;
    }
    i++;
  }
  data[i] = '\0';
  /* printk(KERN_INFO "index: %s\n", index); */
  /* printk(KERN_INFO "data: %s\n", data); */

  
  for(i = 0; i < numEncDevices; i++){
    newDecNode = decIdNodes[i];
    if (strncmp(newDecNode.index, index, index_size) == 0){
      printk(KERN_INFO "device found!\n");
      break;
    }
  }
  
  vinegere_cipher(1, data, newDecNode.key, newDecNode.data);
  printk("message: %s\n", newDecNode.data);
  
  strcpy(encIdNodes[i].data, newDecNode.data);
  printk(KERN_INFO "cryptctl: received characters from the user\n");

  return 0;
}

/**
 * The device open function that is called each time the device is opened
 * This will increment the numberOpens counter
 * @param inodep A poitner to an inode object
 * @param filep A pointer to a file object
*/
static int device_open (struct inode * inodep, struct file * filep) {
  /* if (down_interruptible(&virtualDevice.sem) != 0) { */
  /*   printk(KERN_ALERT "cryptctl: device is in use by another process\n"); */
  /*   return -EBUSY; */
  /* } else { */
    virtualDevice.numberOpens++;
    printk(KERN_INFO "cryptctl: device has been opened %d time(s)\n", virtualDevice.numberOpens);
    return 0;
    //  }
}

/* static int encrypt_open (struct inode * inodep, struct file * filep) { */
/*   printk(KERN_INFO "HERE\n"); */
/*   if (down_interruptible(&virtualDevice.sem) != 0) { */
/*     printk(KERN_ALERT "cryptctl: device is in use by another process\n"); */
/*     return -EBUSY; */
/*   } else { */
/*     virtualDevice.numberOpens++; */
/*     printk(KERN_INFO "cryptctl: device has been opened %d time(s)\n", virtualDevice.numberOpens); */
/*     return 0; */
/*   } */
/* } */

/**
 * This function is called whenever the device is being read from userpsace i.e.
 * data is being sent from the device to the user
 * Uses copy_to_user() function to send the buffer string to the user and captures
 * any errors
 * @param filep A pointer to a file object
 * @param buffer The pointer to the buffer to which this function writes the data
 * @param len The length of the buffer
 * @param offset The offset (if required)
*/
static ssize_t device_read (struct file * filep, char * buffer, size_t len, loff_t * offset) {
  int ret = 0;

  // copy_to_user returns 0 on success
  ret = copy_to_user(buffer, virtualDevice.data, virtualDevice.size_of_message);

  if (ret == 0) {
    // Success
    printk(KERN_INFO "cryptctl: sent %zu characters to the user\n", len);
    return (virtualDevice.size_of_message = 0);
  } else {
    printk(KERN_INFO "cryptctl: failed to send %d characters to the user\n", ret);
    return -EFAULT;
  }
}

/**
 * Function is called whenever the device is being written to from userspace i.e.
 * data is sent to the device from the user
 * The data is copied to the message[] array in this LKM
*/
static ssize_t device_write (struct file * filep, const char * buffer, size_t len, loff_t * offset) {
  sprintf(virtualDevice.data, "%s", buffer); // Appending received string with its length
  virtualDevice.size_of_message = strlen(virtualDevice.data); // Store the length of the stroed message
  printk(KERN_INFO "cryptctl: received characters from the user\n");
  return len;
}

/**
 * The device release function that is called whenever the device is closed/released by
 * the userspace program.
*/
static int device_release (struct inode * inodep, struct file * filep) {
  //  up(&virtualDevice.sem);
  printk(KERN_INFO "cryptctl: device successfully closed\n");
  return 0;
}

/* static int encrypt_release (struct inode * inodep, struct file * filep) { */
/*   up(&virtualDevice.sem); */
/*   printk(KERN_INFO "cryptctl: device successfully closed\n"); */
/*   return 0; */
/* } */

static long device_ioctl (struct file * file, unsigned int ioctl_num, unsigned long ioctl_param) {
  // ------------------------------------------ //
  /* dev_t devno = MKDEV(majNum, device_number); */

  char ch;
  char* temp;
  //char* key = NULL; //, indexchar;
  //int i, index;

  // ------------(mymodule18.c) seperate variables--------------------------- //
  /* dev_t devno = MKDEV(majNum, device_number); */
  dev_t devno = MKDEV(majNum, device_number);
  struct idNode newEncNode;
  struct idNode newDecNode;

  // vars for case destroy device
  char index[4];
  //char data[200];
  char key[30];
  int i = 0;
  int index_size = 0;
  int destroyIndex = -1;
  const char *currChar;// = index;
  int numChars = 0;
  int flag = 0;
  // currNodeIndEnc is last element in array
  int endIndex = currNodeIndEnc;
  // --------------------------------------- //
  
  switch (ioctl_num) {
  case IOCTL_CREATE_DEVICE:
      printk(KERN_INFO "Enter IOCTL_GET_CREATE\n");
      
      // begin create encrypt device
      sprintf(temp_encrypt_device, "cryptEncrypt%d", device_number);
      printk(KERN_INFO "encrypt device: %s\n", temp_decrypt_device);
    
    // only go in if stmt if it's the first time
    if(boolAllocEncrypt == 0){
      if(alloc_chrdev_region(&devno, 0, 10, "encrypt18") < 0){
	printk(KERN_ALERT "COULD NOT ALLOCATE MAJOR NUMBER FOR %s\n", temp_encrypt_device);
	return -1;
      }
      currMajNumEnc = MAJOR(devno);
      currMinNumEnc = MINOR(devno);
      firstDevEnc = devno;
      
      // initialize charDevice for encrypt
      cdev_init(&encrypt_cdev, &enc_fops);
      charDevEncrypt = &encrypt_cdev;
      encrypt_cdev.owner = THIS_MODULE;

      // add charDevice to kernel
      if(cdev_add(charDevEncrypt, devno, 10) == -1){
	device_destroy(crypt_class_encrypt, devno);
	unregister_chrdev_region(devno, numEncDevices);
	printk(KERN_WARNING "Error in attempt to add %s", temp_encrypt_device);
	return -1;
      }

      boolAllocEncrypt = 1;
    }

    devno = MKDEV(currMajNumEnc, device_number);
    new_encrypt_device = device_create(crypt_class_encrypt, NULL, devno, NULL, temp_encrypt_device);

    
    if (IS_ERR(new_encrypt_device)){
      unregister_chrdev_region(devno, numEncDevices);
      printk(KERN_WARNING "Error creating encrypt device: %s", temp_encrypt_device);
      return PTR_ERR(new_encrypt_device);
    }
    else{
      printk(KERN_INFO "encrypt device created\n");
      numEncDevices += 1;
    }

    // setup idNode struct
    newEncNode.cls = crypt_class_encrypt;
    newEncNode.dev = devno;
    strcpy(newEncNode.name, temp_encrypt_device);
    strcpy(newEncNode.key, virtualDevice.data);
    sprintf(newEncNode.index, "%d", device_number);
    
    currNodeIndEnc += 1;
    encIdNodes[currNodeIndEnc] = newEncNode;
    // end create encrypt device

    

    // begin create decrypt device
    sprintf(temp_decrypt_device, "cryptDecrypt%d", device_number);
    printk(KERN_INFO "decrypt device: %s\n", temp_decrypt_device);

    // only go in if stmt if it's the first time
    if(boolAllocDecrypt == 0){
      if(alloc_chrdev_region(&devno, 0, 10, "decrypt18") < 0){
	printk(KERN_ALERT "COULD NOT ALLOCATE MAJOR NUMBER FOR %s\n", temp_decrypt_device);
	return -1;  
      }
      currMajNumDec = MAJOR(devno);
      currMinNumDec = MINOR(devno);
      firstDevDec = devno;
      // initialize charDevice for decrypt
      cdev_init(&decrypt_cdev, &dec_fops);
      charDevDecrypt = &decrypt_cdev;
      decrypt_cdev.owner = THIS_MODULE;

      if(cdev_add(charDevDecrypt, devno, 10) == -1){
	device_destroy(crypt_class_decrypt, devno);
	unregister_chrdev_region(devno, numDecDevices);
	printk(KERN_WARNING "Error in attempt to add %s", temp_decrypt_device);
	return -1;
      }

      boolAllocDecrypt = 1;
    }

    devno = MKDEV(currMajNumDec, device_number);
    new_decrypt_device = device_create(crypt_class_decrypt, NULL, devno, NULL, temp_decrypt_device);
    
    if (IS_ERR(new_decrypt_device)){
      unregister_chrdev_region(devno, numDecDevices);
      printk(KERN_WARNING "Error creating decrypt device: %s", temp_decrypt_device);
      return PTR_ERR(new_decrypt_device);
    }
    else{
      printk(KERN_INFO "decrypt device created\n");
      numDecDevices += 1;
    }

    newDecNode.cls = crypt_class_decrypt;
    newDecNode.dev = devno;
    strcpy(newDecNode.name, temp_decrypt_device);
    strcpy(newDecNode.key, virtualDevice.data);
    sprintf(newDecNode.index, "%d", device_number);
    
    currNodeIndDec += 1;
    decIdNodes[currNodeIndDec] = newDecNode;        
    // end create decrypt device
    
    device_number++;
    //printk(KERN_INFO "device_number is: %d, flag is %d\n", device_number, flag);
    return (device_number - 1);
      
/*
###############################################################
*/

  case IOCTL_DESTROY_DEVICE:
      for(currChar = virtualDevice.data; *currChar != '\0'; currChar+=1){
      numChars += 1;
    }

    strncpy(index, virtualDevice.data, numChars+1);
    i = 0;

    if(endIndex >= 0){
      while(i <= endIndex){
	currNode = encIdNodes[i];

	printk(KERN_INFO "inside while\n");
	printk(KERN_INFO "maj: %d\nmin: %d\nname: %s\nkey: %s\nindex: %s\n", MAJOR(currNode.dev), MINOR(currNode.dev), currNode.name, currNode.key, currNode.index);

	printk(KERN_INFO "maj: %d\nmin: %d\nname: %s\nkey: %s\nindex: %s\n", MAJOR(decIdNodes[i].dev), MINOR(decIdNodes[i].dev), decIdNodes[i].name, decIdNodes[i].key, decIdNodes[i].index);
	
	if(strncmp(index, currNode.index, numChars) == 0){
	  destroyIndex = i;
	}
	i++;
      }
    }

    
    if(destroyIndex == -1){
      printk(KERN_INFO "Device to destroy not found");
      return -1;
    }

    device_destroy(crypt_class_encrypt, encIdNodes[destroyIndex].dev);
    device_destroy(crypt_class_decrypt, decIdNodes[destroyIndex].dev);

    if(destroyIndex != endIndex){
      tempEnc0 = &encIdNodes[endIndex];
      encIdNodes[destroyIndex] = *tempEnc0;
      
      tempDec0 = &decIdNodes[endIndex];
      decIdNodes[destroyIndex] = *tempDec0;
    }

    
    //destroyEncNode = encIdNodes[destroyIndex];
    //destroyDecNode = decIdNodes[destroyIndex];

    
    // destroy devices
    printk(KERN_INFO "Destroying\n");
    printk(KERN_INFO "maj: %d\nmin: %d\nname: %s\nkey: %s\nindex: %s\n", MAJOR(encIdNodes[destroyIndex].dev), MINOR(encIdNodes[destroyIndex].dev), encIdNodes[destroyIndex].name, encIdNodes[destroyIndex].key, encIdNodes[destroyIndex].index);

    printk(KERN_INFO "maj: %d\nmin: %d\nname: %s\nkey: %s\nindex: %s\n", MAJOR(decIdNodes[destroyIndex].dev), MINOR(decIdNodes[destroyIndex].dev), decIdNodes[destroyIndex].name, decIdNodes[destroyIndex].key, decIdNodes[destroyIndex].index);
    
    currNodeIndEnc -= 1;
    currNodeIndDec -= 1;
    

    return 0;
    

/*
###############################################################
*/

    case IOCTL_CHANGE_KEY:
      printk(KERN_INFO "IOCTL_CHANGE_KEY\n");
      numChars = -1; // for incrementing
      printk(KERN_INFO "VD.data: %s\n", virtualDevice.data);
      
      for(currChar = virtualDevice.data; *currChar != '\0'; currChar+=1){
	if (*currChar == '#' && flag == 0){
	  index[i] = '\0';
	  index_size = i - 1;
	  flag = 1;
	  i = 0;
	  continue;
	}
	if (flag == 0){
	  index[i] = *currChar;
	}
	else{
	  key[i] = *currChar; 
	}
	i++;
      }
      key[i] = '\0';
      
      for(i = 0; i < numEncDevices; i++){
	newEncNode = encIdNodes[i];
	newDecNode = decIdNodes[i];
	if (strncmp(newEncNode.index, index, index_size) == 0){
	  printk(KERN_INFO "device found!\n");
	  break;
	}
      }

      strcpy(newEncNode.key, key);
      strcpy(newDecNode.key, key);
      printk(KERN_INFO "key changed successfully for encrypt: %s decrypt: %s\n", newEncNode.key,  newEncNode.key);
      return 0;

/*
###############################################################
*/
    case IOCTL_ENCRYPT:
      printk(KERN_INFO "IOCTL_ENCRYPT\n");
      /* for(currChar = virtualDevice.data; *currChar != '\0'; currChar+=1){ */
      /* 	if (*currChar == '#' && flag == 0){ */
      /* 	  index[i] = '\0'; */
      /* 	  index_size = i - 1; */
      /* 	  flag = 1; */
      /* 	  i = 0; */
      /* 	  continue; */
      /* 	} */
      /* 	if (flag == 0){ */
      /* 	  index[i] = *currChar; */
      /* 	} */
      /* 	else{ */
      /* 	  data[i] = *currChar;  */
      /* 	} */
      /* 	i++; */
      /* } */
      /* data[i] = '\0'; */

      printk(KERN_INFO "virtualDevice.data: %s\n", virtualDevice.data);
      encrypt_write(file, virtualDevice.data, i, 0);
      //i = encrypt_read(file, data, 99, 0);


      /* temp = (char *)ioctl_param; */
      /* get_user(ch, temp); */
      /* for (i = 0; ch && i < 1024; i++, temp++) */
      /*   get_user(ch, temp); */
      /* printk(KERN_INFO "ioctl_param: %s\n", (char*)ioctl_param); */
      /* encrypt_write(file, (char *)ioctl_param, i, 0); */
      /* i = encrypt_read(file, (char *)ioctl_param, 99, 0); */
      /* put_user('\0', (char *)ioctl_param+i); */
      return 0;

/*
###############################################################
*/
    case IOCTL_DECRYPT:
      // needs params for both read and write
      temp = (char *)ioctl_param;
      get_user(ch, temp);
      for (i = 0; ch && i < 1024; i++, temp++)
        get_user(ch, temp);
      decrypt_write(file, (char *)ioctl_param, i, 0);
      i = decrypt_read(file, (char *)ioctl_param, 99, 0);
      put_user('\0', (char *)ioctl_param+i);
      break;
    default:
      return -EINVAL;
  }

  return 0;
}

static int driverInit(void){

  retval = alloc_chrdev_region(&devNum, 0, 1, DEVICE_NAME);
  if(retval < 0){
    printk(KERN_ALERT "COULD NOT ALLOCATE MAJOR NUMBER\n");
    return -1;
  }
  
  mcdev = cdev_alloc();
  mcdev->ops = &fops;
  mcdev->owner = THIS_MODULE;
  
  majNum = MAJOR(devNum);

  if(cdev_add(mcdev, devNum, 1) < 0){
    printk(KERN_ALERT "UNABLE TO ADD CDEV TO KERNEL");
  }
  printk(KERN_INFO "Major number is %d\n", majNum);
  printk(KERN_INFO "\tuse \"sudo mknod -m 666 /dev/%s c %d 0\" for device file\n", DEVICE_NAME, majNum);
    
  // initialize both classes for encrypt/decrypt
  crypt_class_encrypt = class_create(THIS_MODULE, ENCRYPT_CLASS);
  crypt_class_decrypt = class_create(THIS_MODULE, DECRYPT_CLASS);
  
  if(crypt_class_encrypt == NULL || crypt_class_decrypt == NULL ){
    printk(KERN_ALERT "COULD NOT CREATE CLASS cryptclass\n");
    return -1;
  }
  else{
    boolCryptCreated = 1;
  }

  crypt_class_encrypt->dev_uevent = driver_uevent;
  crypt_class_decrypt->dev_uevent = driver_uevent;
  
    
  printk(KERN_INFO "Device Created %s", DEVICE_NAME);
  printk(KERN_INFO "");
  
  sema_init(&virtualDevice.sem, 1);
  return 0;

}

/*
###############################################################
*/


static void driverExit(void){
  printk(KERN_INFO "Good bye");
  printk(KERN_INFO "");

  // destroy en/decrypt devices
  if(boolCryptCreated == 1){
    int i;
    struct idNode currNode;
    dev_t dev;
    
    // unregister module
    cdev_del(mcdev);
    unregister_chrdev_region(devNum, 1);



    
    if(numEncDevices > 0){
      dev = encIdNodes[0].dev;
      printk(KERN_INFO "numEncDevices = %d", numEncDevices);

      for(i = 0; i < numEncDevices; i++){
	currNode = encIdNodes[i];
	printk(KERN_INFO "maj: %d\nmin: %d\nname: %s\nkey: %s\nindex: %s\n", MAJOR(currNode.dev), MINOR(currNode.dev), currNode.name, currNode.key, currNode.index);

	device_destroy(crypt_class_encrypt, currNode.dev);
      }
      //cdev_del(charDevEncrypt);
    }

    i = 0;
    
    if(numDecDevices > 0){
      printk(KERN_INFO "numDecDevices = %d", numDecDevices);
      dev = decIdNodes[0].dev;

      for(i = 0; i < numDecDevices; i++){
	currNode = decIdNodes[i];
	printk(KERN_INFO "maj: %d\nmin: %d\nname: %s\nkey: %s\nindex: %s\n", MAJOR(currNode.dev), MINOR(currNode.dev), currNode.name, currNode.key, currNode.index);
	device_destroy(crypt_class_decrypt, currNode.dev);
      }

      //cdev_del(charDevDecrypt);
    }

    if(boolAllocEncrypt){
      cdev_del(charDevEncrypt);
      unregister_chrdev_region(firstDevEnc, numEncDevices);
    }

    if(boolAllocDecrypt){
      cdev_del(charDevDecrypt);
      unregister_chrdev_region(firstDevDec, numDecDevices);
    }
    
    if(crypt_class_encrypt != NULL)
      class_destroy(crypt_class_encrypt);

    if(crypt_class_decrypt != NULL)
      class_destroy(crypt_class_decrypt);

  }
  
  printk(KERN_ALERT "Unloaded module\n");
  printk(KERN_INFO "");
}


module_init(driverInit);
module_exit(driverExit);



/* static void __exit cryptctl_exit (void) { */
/*   device_destroy(cryptctlClass, MKDEV(majorNumber, 0)); // Remove the device */
/*   class_unregister(cryptctlClass); // Unregister the device class */
/*   class_destroy(cryptctlClass); // Remove the device class */
/*   unregister_chrdev(majorNumber, DEVICE_NAME); // Unregister the major number */
/*   printk(KERN_INFO "cryptctl: goodbye\n"); */
/* } */

/* /\** */
/*  * A module must use the module_init() and module_exit() macros, which identify */
/*  * the initialization function at insertion time and the cleanup function */
/* *\/ */
/* module_init(cryptctl_init); */
/* module_exit(cryptctl_exit); */
