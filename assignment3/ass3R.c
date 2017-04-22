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

#define  DEVICE_NAME "ass3R"    ///< The device will appear at /dev/ass3Read using this value
#define  CLASS_NAME  "ass3R"        ///< The device class -- this is a character device driver


MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Mark Vetro, Nicholas Ho Lung, Jesse Lopez");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("Our great operating systems homework assignment");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ass3ReadClass  = NULL; ///< The device-driver class struct pointer
static struct device* ass3ReadDevice = NULL; ///< The device-driver device struct pointer
static DEFINE_MUTEX(ass2_mutex);  /// A macro that is used to declare a new mutex that is visible in this file
                                     /// results in a semaphore variable ebbchar_mutex with value 1 (unlocked)
                                     /// DEFINE_MUTEX_LOCKED() results in a variable with value 0 (locked)

extern char  * message;




// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);



/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .release = dev_release,
};


/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init ass3Read_init(void){
   printk(KERN_INFO "ass3Read: Initializing the ass3Read LKM\n");

   mutex_init(&ass2_mutex);

   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "ass3Read failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "ass3Read: registered correctly with major number %d\n", majorNumber);

   // Register the device class
   ass3ReadClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(ass3ReadClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ass3ReadClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "ass3Read: device class registered correctly\n");

   // Register the device driver
   ass3ReadDevice = device_create(ass3ReadClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(ass3ReadDevice)){               // Clean up if there is an error
      class_destroy(ass3ReadClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ass3ReadDevice);
   }
   printk(KERN_INFO "ass3Read: device class created correctly\n"); // Made it! device was initialized
   return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit ass3Read_exit(void){
   mutex_destroy(&ass2_mutex);
   device_destroy(ass3ReadClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(ass3ReadClass);                          // unregister the device class
   class_destroy(ass3ReadClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "ass3Read: Goodbye from the LKM!\n");
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
	   printk(KERN_INFO "ass3Read: Device has been opened %d time(s)\n", numberOpens);
	   return 0;
	}
   
}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message, size_of_message);

   if (error_count==0){            // if true then have success
      printk(KERN_INFO "ass3Read: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  // clear the position to the start and return 0
   }

   else {
      printk(KERN_INFO "ass3Read: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep){
   mutex_unlock(&ass2_mutex); 
   printk(KERN_INFO "ass3Read: Device successfully closed\n");
   return 0;
}


/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(ass3Read_init);
module_exit(ass3Read_exit);
