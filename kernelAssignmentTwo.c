


#include <linux/module.h>   //need this to work with LKMs
#include <linux/kernel.h>   //need this for kernel functions
#include <linux/init.h>
#include <linux/device.h>   //need this to use the kernel driver module
#include <linux/fs.h>       //need this for linux file system support


//variables for device number, memory allocation, and user input

static int majorNumber;     //device number
static char message[1024] = {0};    //memory allocation for input strings
static short sizeOfStringStored;
static struct class* kernelAssignmentTwoClass = NULL;
static struct device* kernelAssignmentTwoDevice = NULL;



int init_module(void)
{
    printk(KERN_INFO "Installing module...\n");

    //if there was an error with the device number, we need to terminate
    if(majorNumber<0)
    {
        printk(KERN_ALERT "couldn't register a device number\n");
        return majorNumber;
    }


    printk(KERN_ALERT "successfully registered the device number woohoo!\n");

    //make a device for the module
    kernelAssignmentTwoDevice = device_create(kernelAssignmentTwo, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);

    return 0;
}



//get rid of the kernel module
//I wrapped the exit method from out implementation example into this
//so that we can get rid of the device and class while we clean this
//out of the kernel
void cleanup_module(void)
{
    printk(KERN_INFO "Removing module...\n");
    //get rid of the device
    device_destroy(kernelAssignmentTwoClass, MKDEV(majorNumber, 0));
    //take the device out of its class
    class_unregister(kernelAssignmentTwoClass);
    //remove the class the device was associated with
    class_destroy(kernelAssignmentTwoClass);
    //get rid of the device number
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "finished with module and LKM cleanup\n");
}
