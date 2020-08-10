//mon_umh.c: The user module helper 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kmod.h>


#include "mon_nf.h"
#include "mon_umh.h"


int mon_umh_location(void)
{
	//struct subprocess_info *sub_info;
	int res = 0;
	char *argv[]= {
		"/system/bin/rm",
		"/sdcard/test.txt",
		NULL};
	static char *envp[] = {
		"HOME=/",
		"PATH=/sbin:/bin:/usr/sbin:/system/bin:/system/xbin",
	  "DEBUG=kernel",
	  NULL};
	res = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
	printk(KERN_DEBUG "[MON] The process result is %d\n", res);
	return res;
}

