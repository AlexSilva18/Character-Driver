#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include "mymodule.h"

#define CLASS_NAME "newDEVICES5"

struct newDevice {
  char data[100];
  struct semaphore sem;
} virtualDevice;
//char device[15];
//static struct crypt_dev *crypt_devices = NULL;
struct class *crypt_class;
struct cdev *mcdev;
struct class *cl;
static struct device* new_encrypt_device = NULL;
static struct device* new_decrypt_device = NULL;
//int majNum; // defined in the header file
int retval;
int flag = 0;
MODULE_LICENSE("GPL");
int device_number = 7;

/* dev_t devNum; */ // defined in the header file

/* #define DEVICE_NAME   "testDevice" */ // defined in the header file


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

char temp_encrypt_device[14];
char temp_decrypt_device[14];
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
  //const char* buffer;
  //int i;
  struct cdev encrypt_cdev, decrypt_cdev;
  struct file_operations foops;
  dev_t devno = MKDEV(majNum, device_number);
  switch(cmd){
    
  case IOCTL_CREATE_DEVICE:
    printk(KERN_INFO "Enter IOCTL_GET_CREATE\n");

    sprintf(temp_encrypt_device, "cryptEncrypt%d", device_number);
    alloc_chrdev_region(&devno, 0, 1, temp_encrypt_device);
    // create a device driver
    cdev_init(&encrypt_cdev, &foops);
    new_encrypt_device = device_create(crypt_class, NULL, devno, NULL, temp_encrypt_device);
    if (IS_ERR(new_encrypt_device)){
      unregister_chrdev_region(devno, 1);
      printk(KERN_WARNING "Error creating device");
      return PTR_ERR(new_encrypt_device);
    }
    else
      printk(KERN_INFO "device created\n");
    if (cdev_add(&encrypt_cdev, devno, 1) == -1){
      device_destroy(crypt_class, devno);
      unregister_chrdev_region(devno, 1);
      printk(KERN_WARNING "Error in attempt to add %s%d", DEVICE_NAME, cmd);
    }

    // create decrypt device
    sprintf(temp_decrypt_device, "cryptDecrypt%d", device_number);
    printk(KERN_INFO "decrypt device: %s\n", temp_decrypt_device);
    alloc_chrdev_region(&devno+1, 0, 1, temp_decrypt_device);
    cdev_init(&decrypt_cdev, &foops);
    new_decrypt_device = device_create(crypt_class, NULL, devno+1, NULL, temp_decrypt_device);
    if (IS_ERR(new_decrypt_device)){
      unregister_chrdev_region(devno+1, 1);
      printk(KERN_WARNING "Error creating device");
      return PTR_ERR(new_decrypt_device);
    }
    else
      printk(KERN_INFO "device created\n");
    if (cdev_add(&decrypt_cdev, devno+1, 1) == -1){
      device_destroy(crypt_class, devno+1);
      unregister_chrdev_region(devno+1, 1);
      printk(KERN_WARNING "Error in attempt to add %s%d", DEVICE_NAME, cmd);
    }

    device_number++;
    //printk(KERN_INFO "device_number is: %d, flag is %d\n", device_number, flag);
    return (100 + device_number - 1);
      
  case IOCTL_DESTROY_DEVICE:
    device_destroy(crypt_class, devno);
    cdev_del(&encrypt_cdev);
    cdev_del(&decrypt_cdev);
    printk(KERN_INFO "device being destroyed: %s\n", virtualDevice.data);
    unregister_chrdev(devno, virtualDevice.data);
    return 0;

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
    return retval;
  }


  majNum = MAJOR(devNum);
  printk(KERN_INFO "Major Number is %d\n", majNum);
  printk(KERN_INFO "\tuse \"mknod -m 666 /dev/%s c %d 0\" for device file\n", DEVICE_NAME, majNum);

  mcdev = cdev_alloc();
  mcdev->ops = &fops;
  mcdev->owner = THIS_MODULE;

  crypt_class = class_create(THIS_MODULE, CLASS_NAME);
  if (crypt_class == NULL){
    unregister_chrdev_region(devNum, 1);
    printk(KERN_ALERT "UNABLE TO CREATE CLASSn");
  }
  retval = cdev_add(mcdev, devNum, 1);
  if(retval < 0){
    printk(KERN_ALERT "UNABLE TO ADD CDEV TO KERNEL\n");
    class_destroy(crypt_class);
    unregister_chrdev_region(devNum, 1);
    return retval;
  }
  
  
  sema_init(&virtualDevice.sem, 1);
  return 0;
}




static void driverExit(void){
  cdev_del(mcdev);
  
  unregister_chrdev_region(devNum, 1);
  class_destroy(crypt_class);  
  printk(KERN_ALERT "Unloaded module\n");
}


module_init(driverInit);
module_exit(driverExit);