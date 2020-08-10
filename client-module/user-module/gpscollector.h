// gpscollector.h
#ifndef __GPS_COLLECTOR_H
#define __GPS_COLLECTOR_H


/* 128 bytes in total */
struct GPS_nmea_info {
	uint64_t timestamp;
	uint32_t length;
	uint8_t  sentence[116];
};

struct GPS_location_info {

};

void gps_fini();
int  gps_start_collector();
#endif
