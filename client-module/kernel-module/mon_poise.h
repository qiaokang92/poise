// mon_poise.h
#ifndef	_MON_POISE_H
#define _MON_POISE_H

#define DEBUG
// #define DEBUG_POISE
// #undef	DEBUG_POISE


#ifdef DEBUG


#define dbg_err(...) \
	printk(KERN_ERR "[POISE]:\t"); \
	printk(__VA_ARGS__);

#define dbg_log(...) \
	printk(KERN_DEBUG "[POISE]:\t"); \
	printk(__VA_ARGS__);

#define dbg_enter_fun()	\
	printk(KERN_DEBUG "[POISE]: Enter %s\n", __FUNCTION__)
#define dbg_exit_fun() \
	printk(KERN_DEBUG "[POISE]: Exit  %s\n", __FUNCTION__)

#else

#define dbg_log(format,args...) \
	{}

#define dbg_enter_func() \
	{}
#define dbg_exit_func() \
	{}

#endif



#endif // _MON_POISE_H
