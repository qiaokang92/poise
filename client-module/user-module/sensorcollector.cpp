// sensorcollector.cpp
//

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <hardware/sensors.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <utils/Timers.h>
#include <regex.h>


#include "sensorcollector.h"
#include "montest.h"

extern int target_app_uid;
extern int is_running; // Is the process still running

static struct sensorInfo		sensor_data[MAX_SENSOR_NUM];
static int32_t active_camera			= 0;
static int32_t active_microphone	= 0;
static int32_t screen_state				= 0;
static int32_t focused_window_uid = 0;

struct contextInfo context_info;

static int getLocation(void)
{
	FILE *pp = NULL;
	char tmp1[64], tmp2[64];
	int chread = 0, cinput = 0, gps_hacc = 0, net_hacc = 0;
	const char *cmd = "dumpsys location | grep \"Location\\\[.*\\\]\"";
	float gps_lo = 1.0, gps_la = 1.0, net_lo = 1.0, net_la = 1.0;
	char gps_acc[64], net_acc[64], gps_t[8], gps_et[8], net_t[8], net_et[8];
	printf("Execute: %s\n", cmd);
	pp = popen(cmd, "r");
	printf("\\n=0x%02x\n", '\n');
	fscanf(pp, "%s %s %f,%f acc=%s t=%s et=%s\n", tmp1, tmp2, &gps_lo, &gps_la, gps_acc, gps_t, gps_et);
	if(strcmp(tmp1, "gps") != 0) {
		printf("[MON] Read GPS Location failed (err=%s)!!!\n", tmp1);
		return 0;
	} else {
		gps_lo = 114.18052;
		gps_la = 22.304106;
		strcpy(gps_acc, "22");
		if(strlen(gps_acc) > 5)
			gps_hacc == 0xfffffff;
		else
			gps_hacc = atoi(gps_acc);
		gps_et[strlen(gps_et)-1] = '\0';
	}
	printf("[MON] %s %s %f,%f acc=%s(0x%08x) t=%s et=%s\n", tmp1, tmp2, gps_lo, gps_la, gps_acc, gps_hacc, gps_t, gps_et);
	
	fscanf(pp, "%s %s %f,%f acc=%s t=%s et=%s\n", tmp1, tmp2, &net_lo, &net_la, net_acc, net_t, net_et);
	if(strcmp(tmp1, "net") != 0) {
		printf("[MON] Read NET Location failed (err=%s)!!!\n", tmp1);
		return 0;
	} else {
		if(strlen(net_acc) > 5)
			net_hacc = 65535;
		else
			net_hacc = atoi(net_acc);
		net_et[strlen(net_et)-1] = '\0';
	}
	printf("[MON] %s %s %f,%f acc=%s(0x%08x) t=%s et=%s\n", tmp1, tmp2, net_lo, net_la, net_acc, net_hacc, net_t, net_et);
	
	if((gps_hacc <= net_hacc) && (gps_hacc > 0)) {
		memset((void*)&context_info.location, 0, sizeof(struct locationInfo));
		context_info.location.type			= GPS_LOCATION;
		context_info.location.longitude = (uint32_t)(gps_lo * 1000000.0);
		context_info.location.latitude  = (uint32_t)(gps_la * 1000000.0);
		context_info.location.acc       = gps_hacc;
	} else if(net_hacc > 0) {
		memset((void*)&context_info.location, 0, sizeof(struct locationInfo));
		context_info.location.type			= NET_LOCATION;
		context_info.location.longitude = (uint32_t)(net_lo * 1000000.0);
		context_info.location.latitude  = (uint32_t)(net_la * 1000000.0);
		context_info.location.acc       = net_hacc;
	}
	printf("[MON] Location: type=%d, %f, %f, %d\n",
			context_info.location.type,
			(float)context_info.location.longitude / 1000000.0,
			(float)context_info.location.latitude / 1000000.0,
			context_info.location.acc);
	pclose(pp);
	return 1;
}

static int getScreenState()
{
	FILE *pp = NULL;
	const char *cmd = "dumpsys display | grep mScreenState";
	char state[32];
	int  slen = 0;
	pp = popen(cmd, "r");
	if(fscanf(pp, "%s", state) != EOF) {
		slen = strlen(state);
		if(state[slen-1] == 'N') {
			screen_state = 1;
			context_info.is_screen_active = 1;
		}	else {
			screen_state = 0;
			context_info.is_screen_active = 0;
		}
		printf("[MON] Screen is ON: %d\n", screen_state);
	}
	pclose(pp);
	return screen_state;
}

static int getCameraState()
{
	FILE *pp = NULL;
	char tmp1[8], tmp2[8], tmp3[8];
	int device_id = -1;
	//const char *cmd = "dumpsys media.camera | grep \'Device \d is open|closed'";
	const char *cmd = "dumpsys media.camera | grep -o \'Device [09] is open\'";
	active_camera = 0;
	pp = popen(cmd, "r");
	while(fscanf(pp, "%s %d %s %s", tmp1, &device_id, tmp2, tmp3) != EOF) {
			active_camera += 1;
	}
	if(active_camera > 0) {
		context_info.is_camera_active = 1;
	} else {
		context_info.is_camera_active = 0;
	}
	printf("[MON] There are %d cameras open.\n", active_camera);
	pclose(pp);
	return 1;
}

static int getWindowState(void)
{
	FILE *pp = NULL;
	const char *cmd1 = "dumpsys window | grep mCurrentFocus=Window";
	char tmp1[32], tmp2[32], tmp3[128], cmd2[256];
	uint32_t i = 0, slen = 0;
	pp = popen(cmd1, "r");
	if(fscanf(pp, "%s %s %s", tmp1, tmp2, tmp3) != EOF) {
		tmp3[strlen(tmp3)-1] = '\0';
		printf("[MON] Focused window: %s\n", tmp3);
	} else {
		memset(tmp3, 0, 128);
	}
	pclose(pp);
	focused_window_uid = 0;
	if(strlen(tmp3) > 0) {
		i = 0;
		while(tmp3[i] != '/') {
			if (tmp3[i] == '\0')
				break;
			i++;
		}
		tmp3[i] = '\0';
		sprintf(cmd2, "cat /data/system/packages.xml | grep %s | grep -o userId=\\\"[0-9]*\\\"");
		pp = popen(cmd2, "r");
		if(fscanf(pp, "%s", tmp1) != EOF) {
			slen = strlen(tmp1) - 9;
			memcpy(tmp2, tmp1+8, slen);
			focused_window_uid = atoi(tmp2);
			printf("[MON] Focused window uid: %d\n", focused_window_uid);
		}
	}
	if(focused_window_uid == target_app_uid)
		context_info.is_target_focused = 1;
	else
		context_info.is_target_focused = 0;
	return 1;
}

static int getSerialNo(void) 
{
	FILE *pp = NULL;
	const char *cmd = "getprop ro.serialno";
	pp = popen(cmd, "r");
	fscanf(pp, "%s", context_info.device_serialno);
	pclose(pp);
	printf("[MON] Serial No.: %s\n", context_info.device_serialno);
	return 1;
}

static void readContextInfo(void)
{
	memset((void *)&context_info, 0, sizeof(context_info));
	getSerialNo();
	getLocation();
	getScreenState();
	getWindowState();
	getCameraState();
}

static void sensor_sync_data()
{
	//int dlen = sizeof(sensor_data);
	int dlen = sizeof(context_info);
	readContextInfo();
	//mon_sync_message(MSG_SYNC_SENSOR_INFO, dlen, (uint8_t *)sensor_data);
	mon_sync_message(MSG_SYNC_SENSOR_INFO, dlen, (uint8_t *)&context_info);
	sleep(3);
}

int sensor_start_collector()
{
	 while(is_running)
		 sensor_sync_data();
	 return 0;
}

/* Functions for sensor data collection */
static char const* getSensorName(int type) {
	switch(type) {
		case SENSOR_TYPE_ACCELEROMETER:
			return "Acc";
		case SENSOR_TYPE_MAGNETIC_FIELD:
			return "Mag";
		case SENSOR_TYPE_ORIENTATION:
			return "Ori";
		case SENSOR_TYPE_GYROSCOPE:
			return "Gyr";
		case SENSOR_TYPE_LIGHT:
			return "Lux";
		case SENSOR_TYPE_PRESSURE:
			return "Bar";
		case SENSOR_TYPE_TEMPERATURE:
			return "Tmp";
		case SENSOR_TYPE_PROXIMITY:
			return "Prx";
		case SENSOR_TYPE_GRAVITY:
			return "Grv";
		case SENSOR_TYPE_LINEAR_ACCELERATION:
			return "Lac";
		case SENSOR_TYPE_ROTATION_VECTOR:
			return "Rot";
		case SENSOR_TYPE_RELATIVE_HUMIDITY:
			return "Hum";
		case SENSOR_TYPE_AMBIENT_TEMPERATURE:
			return "Tam";
	}
	return "ukn";
}


int sensor_start_collector1()
{
	int err;
	struct sensors_poll_device_t* device;
	struct sensors_module_t* module;
	struct hw_module_t* comm;
	
	err = hw_get_module(SENSORS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
	if (err != 0) {
		printf("[MON] hw_get_module() failed (%s)\n", strerror(-err));
		return 0;
	} else {
		comm = &module->common;
		printf("[MON] hw_get_module():\n");
		printf("[MON] name: %s\n", comm->name);
		printf("[MON] id: %s\n", comm->id);
		printf("[MON] author: %s\n", comm->id);
	}
	
	err = sensors_open(&module->common, &device);
	if (err != 0) {
		printf("sensors_open() failed (%s)\n", strerror(-err));
		return 0;
	}
	
	struct sensor_t const* list;
	int count = module->get_sensors_list(module, &list);
	printf("%d sensors found:\n", count);
	//count = 4;
	for (int i=0 ; i<count ; i++) {
		printf("%2d %s\n"
				"\tvendor: %s\n"
				"\tversion: %d\n"
				"\thandle: %d\n"
				"\ttype: %d\n"
				"\tmaxRange: %f\n"
				"\tresolution: %f\n"
				"\tpower: %f mA\n",
				i,
				list[i].name,
				list[i].vendor,
				list[i].version,
				list[i].handle,
				list[i].type,
				list[i].maxRange,
				list[i].resolution,
				list[i].power);
	}
	
	static const size_t numEvents = 16;
	sensors_event_t buffer[numEvents];
	printf("Try to deactivate %d\n", sizeof(buffer));
	for (int i=0 ; i<count ; i++) {
		err = device->activate(device, list[i].handle, 0);
		if (err != 0) {
			printf("deactivate() for '%s'failed (%s)\n",
					list[i].name, strerror(-err));
			return 0;
		}
	}

	printf("Try to activate\n");
	for (int i=0 ; i<count ; i++) {
		err = device->activate(device, list[i].handle, 1);
		if (err != 0) {
			printf("activate() for '%s'failed (%s)\n",
					list[i].name, strerror(-err));
			return 0;
		}
		device->setDelay(device, list[i].handle, ms2ns(10));
	}
	printf("[MON] Try to poll for collecting sensor data\n");

	do {
		int n = device->poll(device, buffer, numEvents);
		if (n < 0) {
			printf("poll() failed (%s)\n", strerror(-err));
			break;
		}

		printf("read %d events:\n", n);
		for (int i=0 ; i<n ; i++) {
			const sensors_event_t& data = buffer[i];

			if (data.version != sizeof(sensors_event_t)) {
				printf("incorrect event version (version=%d, expected=%d",
						data.version, sizeof(sensors_event_t));
				break;
			}
			int index = -1;
			switch(data.type) {
				case SENSOR_TYPE_ACCELEROMETER:
					index=0;
					break;
				case SENSOR_TYPE_MAGNETIC_FIELD:
					index=1;
					break;
				case SENSOR_TYPE_ORIENTATION:
					index=2;
					break;
				case SENSOR_TYPE_GYROSCOPE:
					index=3;
					break;
				case SENSOR_TYPE_GRAVITY:
					index=4;
					break;
				case SENSOR_TYPE_LINEAR_ACCELERATION:
					index=5;
					break;
				case SENSOR_TYPE_ROTATION_VECTOR:
					index=6;
					break;
					break;
				default:
					break;
			}
			if (data.type > 0 && index < MAX_SENSOR_NUM && index >= 0) {
				sensor_data[index].type = data.type;
				sensor_data[index].timestamp = data.timestamp;
				sensor_data[index].data[0] = (int64_t)(data.data[0] * 1000000.0);
				sensor_data[index].data[1] = (int64_t)(data.data[1] * 1000000.0);
				sensor_data[index].data[2] = (int64_t)(data.data[2] * 1000000.0);

			}
		}
		for (int i = 0; i < MAX_SENSOR_NUM; i++) {
			printf("index=%d, sensor=%s, time=%llu, value=<%lld,%lld,%lld>\n",
					i,
					getSensorName(sensor_data[i].type),
					sensor_data[i].timestamp,
					sensor_data[i].data[0],
					sensor_data[i].data[1],
					sensor_data[i].data[2]);
		}

		sensor_sync_data();
	} while (is_running == 1); // fix that
	
	for (int i=0 ; i<count ; i++) {
		printf("[MON] Try deactive the sensor devices.\n");
		err = device->activate(device, list[i].handle, 0);
		if (err != 0) {
			printf("deactivate() for '%s'failed (%s)\n",
					list[i].name, strerror(-err));
			return 0;
		}
	}

	err = sensors_close(device);
	if (err != 0) {
		printf("sensors_close() failed (%s)\n", strerror(-err));
	}
	return 1;
}
