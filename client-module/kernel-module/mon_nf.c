// mon_nf.c
// Register netfilter hook to monitor traffic of specific app
//
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/time.h>
#include <linux/in.h>
#include <linux/netdevice.h>
#include <linux/rculist.h>
#include <net/tcp.h>

#include <linux/kthread.h>

#include "mon_poise.h"
#include "mon_nf.h" 
#include "mon_rs.h"
// The attached context info structure

extern struct cxt_info	cxt_location_info;

static unsigned int	cxt_head_len = 16;

// The dest mac of the sent packets
static unsigned long tx_pkt_num = 0;
unsigned char tx_dmac[ETH_ALEN] = {0x0c,0x84,0xdc,0xb2,0x6f,0xd1};
static struct net_device *tx_dev = NULL;
static unsigned long tx_last_ts = 0;
static unsigned int tx_dip, tx_sip, tx_plen;
static unsigned short tx_dport, tx_sport, tx_ipid;
static char tx_buff[1500] = {'\0'};

// Structures utilized to initialize the netfilter hooking information
static struct nf_hook_ops *local_in_hook_op = NULL;
static struct nf_hook_ops *local_out_hook_op = NULL;
static unsigned int  t_uid  = 10059; // AndFTP
// static unsigned int  t_uid  = 10067; // YouToBe

// Pointing to the last received or sent sk_buff
static struct sk_buff *last_tx_skb = NULL;
static struct sk_buff *last_rx_skb = NULL;

// Convert IP address from string format to integer format 
unsigned int inet_addr(char *ip)
{
	int a, b, c, d;
	char addr[4];

	sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d);
	addr[0] = a;
	addr[1] = b;
	addr[2] = c;
	addr[3] = d;

	return *(unsigned int *)addr;
}
// Convert IP address form integer format to string format
char *inet_ntoa(const unsigned int addr, char *buf)
{   
	u_char s1 = (addr & 0xFF000000) >> 24;
	u_char s2 = (addr & 0x00FF0000) >> 16;
	u_char s3 = (addr & 0x0000FF00) >> 8;
	u_char s4 = (addr & 0x000000FF);
	sprintf(buf, "%d.%d.%d.%d", s4, s3, s2, s1);
	return buf;
} 

// Calculate the checksum for the cusmomized packet
uint16_t csum(u_char *addr, int count)
{
	/* Compute Internet Checksum for "count" bytes
	 *	 *         beginning at location "addr".
	 *		 */
	register long sum = 0;

	while(count > 1)  {
		/*  This is the inner loop */
		sum += *((unsigned short *)addr);
		addr += 2;
		count -= 2;
	}

	/*  Add left-over byte, if any */
	if(count > 0)
		sum += *(unsigned char *) addr;

	/*  Fold 32-bit sum to 16 bits */
	while(sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}
struct psd_header {
	unsigned long saddr; // sip
	unsigned long daddr; // dip
	u_char mbz;// 0
	u_char ptcl; // protocol
	unsigned short pl; //TCP lenth

};
uint16_t tcpudp_csum(uint32_t saddr, uint32_t daddr, u_char *pkt_data, uint16_t len, unsigned char proto)
{
	u_char buf[1600], *pkt;
	uint16_t rst;
	struct psd_header *psdh;
	int count = sizeof(struct psd_header) + len;
	memset(buf, 0, count);
	psdh = (struct psd_header *)buf;
	pkt = buf + sizeof(struct psd_header);
	psdh->saddr = saddr;
	psdh->daddr = daddr;
	psdh->mbz = 0;
	psdh->ptcl = proto; //IPPROTO_TCP;
	psdh->pl = htons(len);
	memcpy(pkt, pkt_data, len);
	rst = csum(buf, count);
	return rst;
}


static struct sk_buff* create_cxt_skb(struct sk_buff *skb, int offset, int len)
{
	struct iphdr  *n_iph  = NULL;
	struct tcphdr *n_tcph = NULL;
	unsigned char *data = NULL, *o_data = NULL, *n_data = NULL;
	struct sk_buff* n_skb = NULL;
	int psize=0, trim = 0, dlen = 0;
	char buf1[32], buf2[32];
	n_skb = skb_copy_expand(skb, skb_headroom(skb)+cxt_head_len, skb_tailroom(skb), GFP_ATOMIC);
	if(!n_skb) {
		dbg_err("Copy skbuff error (Line:%d)!!!", __LINE__);
		return NF_ACCEPT;
	} else {
	}
	
	// Fill the IP header
	n_iph = (struct iphdr*)ip_hdr(n_skb);
	if(n_iph) {
		skb_put(n_skb, cxt_head_len);
#if 0 // Add IP options
		n_iph->ihl = ((iph->ihl<<2) + cxt_head_len)>>2;
		data   = (char*)n_iph + (iph->ihl<<2);
		n_data = (char*)n_iph + (n_iph->ihl<<2);
		dlen   = ntohs(iph->tot_len) - (iph->ihl<<2);
		memmove(n_data, data, dlen);
		*((unsigned char*)data) = 0x20;
		data++;
		*((unsigned char*)data) = cxt_head_len;
		data++;
		memset(data, 0xf, cxt_head_len-2);
#endif
		// dbg_log("new skb ip total len: %d %d\n", ntohs(n_iph->tot_len), n_iph->ihl<<2);
	} else {
		dbg_log("Create IP header error (Line:%d)!!!", __LINE__);
		kfree_skb(n_skb);
		return NULL;
	}
	// Fill the TCP header
	n_tcph = (struct tcphdr*)((char*)n_iph + (n_iph->ihl<<2));
	if(n_tcph) {
#if 1 // Add TCP option
		data   = (char*)n_tcph + (n_tcph->doff<<2);
		o_data = data + offset;
		n_data = data + cxt_head_len;
		psize  = ntohs(n_iph->tot_len) - (n_iph->ihl<<2) - (n_tcph->doff<<2);
		if(psize - offset >= len) {
			dlen = len;
		} else {
			dlen = psize - offset;
		}
		trim = psize - len;
		if(dlen > 0) {
			memmove(n_data, o_data, dlen);
			if(offset > 0)
				n_tcph->seq = htonl(ntohl(n_tcph->seq)+offset);
		}
		*((unsigned char*)data++) = 0x20;
		*((unsigned char*)data++) = cxt_head_len;
		memset(data, 0x0f, cxt_head_len-2);	
		n_tcph->doff = ((n_tcph->doff<<2)+cxt_head_len)>>2;
		if(trim > 0) {
			skb_trim(n_skb, n_skb->len-trim);
			n_iph->tot_len = htons(ntohs(n_iph->tot_len)-trim+cxt_head_len);
		} else {
			n_iph->tot_len = htons(ntohs(n_iph->tot_len) + cxt_head_len);
		}
#endif
	} else {
		dbg_log("Create TCP header error (Line:%d)!!!", __LINE__);
		kfree_skb(n_skb);
		return NULL;
	}
	//n_iph->id = htons((unsigned short)0x8888);
	//n_iph->id = htons((unsigned short)offset);
	n_iph->check = 0;
	n_iph->check = ip_fast_csum((unsigned char*)n_iph, n_iph->ihl);
	
	dbg_log("Out(00000): %04d 0x%08x ->%s:%d 0x%04x 0x%04x iph=0x%08x Flag:0x%04x tcphl=%d iplen=%d payload=%d skblen=%d dlen=%d trim=%d\n", 
			tx_pkt_num, ntohl(n_tcph->seq),
			inet_ntoa(n_iph->daddr, buf2), ntohs(n_tcph->dest), 
			ntohs(n_iph->id), ntohs(n_iph->check),
			(unsigned int)n_iph, ntohs(n_iph->frag_off),
			n_tcph->doff<<2, ntohs(n_iph->tot_len), dlen, n_skb->len,
			n_skb->data_len, trim);
	
	return n_skb;
}

// Analyze the Tx sk_buff, invoked in the NF hook functions
static int parse_tx_skb(unsigned int uid, struct sk_buff* skb)
{
	struct iphdr*  iph = NULL;
	struct tcphdr* tcph = NULL;
	struct ethhdr* ethh = NULL;
	unsigned short sport, dport, csum1 = 0, csum2 = 0, csum3 = 0, tmp;
	unsigned int saddr, daddr, seq, ack, psize, l4len, dlen, loptsize;
	unsigned char* data = NULL;
	char buf1[32], buf2[32];
	struct sk_buff* n_skb1 = NULL, *n_skb2 = NULL;
	int res = NF_DROP;

	ethh = (struct ethhdr*)eth_hdr(skb);
	if(!ethh) {
		dbg_err("Failed to get ETH header of the outgoing packet...\n");
		return NF_ACCEPT;
	}
	if(ethh->h_source[0] == 0xff && ethh->h_source[1] == 0x00) {
		return NF_ACCEPT;
	}

	iph = (struct iphdr*) ip_hdr(skb);
	if(!iph) {
		dbg_err("Failed to get IP header of the outgoing packet...\n");
		return NF_ACCEPT;
	}

	if(iph->protocol != IPPROTO_TCP || ntohs(iph->id) == 0x8888) { // TCP packet
		//dbg_log("Out(%d) %s -> %s %d\n", inet_ntoa(saddr, buf1), inet_ntoa(daddr, buf2), iph->protocol);
		return NF_ACCEPT;
	}

	tcph = (struct tcphdr*) tcp_hdr(skb);
	if(!tcph) {
		dbg_err("Failed to get TCP header of the outgoing packet...\n");
		return NF_ACCEPT;
	}
	saddr = iph->saddr;
	daddr = iph->daddr;
	psize = ntohs(iph->tot_len)-(iph->ihl<<2)-(tcph->doff<<2);
	
	l4len = ntohs(iph->tot_len) - iph->ihl*4;
	sport = ntohs(tcph->source);
	dport = ntohs(tcph->dest);
	seq   = ntohl(tcph->seq);
	ack		= ntohl(tcph->ack_seq);
	loptsize = 60-(tcph->doff<<2);

	// csum2 = csum_tcpudp_magic(iph->saddr, iph->daddr, l4len, iph->protocol, csum_partial(tcph, l4len, 0));
	dbg_log("Out(%05d): %04d 0x%08x ->%s:%d 0x%04x 0x%04x iph=0x%08x Flag:0x%04x tcphl=%d iplen=%d payload=%d skblen=%d dlen=%d\n", 
			uid, tx_pkt_num, seq,
			inet_ntoa(daddr, buf2), dport, 
			ntohs(iph->id), ntohs(iph->check),
			(unsigned int)iph, ntohs(iph->frag_off),
			tcph->doff<<2, ntohs(iph->tot_len), psize, skb->len,
			skb->data_len);

	// No room left for CXT options
	if(loptsize < cxt_head_len)
		return NF_ACCEPT;

	

	tx_pkt_num++;	
	if(ntohs(iph->tot_len) + cxt_head_len > 1500) {
		n_skb1 = create_cxt_skb(skb, 0, psize/2);
		n_skb2 = create_cxt_skb(skb, psize/2, psize-psize/2);
		if(!n_skb1 || !n_skb2) {
			if(n_skb1)
				kfree_skb(n_skb1);
			if(n_skb2)
				kfree_skb(n_skb2);
			return NF_ACCEPT;
		} 
		ip_local_out(n_skb1);
		ip_local_out(n_skb2);
	} else {
		n_skb1 = create_cxt_skb(skb, 0, psize);
		if(!n_skb1)
			return NF_ACCEPT;
		ip_local_out(n_skb1);
	}
	
	kfree_skb(skb);
	
	return NF_STOLEN;
	//return NF_DROP;
	//return NF_ACCEPT;
}

// Hook function to deal with the incoming packets
static unsigned int nf_local_in_hook(unsigned int hooknum,
		struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		int(*okfn)(struct sk_buff *))
{
	return NF_ACCEPT;
}

// Hook function to deal with the outgoing skbuff
static unsigned int nf_local_out_hook(unsigned int hooknum,
		struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		int(*okfn)(struct sk_buff *))
{
	const struct file *flip;
	// struct timeval tx_ts;
	unsigned int res = NF_ACCEPT;
	unsigned int uid = 0;
	if(!skb)
		return NF_ACCEPT;
	if (skb->sk) {
		if(skb->sk->sk_socket) {
			flip = skb->sk->sk_socket->file;
			if (flip) {
				uid = flip->f_cred->fsuid.val;
			}
		}
	}
	if(uid == t_uid)
		res = parse_tx_skb(uid, skb);
	else
		res = NF_ACCEPT;
	return res;
}

// Initialize the NETFILTER hook functions
int poise_nf_init(void)
{
	int res = 0;
	dbg_enter_fun();
#if 0
	local_in_hook_op = (struct nf_hook_ops *)kmalloc(sizeof(struct nf_hook_ops), GFP_KERNEL);
	if (!local_in_hook_op) {
		dbg_err("Malloc netfilter hook ops error!!!\n");
	} else {
		memset((void*)local_in_hook_op, 0, sizeof(struct nf_hook_ops));
		local_in_hook_op->hook			= nf_local_in_hook;
		local_in_hook_op->hooknum   = NF_INET_LOCAL_IN;
		local_in_hook_op->pf				= PF_INET;
		local_in_hook_op->priority	= NF_IP_PRI_FIRST;	
		res = nf_register_hook(local_in_hook_op);
		if(res == 0) {
			dbg_log("Successfully registered NF_INET_LOCAL_IN hook function.\n");
		} else {
			dbg_err("Failed to register NF_INET_LOCAL_IN hook function.\n");
		}
	}
#endif

	local_out_hook_op = (struct nf_hook_ops *)kmalloc(sizeof(struct nf_hook_ops), GFP_KERNEL);
	if (!local_out_hook_op) {
		dbg_err("Malloc netfilter hook ops error!!!\n");
	} else {
		memset((void*)local_out_hook_op, 0, sizeof(struct nf_hook_ops));
		local_out_hook_op->hook				= nf_local_out_hook;
		// local_out_hook_op->hooknum		= NF_INET_POST_ROUTING;
		local_out_hook_op->hooknum		= NF_INET_LOCAL_OUT;
		local_out_hook_op->pf					= PF_INET;
		local_out_hook_op->priority		= NF_IP_PRI_FIRST;
		// local_out_hook_op->priority		= NF_IP_PRI_LAST;
		res = nf_register_hook(local_out_hook_op);
		if(res == 0) {
			dbg_log("Successfully registered NF_INET_POST_ROUTING hook function.\n");
		} else {
			dbg_err("Failed to register NF_INET_POSET_ROUTING hook function.\n");
		}
	}
	/* endable network device to log packet's timestamp */
	//net_enable_timestamp();
	dbg_exit_fun();
	return 0;
}

// Unregiter the NETFILTER hook functions
void poise_nf_exit(void)
{
	dbg_enter_fun();
	if(local_in_hook_op) {  // Netfilter incoming packet hook
		nf_unregister_hook(local_in_hook_op);
		kfree(local_in_hook_op);
	}
	if(local_out_hook_op) { // Netfilter outcoming packet hook
		nf_unregister_hook(local_out_hook_op);
		kfree(local_out_hook_op);
	}
	dbg_log("Captured %04d packets in total!\n", tx_pkt_num);
	//net_disable_timestamp();
	dbg_exit_fun();
}
