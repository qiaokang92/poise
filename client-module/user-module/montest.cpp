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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>

#include <sys/socket.h>
#include <linux/netlink.h>


#include <cutils/log.h>

#include <hardware/sensors.h>
#include <utils/Timers.h>


#include "montest.h"
#include "gpscollector.h"
#include "sensorcollector.h"


//static sensor_num = 0;

int target_app_uid = 10022;

static int sync_lock = 0;

static sockaddr_nl s_addr, d_addr;
static struct nlmsghdr *nlh = NULL;
static int sock_fd = 0;
static int dev_fd = 0;

int is_running = 0;
uint8_t *dev_mmap = NULL;

uint32_t cycle = 0;

static int mon_fs_init(void)
{
	dev_fd = open("/dev/monsenor", O_RDWR);
	if(dev_fd < 0) {
		printf("[MON] Open monsenor device error!!!\n");
		return -1;
	}
	printf("[MON] Open monsenor device successfully\n");
	dev_mmap = (unsigned char*)mmap(0, MAX_PAYLOAD, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, 0);

	if(dev_mmap == MAP_FAILED) {
		close(dev_fd);
		dev_fd = -1;
		printf("[MON] Mmap sensor memory failed!!!\n");
		return -1;
	}
	return dev_fd;
}

static void mon_fs_fini(void) 
{
	if(dev_fd > 0) {
		munmap(dev_mmap, MAX_PAYLOAD);
		close(dev_fd);
		dev_fd = -1;
	}
}
static int mon_sync_response_cycle(uint32_t cycle)
{
	int ret = -1;
	if (dev_fd < 0)
		return -1;
	struct comm_msg *msg = (struct comm_msg *)dev_mmap;
	sync_lock = 1;
	msg->type = MSG_CONF_RESPONSE_CYCLE;
	msg->dlen = sizeof(cycle);
	*(uint32_t *)msg->data = cycle;
	ret = fsync(dev_fd);
	printf("[MON] Send %d %d 0x%08x bytes (cycle=%d) to kernel res=%d.\n", 
			ret, msg->dlen, (uint32_t)msg, cycle, ret);
	sync_lock = 0;
	return ret;
}


int mon_sync_message(uint32_t type, uint32_t dlen, uint8_t *data)
{
	if(dev_fd < 0)
		return -1;
	int ret = 0;
	struct comm_msg *msg = (struct comm_msg *)dev_mmap;
	sync_lock = 1;
	msg->type = type;
	msg->dlen = dlen;
	memcpy(msg->data, data, dlen);
	ret = fsync(dev_fd);
	printf("[MON] Syn message %d %d 0x%08x with kernel module res=%d.\n", type, dlen, (uint32_t)msg, ret);
	sync_lock = 0;
	return ret;
}

#if 0
static int mon_nl_init() {
	int listenfd = -1;
	listenfd = socket(PF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
	if(listenfd > 0) {
		printf("[MON] Create test 1 socket successfully.\n");
		close(listenfd);
		listenfd = -1;
	} else {
		printf("[MON] Create test 1 socket failed %d.\n", listenfd);
	}
	listenfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
	if(listenfd > 0) {
		printf("[MON] Create test 2 socket successfully.\n");
		close(listenfd);
		listenfd = -1;
	} else {
		printf("[MON] Create test 2 socket failed %d.\n", listenfd);
	}
	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
	if(sock_fd < 0) {
		printf("[MON] Create netlink socket error.\n");
		return -1;
	}
	printf("[MON] Create netlink socket successfully.\n");
	memset(&s_addr, 0, sizeof(s_addr));
	memset(&d_addr, 0, sizeof(d_addr));
	s_addr.nl_family = AF_NETLINK;
	s_addr.nl_pid    = getpid();

	bind(sock_fd, (struct sockaddr *)&s_addr, sizeof(s_addr));
	return sock_fd;
}
static void mon_nl_fini() {
	if(sock_fd > 0) {
		close(sock_fd);
		sock_fd = -1;
	}
}
static int mon_nl_send(void)
{
	struct iovec iov;
	struct msghdr msg;
	int msg_size = sizeof(sensor_data);
	d_addr.nl_family = AF_NETLINK;
	d_addr.nl_pid = 0; // For linux kernel
	d_addr.nl_groups = 0; // unicast

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msg_size));
	if(nlh == NULL) {
		printf("[MON] Malloc memory for msg error.\n");
		close(sock_fd);
	}
	memset(nlh, 0, NLMSG_SPACE(msg_size));
	nlh->nlmsg_len = msg_size;
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_type = 0x01;
	nlh->nlmsg_flags =  0;

	memcpy(NLMSG_DATA(nlh), (void*)sensor_data, msg_size);

	iov.iov_base = (void*)nlh;
	iov.iov_len  = nlh->nlmsg_len;
	msg.msg_name = (void*)&d_addr;
	msg.msg_namelen = sizeof(d_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen =  1;
	printf("[MON] Sending sensor data to kernel.\n");
	sendmsg(sock_fd, &msg, 0);
	return nlh->nlmsg_len;
}
#endif


void sigint_handler(int signum)
{
	is_running = 0;
	gps_fini();
}


// Output the load of the CPU
void *output_load_thread(void* arg)
{
	uint32_t consumed_time = 0;
	float f1, f2, f3;
	int   i1, i2, i3;
	FILE *loadout = fopen("/sdcard/loadavg.txt", "a+");
	FILE *loadavg = NULL;
	printf("[MON] Enter load reading thread.\n");
	while(is_running) {
		loadavg = fopen("/proc/loadavg", "r");
		if(loadavg) {
			fscanf(loadavg, "%f %f %f %d\/%d %d", &f1, &f2, &f3, &i1, &i2, &i3);
			fclose(loadavg);
		}
		//printf("[MON] %u, %f, %f, %f, %d, %d, %d\n", 
		//		(unsigned int)time(NULL), f1, f2, f3, i1, i2, i3);
		if(loadout) {
			fprintf(loadout, "%u, %f, %f, %f, %d, %d, %d\n", 
					(unsigned int)time(NULL), f1, f2, f3, i1, i2, i3);
		}
		if (consumed_time == 100)
		{
			cycle = 2000;
		} else if (consumed_time == 200) {
			cycle = 1800;
		} else if (consumed_time == 300) {
			cycle = 1600;
		} else if (consumed_time == 400) {
			cycle = 1400;
		} else if (consumed_time == 500) {
			cycle = 1200;
		} else if (consumed_time == 600) {
			cycle = 1000;
		} else if (consumed_time == 700) {
			cycle = 800;
		} else if (consumed_time == 800) {
			cycle = 600;
		} else if (consumed_time == 900) {
			cycle = 400;
		} else if (consumed_time == 1000) {
			cycle = 200;
		} else if (consumed_time == 1100) {
			cycle = 100;
		} else if (consumed_time == 1200) {
			is_running = 0;
		}
		if (cycle > 0) {
			mon_sync_response_cycle(cycle);
			printf("[MON] Schedure cycle=%d\n", cycle);
			if(loadout) fprintf(loadout, "Schedure cycle=%d\n", cycle);
			cycle = 0;
		}
		consumed_time += 10;
		sleep(1);
	}
	if(loadout) fclose(loadout);
	is_running = 0;
	return NULL;
}


int main(int argc, char** argv)
{
	mon_fs_init();
	
	pthread_t th;
	int arg = 10;
	int ret = 0;
	is_running = 1;
	//ret = pthread_create(&th, NULL, output_load_thread, &arg);
	
	signal(SIGINT, sigint_handler);
	
	//gps_start_collector(); // run as a thread

	sensor_start_collector(); // run as the main thread

	while(is_running)
		sleep(1);
	//gps_fini();
	
	mon_fs_fini();
	return 0;
}
