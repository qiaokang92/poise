// mon_nf.h
#ifndef _MON_NF_H
#define _MON_NF_H

#define PAGE_SIZE		4096

struct user_command {
	int MEASURE_MODE;
	unsigned int dest_ip;	//	network byte order
	unsigned int dest_port;	//	network byte order
};

struct kernel_message {
	
	int rtt;
};

struct remote_info {
	unsigned int rem_ip;
	unsigned int rem_port;
};

/* 24 bytes */
struct net_tx_info {
	uint32_t size;  // 4
	uint32_t sip;   // 4
	uint32_t dip;   // 4
	uint16_t sport; // 2
	uint16_t dport; // 2
	uint16_t ipid;  // 2
	uint16_t reserve;  // 2
	uint32_t plen;  // 4
};

//void update_response_cycle(uint32_t *pcycle);
//void update_sensor_data(unsigned char* data, unsigned int len);

//int init_nf_hook(struct user_command *user_cmd);
//int exit_nf_hook(void);

int poise_nf_init();
void poise_nf_exit();

#endif
