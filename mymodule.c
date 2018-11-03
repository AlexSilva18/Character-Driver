#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>

struct newDevice {
  char data[100];
  struct semaphore sem;
} virtualDevice;

struct cdev *mcdev;
struct class *cl;
int majNum;
int retval;
MODULE_LICENSE("GPL");

dev_t devNum;

#define DEVICE_NAME   "testDevice"


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

struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = device_open,
  .release = device_close,
  .write = device_write,
  .read = device_read

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

  retval = cdev_add(mcdev, devNum, 1);
  if(retval < 0){
    printk(KERN_ALERT "UNABLE TO ADD CDEV TO KERNEL\n");
    return retval;
  }
  
  
  sema_init(&virtualDevice.sem, 1);
  return 0;
}




static void driverExit(void){
  cdev_del(mcdev);

  unregister_chrdev_region(devNum, 1);
  printk(KERN_ALERT "Unloaded module\n");
}


module_init(driverInit);
module_exit(driverExit);
