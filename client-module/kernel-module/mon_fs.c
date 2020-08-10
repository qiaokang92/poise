// mon_fs.c
// Register virtual device for receiving data from userspace

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "mon_rs.h"
#include "mon_fs.h"
#include "mon_poise.h"


#define DEVICE_NAME "poisedev"
#define CLASS_NAME	"poise"

struct cxt_info	cxt_location_info;
struct cxt_info cxt_package_info;
struct cxt_info cxt_display_info;

static char poise_msg_response[MAX_CXT_SIZE] = {0};
//static char poise_context_data[MAX_CXT_SIZE] = {0};
static int misc_ret = -1;

extern int syn_msg;

// open()
static int poise_open(struct inode *inode, struct file *fsensor)
{
	dbg_log("Open device.\n");
	return 0;
}

// read()
static ssize_t poise_read(struct file *pfile, char *buffer, size_t len, loff_t *offset)
{
	int error_count = 0;
	error_count = copy_to_user(buffer, poise_msg_response, sizeof(poise_msg_response));
	if(error_count == 0) {
		dbg_log("Send %d characters to userspace.\n", sizeof(poise_msg_response));
		return 0;
	} else {
		printk(KERN_ERR "[POISE]: Failed to send %d characters to userspace.\n", sizeof(poise_msg_response));
		return -EFAULT;
	}
}

// write()
static ssize_t poise_write(struct file *pfile, const char *buffer, size_t len, loff_t *offset)
{
	struct cxt_info *cxt = NULL;
	struct tm t;
	// memset(poise_context_data, '\0', sizeof(poise_context_data));
	// memcpy(poise_context_data, buffer, len);
	// printk(KERN_INFO "POISE: Received %d bytes (%s)\n", len, poise_context_data);
	switch (syn_msg) {
		case CXT_LOCATION:
			cxt = &cxt_location_info;
			syn_msg = 0;
			dbg_log("Received location data %d bytes.\n", len);
			//dbg_log("%s\n", poise_context_data);
			//memset((char*)&cxt_location_info, '\0', sizeof(cxt_location_info));
			break;
		case CXT_DISPLAYSTATE:
			cxt = &cxt_display_info;
			syn_msg = 0;
			dbg_log("Received display power %d bytes.\n", len);
			//dbg_log("%s\n", poise_context_data);
			break;
		case CXT_PACKAGE:
			cxt = &cxt_package_info;
			syn_msg = 0;
			dbg_log("Received installed package list %d bytes.\n", len);
			//dbg_log("%s\n", poise_context_data);
			break;
		default:
			dbg_log("Unknown data %d bytes %s\n", len, buffer);
			break;
	}
	if(cxt != NULL) {
		memset((char*)cxt, '\0', sizeof(struct cxt_info));
		cxt->len = len > MAX_CXT_SIZE ? MAX_CXT_SIZE : len;
		do_gettimeofday(&cxt->ts);
		memcpy(cxt->buf, buffer, cxt->len);
		dbg_log("%u.%u: %s\n", cxt->ts.tv_sec, cxt->ts.tv_usec, cxt->buf);
	}
	return 0;
}

// mmap()
static int poise_mmap(struct file *pfile, struct vm_area_struct* vs) {
	printk(KERN_INFO "[POISE]: Mmap device..\n");
	return 0;
}

// flush()
static int poise_flush(struct file *pfile, fl_owner_t id) {
	printk(KERN_INFO "[POISE]: Flush device (id=0x%08x)...\n", id);
	return 0;
}

// fsync
static int poise_fsync(struct file *pfile, loff_t s, loff_t e, int datasync) {
	printk(KERN_INFO "[POISE]: Fsync device (0x%08x-0x%08x)...\n", s, e);
	return 0;
}

// close()
static int poise_release(struct inode *inode, struct file *fsensor)
{
	dbg_log("Release device.\n");
	if(syn_msg != 0) {
		dbg_log("Release device (syn_msg=%d)\n", syn_msg);
		syn_msg = 0;
	}
	return 0;
}

static struct file_operations poise_fops = {
	.owner = THIS_MODULE,
	.open  = poise_open,
	.read  = poise_read,
	.write = poise_write,
	.mmap  = poise_mmap,
	.flush = poise_flush,
	.fsync = poise_fsync,
	.release = poise_release,
};


/* static struct miscdevice poise_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SENSOR_DEVICE,
	.fops = &poise_fops,
};*/

static int majorNumber;
static struct class*	poiseClass = NULL;
static struct device* poiseDevice = NULL;
int  poise_fs_init(void) {
	dbg_log("Initializing the POISE LKM.\n");
	
	// Try to dynamically allocate a major number for the device
	majorNumber = register_chrdev(0, DEVICE_NAME, &poise_fops);
	if(majorNumber < 0) {
		printk(KERN_ERR "[POISE]: Failed to register a major number..\n");
		return majorNumber;
	}
	dbg_log("Registered correctly with major number %d..\n", majorNumber);

	// Register the device class
	poiseClass = class_create(THIS_MODULE, CLASS_NAME);
	if(IS_ERR(poiseClass)) {
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ERR "[POISE]: Failed to register device class.\n");
		return PTR_ERR(poiseClass);
	}
	dbg_log("Device class registered correctly.\n");

	// Register the device driver
	poiseDevice = device_create(poiseClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if(IS_ERR(poiseDevice)) {
		class_destroy(poiseClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ERR "[POISE]: Failed to create the device.\n");
		return PTR_ERR(poiseDevice);
	}
	dbg_log("Device class created correctly.\n");
	return 0;
}
void poise_fs_fini(void) {
	device_destroy(poiseClass, MKDEV(majorNumber, 0));
	class_unregister(poiseClass);
	class_destroy(poiseClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	dbg_log("Exit from the LKM.\n");
	/*if(misc_ret == 0) { 
		misc_deregister(&poise_misc);
		printk(KERN_INFO "POISE: De-register the virtual device.\n");
		misc_ret = -1;
	}*/
}
