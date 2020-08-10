// mon_rs.h
#ifndef __MON_RS_H
#define __MON_RS_H

#define MAX_CXT_SIZE	4088 

struct LOCATION_DATA {
	char location[128];
};

enum {
	CXT_LOCATION = 1,
	CXT_DISPLAYSTATE,
	CXT_PACKAGE,
	CXT_ACTIVITY,
	CXT_PROCESS
};

struct cxt_info {
	struct timeval	ts;
	unsigned int		len;
	unsigned char		buf[MAX_CXT_SIZE];
};

int readDisplayState();
int readLocation();
int readPackageList();

int  poise_rs_init();
void poise_rs_exit();

#endif
