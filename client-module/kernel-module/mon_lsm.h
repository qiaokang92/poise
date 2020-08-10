#ifndef _MON_LSM_H
#define _MON_LSM_H
#include <linux/major.h>

#define DBG_LSM


#define SPIN_LOCK \
	static spinlock_t	lock = SPIN_LOCK_UNLOCKED; \
	spin_lock(&lock);

#define SPIN_UNLOCK	\
	spin_unlock(&lock);

#define security_alert(normal_msg, flood_msg, args...)			\
({									\
 	static unsigned long warning_time = 0, no_flood_yet = 0;	\
	static spinlock_t security_alert_lock = SPIN_LOCK_UNLOCKED;	\
									\
	spin_lock(&security_alert_lock);				\
									\
/* Make sure at least one minute passed since the last warning logged */\
	if (!warning_time || jiffies - warning_time > 60 * HZ) {	\
		warning_time = jiffies; no_flood_yet = 1;		\
		printk(KERN_ALERT "Security: " normal_msg "\n", ## args);\
	} else if (no_flood_yet) {					\
		warning_time = jiffies; no_flood_yet = 0;		\
		printk(KERN_ALERT "Security: more " flood_msg		\
			", logging disabled for a minute\n");		\
	}								\
									\
	spin_unlock(&security_alert_lock);				\
})

int poise_lsm_init(void);
void poise_lsm_exit(void);
#endif /* _MON_LSM_H */
