// montest.h
#ifndef __MON_TEST_H
#define __MON_TEST_H


#define RUN_DEBUG				1

#define MAX_PAYLOAD			4096
#define MAX_SENSOR_NUM	7

#define PROC_PHONE_CALL 0x1
#define PROC_CAMERA			(0x1 << 1)
#define PROC_C

enum {
	MSG_SYNC_SENSOR_INFO = 1,
	MSG_SYNC_GPS_INFO,
	MSG_CONF_RESPONSE_CYCLE
};

struct comm_msg {
	uint32_t type;
	uint32_t dlen;
	// uint32_t sensor_info_num;
	// uint32_t system_info_num;
	// uint32_t proc_info_num;
	unsigned char data[0];
};

struct system_info {
	uint32_t brightness;
	uint32_t load;
};

struct proc_info {
	uint64_t  is_app_active;
};

int mon_sync_message(uint32_t type, uint32_t dlen, uint8_t *data);
#endif
