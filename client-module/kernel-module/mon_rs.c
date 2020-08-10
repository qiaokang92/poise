// mon_rs.c
// Read context information through invoking dumpsys
//
//
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/if_ether.h>

#include "mon_poise.h"
#include "mon_rs.h"

unsigned char src_dmac[ETH_ALEN] = {0x0};
uint32_t			src_ip = 0;

int syn_msg = 0;

static int isRun = 0;
// static int isFni = 0;
static struct task_struct *p_thread = NULL;

static const char *envp[] = {
	"HOME=/",
	"PATH=/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin",
	"TERM=xterm",
	NULL
};

// Read IP and MAC address
int readIPAndMacAddr() {
}

// Read power realted information
int readDisplayState() {
	int res = 0;
	static char *argv[] = {
		"/system/bin/sh",
		"-c",
		"/system/bin/nohup "
		"/system/bin/dumpsys power | grep -Eo \"^Display Power: state=(ON|OFF)$\""
		" >> /dev/poisedev &",
		NULL
	};
	dbg_log("Try to run \"%s\"\n", argv[2]);
	syn_msg = CXT_DISPLAYSTATE;
	res = call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);
	while(syn_msg != 0){
		msleep(10);
		if(isRun == 0)
			break;
	}
	dbg_log("Read location res=0x%08x\n", res);
	return res;
}
// Read location information from system service
int readLocation() {
	int res = 0;
	static char *argv[] = {
		"/system/bin/sh",
		"-c",
		"/system/bin/nohup "
		"/system/bin/dumpsys location | grep -Eo \"(^gps|^net) Location\\[ ([0-9]*)\\.([0-9]*),([0-9]*)\\.([0-9]*) acc=([0-9]*) t=(.*) et=(.*)\\]$\""
		" >> /dev/poisedev &",
		NULL
	};
	dbg_log("Try to run \"%s\"\n", argv[2]);
	syn_msg = CXT_LOCATION;
	res = call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);
	while(syn_msg != 0){
		msleep(10);
		if(isRun == 0)
			break;
	}
	dbg_log("Read location res=0x%08x\n", res);
	return res;
}
// Read installed package list 
// "/system/bin/pm list packages"
int readPackageList() {
	int res = 0;
	static char *argv[] = {
		"/system/bin/sh",
		"-c",
		"/system/bin/nohup "
		"/system/bin/dumpsys package | grep -Eo \"Package \\[.*\\]\" | cut -d\' \' -f2"
		" >> /dev/poisedev &",
		NULL
	};
	dbg_log("Try to run \"%s\"\n", argv[2]);
	syn_msg = CXT_PACKAGE;
	res = call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);
	while(syn_msg != 0){
		msleep(10);
		if(isRun == 0)
			break;
	}
	dbg_log("Read package list res=0x%08x\n", res);
	return res;
}

int thread_fn() {
	int res1 = 0, res2 = 0, res3 = 0;
	dbg_enter_fun();
#if 0
	/*static char *argv1[] = {
		"/system/bin/sh",
		"-c",
		"echo `/system/bin/date` >> /data/local/tmp/test-dir/date.log",
		NULL
	};*/
	
		//"echo `/system/bin/date` >> /sdcard/date_kernel.log;",
		// "res=\`/system/bin/date -u\`;/system/bin/echo $res >> /sdcard/date_kernel.log",
		//"/system/bin/nohup /system/bin/echo $(/system/bin/dumpsys location | grep gps) >> /sdcard/test1.log &",
   static char *argv1[] = {
		"/system/bin/sh",
		"-c",
		"/system/bin/nohup "
		"/system/bin/echo $("
		"/system/bin/dumpsys location | grep -Eo \"(^gps|^net) Location\\[ ([0-9]*)\\.([0-9]*),([0-9]*)\\.([0-9]*) acc=([0-9]*) t=(.*) et=(.*)\\]$\""
		") >> /sdcard/test1.log &",
		NULL
	}; 
	static char *argv1[] = {
		"/system/bin/sh",
		"-c",
		"/system/bin/nohup "
		"/system/bin/dumpsys location | grep -Eo \"(^gps|^net) Location\\[ ([0-9]*)\\.([0-9]*),([0-9]*)\\.([0-9]*) acc=([0-9]*) t=(.*) et=(.*)\\]$\""
		" >> /dev/poisedev &",
		NULL
	};
	
	static char *argv2[] = {
		"/system/bin/sh",
		"-c",
		"/system/bin/ps >> /data/local/tmp/test-dir/ps.log",
		NULL
	};
		// "/system/bin/echo \`/system/bin/dumpsys location\` >> /sdcard/dumpsys_kernel.log",
		// "/system/bin/echo \`/system/bin/dumpsys location\` >> /sdcard/dumpsys_kernel.log",
		// "p=\`/system/bin/pm list packages\`;/system/bin/echo $p >> /sdcard/dumpsys_kernel.log",
	static char *argv3[] = {
		"/system/bin/sh",
		"-c",
		"ts=$(/system/bin/date -u);/system/bin/echo $ts >> /sdcard/dumpsys_kernel.log;"
		"l=$(system/bin/dumpsys location);/system/bin/echo $l >> /sdcard/dumpsys_kernel.log;"
		"te=$(/system/bin/date -u);/system/bin/echo $te >> /sdcard/dumpsys_kernel.log;"
		"/system/bin/clear",
		NULL
	};
#endif
	while( isRun ) {
		readLocation();
		readDisplayState();
		readPackageList();
		msleep(3000);
	}
	dbg_exit_fun();
	// isFni = 1;
}

// Thread for periodly reading context info
static void poise_context_thread(){
	char thread_name[] = "poise_thread";
	p_thread = kthread_create(thread_fn, NULL, thread_name);
	if( p_thread != NULL ) {
		dbg_log("Enter %s\n", thread_name);
		isRun = 1;
		wake_up_process(p_thread);
	}
	return 0;
}
#if 0
static int rs_read_location(unsigned char* pbuff, unsigned int mlen) {
	int res = 0;
	struct subprocess_info *sub_info;
	/*static char *argv[] = {
		"/system/bin/dumpsys",
		"location",
		">",
		"/data/local/tmp/test-dir/test.log",
		NULL
	};*/
	static char *argv[] = {
		"/system/bin/sh",
		"-c",
		"res=\`/system/bin/date\`;"
			"/system/bin/echo $res > /sdcard/test1.log;"
			"/system/bin/echo $res > /sdcard/date_kernel.log",
		NULL
	};

		// "TERM=linux",
	static char *envp[] = {
		"HOME=/",
		"PATH=/sbin:/bin:/usr/bin:/usr/sbin:/system/bin:/system/xbin",
		"DEBUG=kernel",
		NULL
	};

	sub_info = call_usermodehelper_setup(argv[0], argv, envp, GFP_ATOMIC);
	if(sub_info == NULL) {
		printk(KERN_ERR "[POISE]: Failed to setup usermode helper subprocess..\n");
		return -ENOMEM;
	}
	dbg_log("Setup usermode helper subprocess..\n");

	res = call_usermodehelper_exec(sub_info, UMH_WAIT_PROC);
	dbg_log("The process is \"%s\", pid is %d, res=0x%08x\n",	current->comm, current->pid, res);

	poise_context_thread();

	return res;
}
#endif

unsigned char* rs_read_context() {
	unsigned char* data_buff = NULL;
	//rs_read_location(NULL, 0);
	//rs_read_location_exec(NULL, 0);
	poise_context_thread();
	return data_buff;
}

int poise_rs_init(void)  
{
	dbg_enter_fun();
	if (poise_fs_init() == 0) {
		dbg_log("Successfully initialize virtual device..\n");
	} else {
		printk(KERN_ERR "[POISE]: Failed to initialize virtual device..\n");
		return -1;
	}
	rs_read_context();
	return 0;
}

void poise_rs_exit(void)
{
	int ret = 0;
	dbg_enter_fun();
#if 1
	isRun = 0;
#else
	ret = kthread_stop(p_thread);
	if(!ret) {
		printk(KERN_INFO "POISE: Sensor collectiong thread stopped.\n");
	}
#endif
	poise_fs_fini();
	dbg_exit_fun();
}
