// mon_lsm.c
#include <linux/fs.h>
#include <linux/cred.h>
#include <linux/sched.h>
#include <linux/security.h>
#include <linux/mm_types.h>

#include "mon_poise.h"
#include "mon_lsm.h"

#define current_pid()		((uint32_t)(current->pid))
#define current_uid()		((uint32_t)(current->cred->uid.val))
#define current_comm()	((char*)(current->comm))
#define task_pid(task)	(((uint32_t)((task)->pid)))
#define task_uid(task)	((uint32_t)((task)->real_cred->uid.val))
#define task_comm(task)	((char*)((task)->comm))

static char *file_to_name(const struct file *filp)
{
	struct dentry *dentry = NULL;
	char *filename = NULL;
	if(!filp || !filp->f_dentry)
		return "Unknown";

	dentry = filp->f_dentry;
	filename = dentry->d_iname;

	return filename;
}

static char *path_to_name(const struct path *path)
{
	struct dentry *dentry = NULL;
	char *filename = NULL;
	if(!path || !path->dentry)
		return "Unknown";
	
	dentry = path->dentry;
	filename = dentry->d_iname;
	return filename;
}

/*
 * Check whether @mgr is allowed to be the binder context manager
 * @mgr contains the task_struct for the task being registered
 * return 0 if permission is granted
 */
static int mon_binder_set_context_mgr(struct task_struct *mgr)
{
	return 0;
}

/*
 * Check whether @from is allowed to invoke a binder transaction all to @to
 * @from contains the task_struct for the sending task
 * @to contains the task struct for the receiving task
 */
static int mon_binder_transaction(struct task_struct *from, struct task_struct *to)
{
#if 0
	u32 cursid	= current_sid();
	u32 srcsid	= task_sid(from);
	u32	dstsid	= task_sid(to);
#endif
	printk(KERN_INFO "LSMTEST(%d) %s: %d(%d:%s) -> %d(%d:%s) \n", 
			current_pid(),
			__func__, 
			task_uid(from), task_pid(from), task_comm(from),
			task_uid(to), task_pid(to), task_comm(to));
	return 0;
}

/*
 * Check whether @from is allowed to transfer a binder reference to @to
 */
static int mon_binder_transfer_binder(struct task_struct *from, struct task_struct *to)
{
	printk(KERN_INFO "LSMTEST(%d) %s: %d(%d:%s) -> %d(%d:%s)\n", 
			current_pid(), __func__, task_uid(from), task_pid(from), task_comm(from),
			task_uid(to), task_pid(to), task_comm(to));
	return 0;
}

/*
 * Check whether @from is allwed to transfer @file to @to
 * @file contains the struct file being transferred
 */
static int mon_binder_transfer_file(struct task_struct *from, struct task_struct *to, struct file *file)
{
	char *fname = file_to_name(file);
	printk(KERN_INFO "LSMTEST(%d) %s: %d(%d:%s) -> %d(%d:%s) %s\n", 
			current_pid(), __func__, task_uid(from), task_pid(from), task_comm(from),
			task_uid(to), task_pid(to), task_comm(to), fname);
	return 0;
}

/*
 * Check for permission to chage DAC's permission of a file or a directory
 * @path contains the path structure
 * @mode contains DAC's mode
 * return 0 if permission is granted
 */
static int mon_path_chmod(struct path *path, umode_t mode)
{
	char* pname = path_to_name(path);
	printk(KERN_INFO "LSMTEST(%d) %s: Try to change %s to mode %04x\n", current_pid(), __func__, pname, mode);
	return 0;
}

/*
 * Check for permission to change owner/group of a file or a directory
 * @path contains the path structure
 * @uid contains new owner's ID
 * @gid contains new group's ID
 * return 0 if  permission is granted
 */
static int mon_path_chown(struct path *path, kuid_t uid, kgid_t gid)
{
	char* pname = path_to_name(path);
	printk(KERN_INFO "LSMTEST(%d) %s: Try to change %s to owner %d:%d\n", current_pid(), __func__,  pname, uid.val, gid.val);
	return 0;
}

/*
 * Check permission for an ioctl operation on @file
 */
static int mon_file_ioctl(struct file *file, unsigned int command,
		unsigned long arg)
{ // No operation is allowed to operate the virtual devices of Poise
	return 0;
}

/*
 * Check permissions before changing memory access permissions
 * @vma contains the memory region to be modified
 * @reqprot contains the protection requested by the application
 * @prot contains the protection that will be applied by the kernel
 * Return 0 if permission is granted
 */
static int mon_file_mprotect(struct vm_area_struct *vma, unsigned long reqprot, unsigned long prot)
{ // No operation is allowed to modify the permission of the memory map of the system service
#if 0
	unsigned long vm_start	= vma->vm_start;
	unsigned long vm_end		= vma->vm_end;
	struct mm_struct *vm_mm	= vma->vm_mm;						// The address spacke it belongs to
	// unsigned long vm_perm		= vma->vm_page_prot.pgprot;
	struct file *exe_file		= vm_mm->exe_file;
	char *exe_file_name			= file_to_name(exe_file);
	printk(KERN_INFO "LSMTEST(%d): Try to chage the mem permission (0x%08x-0x%08x) of %s to 0x%08x:0x%08x\n",
			current_pid(), vm_start, vm_end, exe_file_name, reqprot, prot);
#endif
	return 0;
}

static int mon_file_open(struct file *filp, const struct cred *cred)
{ // No operation is allowed to write data to the virtual devices of Poise
	int ret = 0;
	char *fname = NULL;
	if(!filp || !filp->f_path.dentry)
		return 0;
	fname = file_to_name(filp);
	if(memcmp("/dev/", fname, 5) == 0)
		printk(KERN_INFO "LSMTEST(%d): Try to open file %s\n", current_pid(), fname);
	return 0;
}

static int mon_task_create(unsigned long clone_flags)
{
	printk(KERN_INFO "LSMTEST(%d): New task is created uid=%d comm=%s\n", 
			current_pid(), current_uid(), current_comm());
	return 0;
}

/*
 * Determine whether the current process may access another
 * @child is the process to be accessed
 * @mode is the mode of attachment
 */
static int mon_ptrace_access_check(struct task_struct *child, unsigned int mode)
{ // Check wether the target task is the system service
	printk(KERN_INFO "LSMTEST(%d): Current process %d(%s) will inject process %d(%s)...\n", 
			current_pid(), current_pid(), current_comm(), task_pid(child), task_comm(child));
	return -EINVAL;
	//return 0;
}

/*
 * Determine whether another process may trace the current process
 * @parent is the task proposed to be the tracer
 */
static int mon_ptrace_traceme(struct task_struct *parent)
{ // Check whether there is task that tries to inject the system service
	printk(KERN_INFO "LSMTEST(%d): Process %d(%s) will inject current process %d(%s)...\n", 
			current_pid(), task_pid(parent), task_comm(parent), current_pid(), current_comm());
	return 0;
}

/*==============================================================================*/
#if 0
/*
 * Lookup the task the target system service according to its name (@exeName)
 */
static struct task_struct* lookup_task(const char* exeName)
{
	struct task_struct *task = &init_task;
}
#endif

static struct security_operations mon_lsm_ops = {
	.name = "lsmtest",
	.binder_set_context_mgr = mon_binder_set_context_mgr,
	.binder_transaction			= mon_binder_transaction,
	.binder_transfer_binder	= mon_binder_transfer_binder,
	.path_chmod							= mon_path_chmod,
	.path_chown							= mon_path_chown,
	.file_ioctl							= mon_file_ioctl,
	.file_mprotect					= mon_file_mprotect,
	.file_open							= mon_file_open,
	.task_create						= mon_task_create,
	.ptrace_access_check		= mon_ptrace_access_check,
	.ptrace_traceme					= mon_ptrace_traceme
};
#if 0
static char* mon_name = "lsm_poise";
#define set_to_mon_lsm(ops, function)				\
	do {								\
		if (!ops->function) {					\
			pr_debug("Had to override the " #function	\
					" security operation with the default.\n");\
		}						\
		ops->function = mon_##function;			\
	} while (0)
#endif
int poise_lsm_init(void) 
{
	dbg_enter_fun();
#if 0
	struct cred *cred;
	struct security_operations *ops = &mon_lsm_ops;
	strcpy(ops->name, mon_name);
	set_to_mon_lsm(ops, binder_set_context_mgr);
	set_to_mon_lsm(ops, binder_transaction);
	set_to_mon_lsm(ops, binder_transfer_binder);
	set_to_mon_lsm(ops, binder_transfer_file);
	set_to_mon_lsm(ops, path_chmod);
	set_to_mon_lsm(ops, path_chown);
	set_to_mon_lsm(ops, file_ioctl);
	set_to_mon_lsm(ops, file_mprotect);
	set_to_mon_lsm(ops, file_open);
	set_to_mon_lsm(ops, task_create);
	set_to_mon_lsm(ops, ptrace_access_check);
	set_to_mon_lsm(ops, ptrace_traceme);
	if(!security_module_enable(&mon_lsm_ops)) {
		printk(KERN_INFO "LSMTEST(%d): It has already started..\n", current_pid());
		return 0;
	}
#endif
	if(!cus_register_security(&mon_lsm_ops)) {
		printk(KERN_INFO "LSMTEST(%d): Unable to register with kernle!!!\n", current_pid());
	}
	
	printk(KERN_INFO "LSMTEST(%d): It is registered with kernle!!!\n", current_pid());
	return 0;
}

void poise_lsm_exit(void)
{
	cus_unregister_security();
	printk(KERN_INFO "LSMTEST(%d): Unregistered LSMTEST from kernle!!!\n", current_pid());
}


#if 0
module_init (mon_lsm_init);
module_exit (mon_lsm_exit);

security_initcall(mon_lsm_init);

MODULE_AUTHOR("Lei");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Linux Security Module of POISE");
#endif
