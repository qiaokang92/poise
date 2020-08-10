// sensorcollector.h
#ifndef __SENSOR_COLLECTOR_H
#define __SENSOR_COLLECTOR_H

enum {
	GPS_LOCATION = 1,
	NET_LOCATION
};

struct sensorInfo {
	uint32_t type;
	uint32_t revserve;
	uint64_t timestamp;
	int64_t  data[3];
};

/* The location informaion 16 bytes */
struct locationInfo {
	uint32_t  type;      // 1=GPS location; 2=NET location
	uint32_t  longitude; // the value divided by 1000000.0 is the actual value
	uint32_t  latitude;  // the value divided by 1000000.0 is the actual value
	uint32_t	acc;       // accuracy
};

/* The context information (48 bytes) */
struct contextInfo { 
	struct locationInfo location; // 16 bytes: The location information (p3/p4)
	uint32_t is_screen_active;    //  4 bytes: Is screen active? (p5)
	uint32_t is_target_focused;   //  4 bytes: Is UI window is focused? (p6)
	uint32_t is_camera_active;		//  4 bytes: Is camera active? (p7)
	uint8_t  device_serialno[20]; // 20 bytes: The device serial number (p1/p2)
};

//void readContextInfo(void);
int sensor_start_collector(void);
#endif
