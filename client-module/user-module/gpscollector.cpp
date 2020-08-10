// gpscollector.cpp

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <hardware/gps.h>

#include "gpscollector.h"
#include "montest.h"

struct GPS_nmea_info nmea_info;

static const GpsInterface*  gGpsInterface = NULL;
static const AGpsInterface* gAGpsInterface = NULL;
static const AGpsRilInterface* gAGpsRilInterface = NULL;

/* Function for obtaining GPS information */
static const GpsInterface* get_gps_interface(void) {
	int err;
	hw_module_t* module;
	const GpsInterface* interface = NULL;
	err = hw_get_module(GPS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
	if (!err) {
		hw_device_t* device;
		err = module->methods->open(module, GPS_HARDWARE_MODULE_ID, &device);
		if (!err) {
			gps_device_t* gps_device = (gps_device_t *)device;
			interface = gps_device->get_gps_interface(gps_device);
		}
	}
	return interface;
}

static const AGpsInterface* get_agps_interface(const GpsInterface *gps)
{
	const AGpsInterface* interface = NULL;
	if (gps) {
		interface = (const AGpsInterface*)gps->get_extension(AGPS_INTERFACE);
	}
	return interface;
}

static const AGpsRilInterface* get_agps_ril_interface(const GpsInterface *gps)
{
	const AGpsRilInterface* interface = NULL;
	if (gps) {
		interface = (const AGpsRilInterface*)gps->get_extension(AGPS_RIL_INTERFACE);
	}
	return interface;
}

static void location_callback(GpsLocation* location)
{
	printf("[MON] *** location\n");
	printf("[MON] flags:\t%d\n", location->flags);
	printf("[MON] lat:  \t%lf\n", location->latitude);
	printf("[MON] long: \t%lf\n", location->longitude);
	printf("[MON] accur:\t%f\n",  location->accuracy);
	printf("[MON] utc:  \t%ld\n", (long)location->timestamp);
}

static void status_callback(GpsStatus* status)
{
	switch (status->status) {
		case GPS_STATUS_NONE:
			printf("[MON] *** no gps\n");
			break;
		case GPS_STATUS_SESSION_BEGIN:
			printf("[MON] *** session begin\n");
			break;
		case GPS_STATUS_SESSION_END:
			printf("[MON] *** session end\n");
			break;
		case GPS_STATUS_ENGINE_ON:
			printf("[MON] *** engine on\n");
			break;
		case GPS_STATUS_ENGINE_OFF:
			printf("[MON] *** engine off\n");
			break;
		default:
			printf("[MON] *** unknown status\n");
	}
}


static void sv_status_callback(GpsSvStatus* sv_info)
{
	printf("[MON] *** sv info\n");
	printf("[MON] num_svs:\t%d\n", sv_info->size);
}

static void nmea_callback(GpsUtcTime timestamp, const char* nmea, int length)
{
	char test_gps_str[] = "$GPRMC,013257.00,A,3129.51829,N,12022.10562,E,0.093,,270813,,,A*7A\r\n";
	if(nmea[5] == 'V' && length == 62) // GPGSV or GLGSV
	{
		printf("[MON] *** nmea info\n");
		printf("[MON] timestamp:\t%ld\n", (long)timestamp);
		printf("[MON] nmea:     \t%s\n", nmea);
		printf("[MON] length:   \t%d\n", length);
	} else {
		length = strlen(test_gps_str);
		timestamp = 179376894285;
		nmea = test_gps_str;
	}
	memset((void *)&nmea_info, 0, sizeof(nmea_info));
	nmea_info.length = length;
	nmea_info.timestamp = (long)timestamp;
	memcpy(nmea_info.sentence, nmea, length);
	mon_sync_message(MSG_SYNC_GPS_INFO, sizeof(nmea_info), (uint8_t *)&nmea_info);
}

static void set_capabilities_callback(uint32_t capabilities)
{
	printf("[MON] *** set capabilities\n");
}

static void acquire_wakelock_callback()
{
	printf("[MON] *** acquire wakelock\n");
}

static void release_wakelock_callback()
{
	printf("[MON] *** release wakelock\n");
}

static pthread_t create_thread_callback(const char* name, void (*start)(void *), void* arg)
{
	pthread_t thread;
	pthread_attr_t attr;
	int err;

	err = pthread_attr_init(&attr);
	err = pthread_create(&thread, &attr, (void*(*)(void*))start, arg);

	return thread;
}

GpsCallbacks callbacks = {
	sizeof(GpsCallbacks),
	location_callback,
	status_callback,
	sv_status_callback,
	nmea_callback,
	set_capabilities_callback,
	acquire_wakelock_callback,
	release_wakelock_callback,
	create_thread_callback,
};

	static void
agps_status_cb(AGpsStatus *status)
{
	switch (status->status) {
		case GPS_REQUEST_AGPS_DATA_CONN:
			printf("[MON] *** data_conn_open\n");
			gAGpsInterface->data_conn_open("internet");
			break;
		case GPS_RELEASE_AGPS_DATA_CONN:
			printf("[MON] *** data_conn_closed\n");
			gAGpsInterface->data_conn_closed();
			break;
	}
}

AGpsCallbacks callbacks2 = {
	agps_status_cb,
	create_thread_callback,
};

	static void
agps_ril_set_id_cb(uint32_t flags)
{
	printf("[MON] *** set_id_cb\n");
	gAGpsRilInterface->set_set_id(AGPS_SETID_TYPE_IMSI, "000000000000000");
}

	static void
agps_ril_refloc_cb(uint32_t flags)
{
	printf("[MON] *** refloc_cb\n");
	AGpsRefLocation location;
	//gAGpsRilInterface->set_ref_location(&location, sizeof(location));
}


AGpsRilCallbacks callbacks3 = {
	agps_ril_set_id_cb,
	agps_ril_refloc_cb,
	create_thread_callback,
};


void gps_fini()
{
	printf("[MON] *** GPS collection finishes.\n");
	if (gGpsInterface) {
		gGpsInterface->stop();
		gGpsInterface->cleanup();
	}
}

int gps_start_collector() 
{
	printf("[MON] Start gps information collection..\n");
	gGpsInterface = get_gps_interface();
	if (gGpsInterface && !gGpsInterface->init(&callbacks)) {
		gAGpsInterface = get_agps_interface(gGpsInterface);
		if (gAGpsInterface) {
			gAGpsInterface->init(&callbacks2);
			gAGpsInterface->set_server(AGPS_TYPE_SUPL, "supl.google.com", 7276);
		}

		gAGpsRilInterface = get_agps_ril_interface(gGpsInterface);
		if (gAGpsRilInterface) {
			gAGpsRilInterface->init(&callbacks3);
		}

		printf("[MON] *** start gps track\n");
		gGpsInterface->delete_aiding_data(GPS_DELETE_ALL);
		gGpsInterface->start();
		gGpsInterface->set_position_mode(GPS_POSITION_MODE_MS_BASED,
				GPS_POSITION_RECURRENCE_PERIODIC,
				1000, 0, 0);
	}
	return 1;
}
/* end of GPS collections */
