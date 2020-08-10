// mon_sk.h
//
#ifndef _MON_SK_H
#define _MON_SK_H


int dev_udp_tx(struct net_device *dev, 
		unsigned char *dmac,
		unsigned int sip, 
		unsigned short sport,
		unsigned int   dip,
		unsigned short dport,
		unsigned char* data, int dsize);


int  mon_co_init(uint32_t dip, uint16_t dport);
int  mon_co_sendmsg(uint8_t *data, uint32_t size);
void mon_co_fini(void);

int poise_reply_server(char *msg, int len);

#endif
