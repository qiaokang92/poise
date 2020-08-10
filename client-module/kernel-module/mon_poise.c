// mon_poise.c 
// The main file of poise
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include "mon_poise.h"
#include "mon_nf.h"
#include "mon_lsm.h"
#include "mon_rs.h"
#include "mon_sk.h"
#include "totp.h"

extern struct cxt_info cxt_location_info;
extern struct cxt_info cxt_package_info;
extern struct cxt_info cxt_display_info;
static unsigned char reply_msg[65535] = {0};

static int		ms_period = 300;
static int		read_context_auto = 0;
static struct task_struct *poll_thread = NULL;


/*
 * Encode the context information into the packet buffer
 */
static unsigned char* encode_cxt(unsigned char *p, struct cxt_info *cxt, int type) {
	if(cxt->len == 0)
		return p;
	*((int *)p) = type;
	p += sizeof(type);
	memcpy(p, (char*)&cxt->ts, sizeof(struct timeval));
	p += sizeof(struct timeval);
	*((int *)p) = cxt->len;
	p += sizeof(cxt->len);
	memcpy(p, cxt->buf, cxt->len);
	p += cxt->len;
	return p;
}
/*
 * Encode the context authentication or heartbeat data
 */
static unsigned char* encode_auth_heartbeat(unsigned char *p, uint32_t key, uint32_t index) {
	struct timeval ts;
	do_gettimeofday(&ts);
	return p;
}
/*
 * Reply context info periodly and automactically (heartbeat packet)
 */
static int thread_reply_context() 
{
	unsigned char *p = NULL;
	unsigned int  size = 0;
	uint32_t index	= 0; // 0: authentication packet; Others: heartbeat packet
	uint32_t newCode = 0;
	uint8_t hmacKey[] = {0x4d, 0x79, 0x4c, 0x65, 0x67, 0x6f, 0x44, 0x6f, 0x6f, 0x72}; // Secret key
	totp_init(hmacKey, 10, 7200); // Secret key, key lengh, Timestep (7200s/2hours)
	totp_setTimezone(8);
	while(read_context_auto) {
		newCode = totp_getCodeFromSteps(index);
		p = reply_msg;
		p = encode_auth_heartbeat(p, newCode, index);
		p = encode_cxt(p, &cxt_location_info, CXT_LOCATION);
		p = encode_cxt(p, &cxt_package_info, CXT_PACKAGE);
		p = encode_cxt(p, &cxt_display_info, CXT_DISPLAYSTATE);
		size = (unsigned int)(p-reply_msg);
		dbg_log("Try to reply server %d bytes...\n", size);
		poise_reply_server(reply_msg, size);
		msleep(ms_period);
		index++;
	}
}

static int __init poise_init_module(void)  
{
	dbg_enter_fun();

#ifndef DBG_LSM
	// Initialize sensor collecting module
	if(poise_rs_init() != 0) { 
		printk(KERN_ERR "[POISE]: Failed to initialize sensor collecting module..\n");
		return 0;
	} else {
		dbg_log("Successfully initialized sensor collecting module.\n");
	}

	// Create thread to periodly reply context information
	char thread_info[] = "ThreadReadContext";
	poll_thread = kthread_create((void*)thread_reply_context, NULL, thread_info);
	if (poll_thread == NULL)
	{
		printk(KERN_ERR "[POISE]: Failed to create the poll thread...\n");
	} else {
		read_context_auto = 1;
		wake_up_process(poll_thread);
	}

	// Initialize NETFILTER module to monitor the traffic of the specified apps
	if(poise_nf_init() != 0) {
		printk(KERN_ERR "[POISE]: Failed to initialize NETFILTER module..\n");
	} else {
		dbg_log("Successfully initialized NETFILTER module.\n");
	}
#endif
	// Initialize the LSM submodule
	poise_lsm_init();
	
	dbg_exit_fun();
	return 0;
}  

static void __exit poise_exit_module(void)  
{  
	dbg_enter_fun();
#ifndef DBG_LSM
	// Exit NETFILTER module
	poise_nf_exit();
#endif
	// Exit the LSM submodule
	poise_lsm_exit();
#if 0
	int ret = 0;
	ret = kthread_stop(poll_thread);
	if(!ret) {
		printk(KERN_ERR "[POISE]: Failed to stop sensor collectiong thread...\n");
	} else {
		dbg_log("POISE: Sensor collectiong thread stopped.\n");
	}
#endif

#ifndef DBG_LSM
	// Exit the thread that periodly replies context information
	read_context_auto = 0;
	// Exit the thread that periodly reads context information
	poise_rs_exit();
	// Waiting for all threads exit
	msleep(ms_period);
#endif
	dbg_exit_fun();
}  

module_init(poise_init_module);  
module_exit(poise_exit_module);  

MODULE_AUTHOR("Lei");  
MODULE_LICENSE("GPL");  
MODULE_DESCRIPTION("POISE Context Collector");  
