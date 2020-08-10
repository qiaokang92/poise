// mon_sk.c
// Socket for cummunicating with the control server
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

#include "mon_sk.h"
#include "mon_poise.h"

#define MAX_PKT_SIZE	1400
#define MAX_BUF_SIZE	4096

static struct socket  *stream_sock = NULL;
static struct socket  *con_sk = NULL;

static int mon_sk_udp_skbuf()
{
	return 0;
}

static int mon_sk_tcp_skbuf()
{
	return 0;
}

int mon_sk_tx(struct net_device *dev, 
		unsigned short sport,
		unsigned int	 sip,
		unsigned short dport,
		unsigned int   dip,
		unsigned char *buf, 
		int size)
{
	struct sk_buff *skb   = NULL;
	struct ethhdr  *ethdr = NULL;
	struct iphdr   *iph   = NULL;
	struct udphdr  *udph  = NULL;

	int l_skb = 0;
	int l_ip  = 0;
	int l_udp = 0;
	
	int left_size = size;
	int pkt_size = 0;
	unsigned char *pload = NULL;

	while(left_size > 0) {
		if(left_size > MAX_PKT_SIZE)
			pkt_size = MAX_PKT_SIZE;
		else
			pkt_size = left_size;

		l_skb = pkt_size + LL_RESERVED_SPACE(dev) + sizeof(struct iphdr) + sizeof(struct udphdr);
		l_ip  = pkt_size + sizeof(struct iphdr) + sizeof(struct udphdr);
		l_udp = pkt_size + sizeof(struct udphdr);

		// Initialize sk_buf structure
		skb   = alloc_skb(l_skb, GFP_ATOMIC); // Malloc a sk_buf structure
		if(skb == NULL) {
			printk(KERN_ERR "[POISE]: Failed to malloc skb..\n");
			break;
		} else {
			dbg_log("Successfully malloc skb buffer.\n");
		}
		prefetchw(skb->data);
		skb_reserve(skb, LL_RESERVED_SPACE(dev));
		skb->dev = dev;
		skb->pkt_type = PACKET_OTHERHOST;
		skb->protocol = __constant_htons(ETH_P_IP);
		skb->ip_summed = CHECKSUM_NONE;
		skb->priority = 0;
	
		// Alloc memory for IP header 
		skb_set_network_header(skb, 0);
		skb_put(skb, sizeof(struct iphdr));
		
		// Alloc memory for UDP header
		skb_set_transport_header(skb, sizeof(struct iphdr));
		skb_put(skb, sizeof(struct udphdr));
		
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
		iph->tot_len = htons(l_ip);
		iph->id = htons(0x8888);
		
		// Construct UDP header
		udph = udp_hdr(skb);
		udph->source = sport;
		udph->dest   = dport;
		udph->len    = htons(pkt_size);
		udph->check  = 0;
		
		/* Put data into skb */
		pload = skb_put(skb, pkt_size);
		if(buf){
			memcpy(pload, buf, pkt_size);
		}
		left_size -= pkt_size;
	}
	return 0;
}

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
		printk(KERN_ERR "[POISE] xmit: malloc skb error (%d bytes)!!\n", skb_len);
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
		printk(KERN_ERR "[POISE]: Send UDP packet failed!!\n");
		return 0;
	}
	do_gettimeofday(&ts);
	nts = timeval_to_ns(&ts);
	printk(KERN_INFO "[POISE]: %lu Send UDP packet (%d/%d bytes) success.\n", 
			nts, dsize, skb->len);
	return 1;
}

/*
 * Try to build a socket connecting to the destination
 */
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
		printk(KERN_ERR "[POISE]: Create stream socket error.\n");
		kfree(con_sk);
		con_sk = NULL;
		return 0;
	} else {
		printk(KERN_INFO "[POISE]: Create stream socket successfully.\n");
	}
	
	//cn = inet_stream_connect(con_sk, (struct sockaddr *)server, sizeof(struct sockaddr_in), 0);
	cn = con_sk->ops->connect(con_sk, (struct sockaddr *)&s_addr, sizeof(s_addr), 0);
	if (cn != 0) {
		printk(KERN_ERR "[POISE]: Socket conntects to server error!!!\n");
		kernel_sock_shutdown(con_sk, SHUT_RDWR);
		kfree(con_sk);
		con_sk = NULL;
		return 0;
	}
	printk(KERN_INFO "[POISE]: Create connection cn=%d.\n", cn);

	return 1;
}

/*
 * Send a message to the target
 */
int mon_co_sendmsg(uint8_t *data, uint32_t size)
{ 
	struct msghdr sock_msg;
	struct iovec iov;
	unsigned char *buff = NULL;
	int suc = 0;

	buff = (unsigned char *) kmalloc(size, GFP_KERNEL);
	if (!buff) {
		printk(KERN_ERR "[POISE]: Malloc TX buff error!!!");
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
		return 0;
	} else {
		printk(KERN_INFO "[MON] Sendmsg (%d bytes) to server successfully!!!\n", suc);
	}

	return suc;
}


/*
 * Close the socket and finish the connection
 */ 
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




/****************************************************************************/
#if 0
static u32 create_address(u8 *ip) 
{
	u32 addr = 0;
	int i;
	for(i = 0; i < 4; i++) {
		addr += ip[i];
		if(i==3)
			break;
		addr <= 8;
	}
	return addr;
}
#endif

/*
 * Read Tx data from the specified stream socket
 */
static int stream_msg_receive(struct socket *sock, char *str, unsigned long flags)
{
	struct msghdr msg;
	struct kvec vec;
	int len = 0;
	int max_size = 50;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags   = flags;
	vec.iov_len  = max_size;
	vec.iov_base = str;

read_again:
	len = kernel_recvmsg(sock, &msg, &vec, max_size, max_size, flags);
	if(len == -EAGAIN || len == -ERESTARTSYS) {
		printk(KERN_ERR "[POISE]: Error while reading from stream socket: %d\n", len);
		goto read_again;
	}
	dbg_log("Data from server: %s\n", str);
	return len;
}

/*
 * Trainsmit data to the server using the specified stream socket
 */
static int stream_msg_send(struct socket *sock, const char *buf, const size_t length, unsigned long flags)
{
	struct msghdr msg;
	struct kvec vec;
	int len, written = 0, left = length;
	mm_segment_t oldmm;

	// Create the message header for Tx data
	msg.msg_name		= 0;
	msg.msg_namelen = 0;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags      = flags;

	// Get current memory address limition
	oldmm = get_fs();
	// Set kernel memory address limition
	set_fs(KERNEL_DS);

repeat_send:
	vec.iov_len		= left; // The length of the data to be sent 
	vec.iov_base  = (char *)buf + written; // Base address of the data to be sent

	// Send message to target
	len = kernel_sendmsg(sock, &msg, &vec, left, left);
	if((len == -ERESTARTSYS) || (!(flags & MSG_DONTWAIT) && (len == -EAGAIN)))
		goto repeat_send;
	if(len > 0) {
		written += len;
		left -= len;
		if(left)
			goto repeat_send;
	}
	set_fs(oldmm); // Recover the original message address limition
	return written ? written : len;
}


/*
 * Build TCP stream connection to the control server
 */
int poise_reply_server(char *msg, int len)
{
	struct sockaddr_in saddr;
	char response[MAX_BUF_SIZE];
	int ret = -1;

	DECLARE_WAIT_QUEUE_HEAD(recv_wait);

	// Create a kernel stream socket
	ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &stream_sock);
	if(ret < 0) {
		printk(KERN_ERR "[POISE]: TCP stream socket creation error (%d)\n", ret);
		goto err;
	}

	// Create the server address structure
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port   = htons(12345);
	saddr.sin_addr.s_addr = in_aton("10.42.0.1");

	// Connect to the server 
	ret = stream_sock->ops->connect(stream_sock, (struct sockaddr *)&saddr, sizeof(saddr), O_RDWR);
	if(ret && (ret != -EINPROGRESS)) {
		printk(KERN_ERR "[POISE]: TCP stream connection error (%d)\n", ret);
		goto err;
	}

	// Try to send data to server
	stream_msg_send(stream_sock, msg, len, MSG_DONTWAIT);
	
	// Wait for response from server
	wait_event_timeout(recv_wait, !skb_queue_empty(&stream_sock->sk->sk_receive_queue), 5*HZ);
	// Read the reponse from the server in the Rx message queue
	if(!skb_queue_empty(&stream_sock->sk->sk_receive_queue)) {
		memset(&response, 0, len+1);
		stream_msg_receive(stream_sock, response, MSG_DONTWAIT);
	}

err:
	if(stream_sock != NULL) {
		sock_release(stream_sock);
		return 0;
	}
	return -1;
}
