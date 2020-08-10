#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf2749611, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x897473df, __VMLINUX_SYMBOL_STR(mktime) },
	{ 0x8f083c31, __VMLINUX_SYMBOL_STR(cus_unregister_security) },
	{ 0xab1b9772, __VMLINUX_SYMBOL_STR(cus_register_security) },
	{ 0x449ad0a7, __VMLINUX_SYMBOL_STR(memcmp) },
	{ 0x8b7780bb, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0xf145b863, __VMLINUX_SYMBOL_STR(sock_release) },
	{ 0xf7f7edd8, __VMLINUX_SYMBOL_STR(kernel_recvmsg) },
	{ 0xfa66f77c, __VMLINUX_SYMBOL_STR(finish_wait) },
	{ 0xd62c833f, __VMLINUX_SYMBOL_STR(schedule_timeout) },
	{ 0x34f22f94, __VMLINUX_SYMBOL_STR(prepare_to_wait_event) },
	{ 0x4c4fef19, __VMLINUX_SYMBOL_STR(kernel_stack) },
	{ 0x1b6314fd, __VMLINUX_SYMBOL_STR(in_aton) },
	{ 0x547c5b50, __VMLINUX_SYMBOL_STR(sock_create) },
	{ 0x6b754597, __VMLINUX_SYMBOL_STR(inet_release) },
	{ 0xfc536ae2, __VMLINUX_SYMBOL_STR(kernel_sendmsg) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x9af131a0, __VMLINUX_SYMBOL_STR(kernel_sock_shutdown) },
	{ 0xe468f8eb, __VMLINUX_SYMBOL_STR(sock_create_kern) },
	{ 0xa515ec54, __VMLINUX_SYMBOL_STR(dev_queue_xmit) },
	{ 0xfe552a6a, __VMLINUX_SYMBOL_STR(skb_push) },
	{ 0xe113bbbc, __VMLINUX_SYMBOL_STR(csum_partial) },
	{ 0x9f5a2b54, __VMLINUX_SYMBOL_STR(__alloc_skb) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x85ca7dc9, __VMLINUX_SYMBOL_STR(nf_unregister_hook) },
	{ 0x3179b24c, __VMLINUX_SYMBOL_STR(nf_register_hook) },
	{ 0x3c23b921, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xc464e63c, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xcb44ddf6, __VMLINUX_SYMBOL_STR(ip_local_out_sk) },
	{ 0xf6b2d3a, __VMLINUX_SYMBOL_STR(kfree_skb) },
	{ 0x9d6b3454, __VMLINUX_SYMBOL_STR(skb_trim) },
	{ 0xb0e602eb, __VMLINUX_SYMBOL_STR(memmove) },
	{ 0x1bbe7976, __VMLINUX_SYMBOL_STR(skb_put) },
	{ 0x23db55df, __VMLINUX_SYMBOL_STR(skb_copy_expand) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x20c55ae0, __VMLINUX_SYMBOL_STR(sscanf) },
	{ 0x3be15cfd, __VMLINUX_SYMBOL_STR(class_unregister) },
	{ 0x5175c333, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x7f1cd6e5, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x547012e0, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x6bc3fbc0, __VMLINUX_SYMBOL_STR(__unregister_chrdev) },
	{ 0xa2f0c10b, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x437f3b0c, __VMLINUX_SYMBOL_STR(__register_chrdev) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x4f68e5c9, __VMLINUX_SYMBOL_STR(do_gettimeofday) },
	{ 0xf9a482f9, __VMLINUX_SYMBOL_STR(msleep) },
	{ 0xc5fdef94, __VMLINUX_SYMBOL_STR(call_usermodehelper) },
	{ 0x6f123511, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0xcbddba80, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

