#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init hello_start(void){
  printk(KERN_INFO "Loading hello module...\n");
  return 0;
}

static void __exit hello_end(void){
  printk(KERN_INFO "Goodbye Mr.\n");
}

module_init(hello_start);
module_exit(hello_end);


/* int init_module(void){ */
/*   printk(KERN_INFO "init_module() called\n"); */
/*   return 0; */
/* } */

/* void cleanup_module(void){ */
/*   printk(KERN_INFO "cleanup_module() called\n"); */
/* } */

