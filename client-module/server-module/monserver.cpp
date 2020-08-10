/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>

#include <arpa/inet.h>
#include <sys/socket.h>


#define PAGE_SIZE 4096


/* The head of the received message */
struct net_tx_info {
	uint32_t size;
	uint32_t sip;
	uint32_t dip;
	uint16_t sport;
	uint16_t dport;
	uint16_t ipid;
	uint16_t reserve;
	uint32_t plen;
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
/* The structure used to store the sensor information */
struct sensor_info {
	uint32_t type;
	uint32_t reverse;
	uint64_t timestamp;
	int64_t  data[3];
};

struct gps_nmea_info {
	uint64_t timestamp;
	uint32_t length;
	uint8_t sentence[116];
};



char const* getSensorName(int type) {
	switch(type) {
		case 1: //SENSOR_TYPE_ACCELEROMETER:
			return "Acc";
		case 2: //SENSOR_TYPE_MAGNETIC_FIELD:
			return "Mag";
		case 3: //SENSOR_TYPE_ORIENTATION:
			return "Ori";
		case 4: //SENSOR_TYPE_GYROSCOPE:
			return "Gyr";
		case 5: //SENSOR_TYPE_LIGHT:
			return "Lux";
		case 6: //SENSOR_TYPE_PRESSURE:
			return "Bar";
		case 7: // SENSOR_TYPE_TEMPERATURE:
			return "Tmp";
		case 8: //SENSOR_TYPE_PROXIMITY:
			return "Prx";
		case 9: //SENSOR_TYPE_GRAVITY:
			return "Grv";
		case 10: //SENSOR_TYPE_LINEAR_ACCELERATION:
			return "Lac";
		case 11: //SENSOR_TYPE_ROTATION_VECTOR:
			return "Rot";
		case 12: //SENSOR_TYPE_RELATIVE_HUMIDITY:
			return "Hum";
		case 13: //SENSOR_TYPE_AMBIENT_TEMPERATURE:
			return "Tam";
	}
	return "ukn";
}

void parse_msg(unsigned char *msg, unsigned int len)
{
	unsigned char* tmp_ptr = msg;
	struct net_tx_info *net_info = NULL;
	struct contextInfo *context_info = NULL;
	if(len < sizeof(struct net_tx_info))
	{
		printf("[MON] Received message is too small!!!\n");
		return;
	}
	net_info = (struct net_tx_info *) msg;
	if (net_info->size > len)
	{
		printf("[MON] Received message dis-match!!!!\n");
		return;
	}
	printf("[MON] Packet info: 0x%08x:%04d -> 0x%08x:%04d 0x%04x %d\n",
			net_info->sip, net_info->sport, net_info->dip, net_info->dport,
			net_info->ipid, net_info->plen);
	tmp_ptr = msg + sizeof(struct net_tx_info);
	context_info = (struct contextInfo *)tmp_ptr;
	printf("[MON] Device serial No.: %s\n", context_info->device_serialno);
	printf("[MON] Location info: Type=%s, %f, %f, %d\n", 
			context_info->location.type == 1 ? "GPS" : "NET",
			(float)context_info->location.longitude / 1000000.0,
			(float)context_info->location.latitude / 1000000.0,
			context_info->location.acc);
	printf("[MON] isScreenActive = %d\n", context_info->is_screen_active);
	printf("[MON] isTargetAppFocused = %d\n", context_info->is_target_focused);
	printf("[MON] isCameraActive = %d\n", context_info->is_camera_active);
}

void parse_msg1(unsigned char *msg, unsigned int len)
{
	unsigned char* tmp_ptr = msg;
	struct net_tx_info *net_info = NULL;
	struct sensor_info *sen_info = NULL;
	if(len < sizeof(struct net_tx_info))
	{
		printf("[MON] Received message is too small!!!\n");
		return;
	}
	net_info = (struct net_tx_info *) msg;
	if (net_info->size > len)
	{
		printf("[MON] Received message dis-match!!!!\n");
		return;
	}
	printf("[MON] Packet info: 0x%08x:%04d -> 0x%08x:%04d 0x%04x %d\n",
			net_info->sip, net_info->sport, net_info->dip, net_info->dport,
			net_info->ipid, net_info->plen);
	tmp_ptr = msg + sizeof(struct net_tx_info);
	struct gps_nmea_info *nmea_info = (struct gps_nmea_info *)tmp_ptr;
	printf("[MON] *** nmea info\n");
	printf("[MON] timestamp:\t%lu\n", nmea_info->timestamp);
	printf("[MON] lengh:    \t%d\n",  nmea_info->length);
	printf("[MON] nmea:     \t%s\n",  nmea_info->sentence);
	tmp_ptr += sizeof(struct gps_nmea_info);
	while(tmp_ptr < msg + net_info->size - sizeof(struct sensor_info)) {
		sen_info = (struct sensor_info *)tmp_ptr;
		printf("sensor=%s, time=%lu, value=<%5.1f,%5.1f,%5.1f>\n",
				getSensorName(sen_info->type),
				sen_info->timestamp,
				(float)sen_info->data[0]/1000000.0,
				(float)sen_info->data[1]/1000000.0,
				(float)sen_info->data[2])/1000000.0;
		tmp_ptr += sizeof(struct sensor_info);
	}

}

int create_server_socket(void)
{
	int listenfd = 0, connfd = 0, rcv_size = 0;
	struct sockaddr_in s_addr;
	unsigned char rcv_msg[PAGE_SIZE];
	time_t ticks;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd == -1) {
		printf("[MON] Create socket error!!!\n");
		return 0;
	}

	memset(&s_addr, 0, sizeof(s_addr));
	//memset(sendbuff, 0, sizeof(sendbff));

	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_port   = htons(8081);

	if(bind(listenfd, (struct sockaddr *)&s_addr, sizeof(s_addr)))
	{
		printf("[MON] Bind failed!!!\n");
		close(listenfd);
		return 0;
	}
	listen(listenfd, 10);
	printf("[MON] Start listening.....\n");
	while(1) {
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
		if(connfd < 0) {
			printf("[MON] Accept socket connection failed!!!\n");
			continue;
		} else {
			printf("[MON] Accept an income connection!!!\n");
		}
		while((rcv_size = recv(connfd, rcv_msg, PAGE_SIZE, 0))) {
			printf("[MON] Received %d bytes from client.\n", rcv_size);
			parse_msg(rcv_msg, rcv_size);
		}
		if(rcv_size == 0) {
			printf("[MON] Client connect closed.\n");
			break;
		} else if(rcv_size == -1){
			printf("[MON] Receive data from client error!!!\n");
			break;
		}
	}
	close(listenfd);
	return 0;
}


int main()
{
	create_server_socket();
	return 0;
}
