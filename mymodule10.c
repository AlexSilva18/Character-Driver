#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include "mymodule.h"
#include <linux/string.h>

#define ENCRYPT_CLASS "EncryptClass10"
#define DECRYPT_CLASS "DecryptClass10"

#define CLASS_NAME_MODULE "modClass"

struct newDevice {
  char data[100];
  struct semaphore sem;
} virtualDevice;

//char device[15];
//static struct crypt_dev *crypt_devices = NULL;

//structs for creating our module
struct cdev *mcdev;
struct class *cl;


// class for creating encrypt/decrypt
static struct class *crypt_class_encrypt;
static struct class *crypt_class_decrypt;
static struct device* new_encrypt_device = NULL;
static struct device* new_decrypt_device = NULL;

//int majNum; // defined in the header file


int retval;
int flag = 0;
int boolCryptCreated = 0;
int device_number = 0;
int currNodeIndEnc = -1;
int currNodeIndDec = -1;

MODULE_LICENSE("GPL");

/* dev_t devNum; */ // defined in the header file

/* #define DEVICE_NAME   "testDevice" */ // defined in the header file


// struct to store what we need to remove a device
struct idNode{
  struct class *cls;
  dev_t dev;
  //struct cdev cdev;
  char name[100];
};


// arrays that will hold the idNode structs
struct idNode encIdNodes[100];
struct idNode decIdNodes[100];

// boolean vars to check if we allocated space for the drivers
int boolAllocEncrypt = 0;
int boolAllocDecrypt = 0;


int device_open(struct inode *inode, struct file *filp){
  if(down_interruptible(&virtualDevice.sem) != 0){
    printk(KERN_ALERT "COULD NOT LOCK DEVICE DURING OPEN\n");
    return -1;
  }

  printk(KERN_INFO "Opened Device\n");
  return 0;
}

ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset){
 
  printk(KERN_INFO "Reading from device\n");
  retval = copy_to_user(bufStoreData, virtualDevice.data, bufCount);
 return retval;  
}

ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset){

  printk(KERN_INFO "Writing to device\n");
  retval = copy_from_user(virtualDevice.data, bufSourceData, bufCount);
  return retval;
}


int device_close(struct inode *inode, struct file *filp){
  up(&virtualDevice.sem);
  printk(KERN_INFO "Closed Device\n");
  return 0;
}

/* struct file_operations cryptops = { */
/*   .encrypt = device_encrypt, */
/*   .decrypt = device_decrypt */
/* }; */




// arrays to store the names of the en/decrypt device files
char temp_encrypt_device[14];
char temp_decrypt_device[14];

int currMajNumEnc = 0;
int currMinNumEnc = 0;
int currMajNumDec = 0;
int currMinNumDec = 0;

int numEncDevices = 0; //number of encrypt devices we've created
int numDecDevices = 0;

struct cdev *charDevEncrypt;
struct cdev *charDevDecrypt;

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
  //const char* buffer;
  //int i;
  struct cdev encrypt_cdev, decrypt_cdev;
  struct file_operations fopsEncrypt;
  struct file_operations fopsDecrypt;
  dev_t devno = MKDEV(majNum, device_number);
  struct idNode newEncNode;
  struct idNode newDecNode;
  
  switch(cmd){
    
  case IOCTL_CREATE_DEVICE:
    printk(KERN_INFO "Enter IOCTL_GET_CREATE\n");

    // begin create encrypt device
    sprintf(temp_encrypt_device, "cryptEncrypt%d", device_number);

    if(boolAllocEncrypt == 0){
      if(alloc_chrdev_region(&devno, 0, 10, "encrypt10") < 0){
	printk(KERN_ALERT "COULD NOT ALLOCATE MAJOR NUMBER FOR %s\n", temp_encrypt_device);
	return -1;
      }
      currMajNumEnc = MAJOR(devno);
      currMinNumEnc = MINOR(devno);
      

      // initialize charDevice for encrypt
      cdev_init(&encrypt_cdev, &fopsEncrypt);
      charDevEncrypt = &encrypt_cdev;
      encrypt_cdev.owner = THIS_MODULE;

      // add charDevice to kernel
      if(cdev_add(&encrypt_cdev, devno, 10) == -1){
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
      printk(KERN_WARNING "Error creating encrypt device");
      return PTR_ERR(new_encrypt_device);
    }
    else{
      printk(KERN_INFO "encrypt device created\n");
      numEncDevices += 1;
    }




    newEncNode.cls = crypt_class_encrypt;
    newEncNode.dev = devno;
    //newEncNode.cdev = encrypt_cdev;
    strcpy(newEncNode.name, temp_encrypt_device);

    currNodeIndEnc += 1;
    encIdNodes[currNodeIndEnc] = newEncNode;

    // end create encrypt device

    
    /*

    // begin create decrypt device
    sprintf(temp_decrypt_device, "cryptDecrypt%d", device_number);
    printk(KERN_INFO "decrypt device: %s\n", temp_decrypt_device);

    if(boolAllocDecrypt == 0){
      if(alloc_chrdev_region(&devno, 0, 10, "decrypt10") < 0){
	printk(KERN_ALERT "COULD NOT ALLOCATE MAJOR NUMBER FOR %s\n", temp_decrypt_device);
	return -1;  
      }
      currMajNumDec = MAJOR(devno);
      currMinNumDec = MINOR(devno);

      // initialize charDevice for decrypt
      cdev_init(&decrypt_cdev, &fopsDecrypt);
      decrypt_cdev.owner = THIS_MODULE;

      if(cdev_add(&decrypt_cdev, devno, 10) == -1){
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
      printk(KERN_WARNING "Error creating decrypt device");
      return PTR_ERR(new_decrypt_device);
    }
    else{
      printk(KERN_INFO "decrypt device created\n");
      numDecDevices += 1;
    }

    
    
    newDecNode.cls = crypt_class_decrypt;
    newDecNode.dev = devno;
    newDecNode.cdev = &decrypt_cdev;
    newDecNode.name = temp_decrypt_device;


    currNodeIndDec += 1;
    decIdNodes[currNodeIndDec] = newDecNode;
        
    // end create decrypt device
    */

    
    device_number++;
    //printk(KERN_INFO "device_number is: %d, flag is %d\n", device_number, flag);
    return (device_number - 1);
      
  case IOCTL_DESTROY_DEVICE:

    
    cdev_del(charDevEncrypt);
    cdev_del(charDevDecrypt);
    device_destroy(crypt_class_encrypt, encIdNodes[currNodeIndEnc].dev);
    device_destroy(crypt_class_decrypt, decIdNodes[currNodeIndDec].dev);
    currNodeIndEnc -= 1;
    currNodeIndDec -= 1;
    printk(KERN_INFO "device being destroyed: %s\n", virtualDevice.data);
    unregister_chrdev(devno, virtualDevice.data);
    return 0;
    
    
    /*
    device_destroy(crypt_class, devno);
    cdev_del(&encrypt_cdev);
    cdev_del(&decrypt_cdev);
    printk(KERN_INFO "device being destroyed: %s\n", virtualDevice.data);
    unregister_chrdev(devno, virtualDevice.data);
    return 0;
    */
    
  default:
    return -EINVAL;

  }
  
  return 0;
}


struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = device_open,
  .release = device_close,
  .write = device_write,
  .read = device_read,
  .unlocked_ioctl = device_ioctl
};




static int driverInit(void){

  retval = alloc_chrdev_region(&devNum, 0, 1, DEVICE_NAME);
  if(retval < 0){
    printk(KERN_ALERT "COULD NOT ALLOCATE MAJOR NUMBER\n");
    return -1;
  }
  
  mcdev = cdev_alloc();
  mcdev->ops = &fops;
  mcdev->owner = THIS_MODULE;

  //cdev_init(mcdev, &fops);
  
  majNum = MAJOR(devNum);

  if(cdev_add(mcdev, devNum, 1) < 0){
    printk(KERN_ALERT "UNABLE TO ADD CDEV TO KERNEL");
  }
  printk(KERN_INFO "Major number is %d\n", majNum);
  printk(KERN_INFO "\tuse \"sudo mknod -m 666 /dev/%s c %d 0\" for device file\n", DEVICE_NAME, majNum);
    
  crypt_class_encrypt = class_create(THIS_MODULE, ENCRYPT_CLASS);
  crypt_class_decrypt = class_create(THIS_MODULE, DECRYPT_CLASS);
  if(crypt_class_encrypt == NULL || crypt_class_decrypt == NULL ){
    //unregister_chrdev_region(devNum, 1);
    printk(KERN_ALERT "COULD NOT CREATE CLASS cryptclass\n");
    return -1;
  }
  else
    boolCryptCreated = 1;

  
  /*
  cl = class_create(THIS_MODULE, CLASS_NAME_MODULE);
  if(cl == NULL){
    unregister_chrdev_region(devNum, 1);
    printk(KERN_ALERT "COULD NOT CREATE CLASS\n");
    return -1;
  }

  if(device_create(cl, NULL, devNum, NULL, DEVICE_NAME) == NULL){
    class_destroy(cl);
    unregister_chrdev_region(devNum, 1);
    printk(KERN_ALERT "COULD NOT CREATE DEVICE\n");
    return -1;
  }

  if(cdev_add(&mcdev, devNum, 1) == -1){
    device_destroy(cl, devNum);
    class_destroy(cl);
    unregister_chrdev_region(devNum, 1);
    printk(KERN_ALERT "COULD NOT ADD DEVICE\n");
    return -1;
  }
  */

  

  /*
  if(cdev_add(mcdev, devNum, 1) < 0){
    printk(KERN_ALERT "COULD NOT ADD MODULE");
    class_destroy(crypt_class);
    unregister_chrdev_region(devNum, 1);
    return -1;
  }
  */
  
  printk(KERN_INFO "Device Created %s", DEVICE_NAME);
  printk(KERN_INFO "");
  
  sema_init(&virtualDevice.sem, 1);
  return 0;

}




static void driverExit(void){
  printk(KERN_INFO "Good bye");
  printk(KERN_INFO "");


  // destroy en/decrypt devices
  if(boolCryptCreated == 1){
    int i;
    struct idNode currNode;
    //struct cdev *charDev;
    dev_t dev;
    // unregister module
    cdev_del(mcdev);
    unregister_chrdev_region(devNum, 1);

    

    if(numEncDevices > 0){
      dev = encIdNodes[0].dev;
      printk(KERN_INFO "numEncDevices = %d", numEncDevices);
      //charDev = &encIdNodes[0].cdev;

      for(i = 0; i < numEncDevices; i++){
	currNode = encIdNodes[i];
	printk(KERN_INFO "maj: %d\nmin: %d\nname: %s\n", MAJOR(currNode.dev), MINOR(currNode.dev), currNode.name);
	device_destroy(crypt_class_encrypt, currNode.dev);
      }
    
      cdev_del(charDevEncrypt);
    }

    i = 0;
    
    if(numDecDevices > 0){
      printk(KERN_INFO "numDecDevices = %d", numDecDevices);
      dev = decIdNodes[0].dev;
      //charDev = &decIdNodes[0].cdev;

      for(i = 0; i < numDecDevices; i++){
	currNode = decIdNodes[i];
	printk(KERN_INFO "maj: %d\nmin: %d\nname: %s\n", MAJOR(currNode.dev), MINOR(currNode.dev), currNode.name);
	device_destroy(crypt_class_decrypt, currNode.dev);
      }

      cdev_del(charDevDecrypt);
      
    }
  

  
    if(crypt_class_encrypt != NULL)
      class_destroy(crypt_class_encrypt);

    if(crypt_class_decrypt != NULL)
      class_destroy(crypt_class_decrypt);

  }
  
  /*
  if(crypt_class_encrypt != NULL)
    class_destroy(crypt_class_encrypt);

  if(crypt_class_decrypt != NULL)
    class_destroy(crypt_class_decrypt);
  */

  printk(KERN_ALERT "Unloaded module\n");
  printk(KERN_INFO "");
}


module_init(driverInit);
module_exit(driverExit);
