// mon_co.c
//
//
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/time.h>
#include <linux/netdevice.h>

#include <net/tcp.h>

#include "mon_nf.h"
#include "mon_co.h"

static struct socket  *con_sk = NULL;


int dev_udp_tx(struct net_device *dev, 
		unsigned char *dmac,
		unsigned int sip, 
		unsigned short sport,
		unsigned int   dip,
		unsigned short dport,
		unsigned char* data, int dsize)
{
	struct sk_buff *skb = NULL;
	struct ethhdr  *ethdr = NULL;
	struct iphdr   *iph   = NULL;
	struct udphdr  *udph  = NULL;

	struct timeval ts;
	unsigned long nts;

	unsigned char *pdata = NULL;
	unsigned int skb_len  = dsize + LL_RESERVED_SPACE(dev) + sizeof(struct iphdr) + sizeof(struct udphdr);
	unsigned int ip_len   = dsize + sizeof(struct iphdr) + sizeof(struct udphdr);
	unsigned int udp_len  = dsize + sizeof(struct udphdr);
	skb = alloc_skb(skb_len, GFP_ATOMIC);
	//skb = dev_alloc_skb(skb_len);

	if(!skb) {
		printk(KERN_ERR "[MON] xmit: malloc skb error (%d bytes)!!\n", skb_len);
		return 0;
	}

	/*
		 dip = inet_addr("158.132.255.183");
		 sip = inet_addr("10.10.0.12");
		 sport = htons(25007);
		 dport = htons(25008);*/

	printk(KERN_INFO "[MON] xmit: malloc skb len=%d\n", skb->len);
	prefetchw(skb->data);
	skb_reserve(skb, LL_RESERVED_SPACE(dev));
	skb->dev = dev;
	skb->pkt_type = PACKET_OTHERHOST;
	skb->protocol = __constant_htons(ETH_P_IP);
	skb->ip_summed = CHECKSUM_NONE;
	skb->priority = 0;

	/* Alloc memory for IP header */
	skb_set_network_header(skb, 0);
	skb_put(skb, sizeof(struct iphdr));

	/* Alloc memory for UDP header */
	skb_set_transport_header(skb, sizeof(struct iphdr));
	skb_put(skb, sizeof(struct udphdr));

	/* Construct UDP header */
	udph = udp_hdr(skb);
	udph->source = sport;
	udph->dest   = dport;
	udph->len    = htons(dsize);
	udph->check  = 0;
	/* Construct IP header */
	iph = ip_hdr(skb);
	iph->version = 4;
	iph->ihl = sizeof(struct iphdr) >> 2;
	iph->frag_off = 0;
	iph->protocol = IPPROTO_UDP;
	iph->tos = 0;
	iph->daddr = dip;
	iph->saddr = sip;
	iph->ttl = 0x40;
	iph->tot_len = htons(ip_len);
	iph->id = htons(0x8888);
	iph->check = 0;

	/* Put data into skb */
	pdata = skb_put(skb, dsize);

	if(data){
		memcpy(pdata, data, dsize);
	}

	skb->csum = csum_partial((char*) udph, dsize, 0);
	udph->check = csum_tcpudp_magic(sip, dip,	dsize, IPPROTO_UDP, skb->csum);
	iph->check = ip_fast_csum(iph, iph->ihl);


	ethdr = (struct ethhdr*)skb_push(skb, ETH_HLEN);
	memcpy(ethdr->h_source, dev->dev_addr, ETH_ALEN);
	memcpy(ethdr->h_dest, dmac, ETH_ALEN);
	ethdr->h_proto = __constant_htons(ETH_P_IP);

	if (dev_queue_xmit(skb) < 0) {
		dev_put(dev);
		kfree_skb(skb);
		printk(KERN_ERR "[MON] Send UDP packet failed!!\n");
		return 0;
	}
	do_gettimeofday(&ts);
	nts = timeval_to_ns(&ts);
	printk(KERN_INFO "[MON] %lu Send UDP packet (%d/%d bytes) success.\n", 
			nts, dsize, skb->len);
	return 1;
}


int mon_co_init(uint32_t dip, uint16_t dport)
{
	struct sockaddr_in s_addr;
	int acc, cn;

	con_sk = (struct socket*) kmalloc(sizeof(struct socket), GFP_KERNEL);
	if (!con_sk) {
		printk(KERN_INFO "[MON] Malloc socket struct error!!!\n");
		return 0;
	}
	memset((void*)&s_addr, 0, sizeof(struct sockaddr_in));
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = dip;
	s_addr.sin_port = dport;
	acc = sock_create_kern(AF_INET, SOCK_STREAM, 0, &con_sk);
	if(acc < 0) {
		printk(KERN_ERR "[MON] Create stream socket error.\n");
		kfree(con_sk);
		con_sk = NULL;
		return 0;
	} else {
		printk(KERN_INFO "[MON] Create stream socket successfully.\n");
	}

	//cn = inet_stream_connect(con_sk, (struct sockaddr *)server, sizeof(struct sockaddr_in), 0);
	cn = con_sk->ops->connect(con_sk, (struct sockaddr *)&s_addr, sizeof(s_addr), 0);
	if (cn != 0) { // Try to connect again after 200 ms if it is not exiting.
		printk(KERN_ERR "[MON] Socket conntects to server error!!!\n");
		kernel_sock_shutdown(con_sk, SHUT_RDWR);
		kfree(con_sk);
		con_sk = NULL;
		return 0;
	}
	printk(KERN_INFO "[MON] Create connection cn=%d.\n", cn);

	return 1;
}

int  mon_co_sendmsg(uint8_t *data, uint32_t size)
{ 
	struct msghdr sock_msg;
	struct iovec iov;
	unsigned char *buff = NULL;
	int suc = 0;

	if(con_sk == NULL) {
		printk(KERN_ERR "[MON] There is no connection to the server.");
		if(mon_co_init(in_aton(SERVER_IP_STR), htons(SERVER_PORT)) == 0)
			return 0;
	}

	buff = (unsigned char *) kmalloc(size, GFP_KERNEL);
	if (!buff) {
		printk(KERN_ERR "[MON] Malloc TX buff error!!!");
		return 0;
	}

	memcpy(buff, data, size);
	memset((void *) &iov, 0, sizeof(struct iovec));
	memset((void *) &sock_msg, 0, sizeof(struct msghdr));

	iov.iov_base = (void *) buff; 
	iov.iov_len  = size;

	// sock_msg->msg_flags = 0;
	suc = kernel_sendmsg(con_sk, &sock_msg, &iov, 1, size);
	if(suc < 0) {
		printk(KERN_INFO "[MON] Sendmsg to server failed!!!\n");
		mon_co_fini();
		return 0;
	} else {
		printk(KERN_INFO "[MON] Sendmsg (%d bytes) to server successfully!!!\n", suc);
	}

	return suc;
}

void mon_co_fini(void)
{
	if(con_sk) {
		kernel_sock_shutdown(con_sk, SHUT_RDWR);
		printk(KERN_INFO "[MON] Shutdown kernel socket.\n");
		inet_release(con_sk);
		printk(KERN_INFO "[MON] Release kernel socket.\n");
		//kfree(con_sk);
		con_sk = NULL;
		printk(KERN_INFO "[MON] Free Kernel socket.\n");
	}
}
