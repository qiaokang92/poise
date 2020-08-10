// mon_gps.c is used for read gps related information
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>

#include <mach/msm_smd.h>

#define MAX_BUF_SIZE	200


