/*
Mark Vetro
Nicoholas Ho Lung
Jesse Lopez

Operating Systems Assignment 2
*/




//necessary linux libraries for operation
#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>
#include <asm/uaccess.h>          // Required for the copy to user function
#include <linux/mutex.h>	         /// Required for the mutex functionality

#define  DEVICE_NAME "ass3W"    ///< The device will appear at /dev/ass3Write using this value
#define  CLASS_NAME  "ass3W"        ///< The device class -- this is a character device driver


MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Mark Vetro, Nicholas Ho Lung, Jesse Lopez");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("Our great operating systems homework assignment");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[1024] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ass3WriteClass  = NULL; ///< The device-driver class struct pointer
static struct device* ass3WriteDevice = NULL; ///< The device-driver device struct pointer
static DEFINE_MUTEX(ass2_mutex);  /// A macro that is used to declare a new mutex that is visible in this file
                                     /// results in a semaphore variable ebbchar_mutex with value 1 (unlocked)
                                     /// DEFINE_MUTEX_LOCKED() results in a variable with value 0 (locked)

EXPORT_SYMBOL(message);





// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);



/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
   .open = dev_open,
   .write = dev_write,
   .release = dev_release,
};


/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init ass3Write_init(void){
   printk(KERN_INFO "ass3Write: Initializing the ass3Write LKM\n");

   mutex_init(&ass2_mutex);

   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "ass3Write failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "ass3Write: registered correctly with major number %d\n", majorNumber);

   // Register the device class
   ass3WriteClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(ass3WriteClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ass3WriteClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "ass3Write: device class registered correctly\n");

   // Register the device driver
   ass3WriteDevice = device_create(ass3WriteClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(ass3WriteDevice)){               // Clean up if there is an error
      class_destroy(ass3WriteClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ass3WriteDevice);
   }
   printk(KERN_INFO "ass3Write: device class created correctly\n"); // Made it! device was initialized
   return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit ass3Write_exit(void){
   mutex_destroy(&ass2_mutex);
   device_destroy(ass3WriteClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(ass3WriteClass);                          // unregister the device class
   class_destroy(ass3WriteClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "ass3Write: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep){

	if(!mutex_trylock(&ass2_mutex))
	   {    /// Try to acquire the mutex (i.e., put the lock on/down)
		                                  /// returns 1 if successful and 0 if there is contention
	      printk(KERN_ALERT "EBBChar: Device in use by another process");
	      return -EBUSY;
	   }
	else{
	   numberOpens++;
	   printk(KERN_INFO "ass3Write: Device has been opened %d time(s)\n", numberOpens);
	   return 0;
	}
   
}

/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){

   if(size_of_message>1024)
   {
       printk(KERN_INFO "buffer is full\n");
       return 0;
   }

   else if(len + size_of_message > 1024)
   {
       snprintf(message, 1024-size_of_message, "%s", buffer);
       printk(KERN_INFO "received %d characters from the user\n", 1024-size_of_message);
       return (1024-size_of_message);
   }

   else
    {
        sprintf(message, "%s", buffer);   // appending received string with its length
        size_of_message = strlen(message); // store the length of the stored message
        return len;
    }
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep){
   mutex_unlock(&ass2_mutex); 
   printk(KERN_INFO "ass3Write: Device successfully closed\n");
   return 0;
}


/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(ass3Write_init);
module_exit(ass3Write_exit);
