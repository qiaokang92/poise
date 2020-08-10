// mon_nl.c
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>


#include "mon_nl.h"
#include "mon_nf.h"

static struct sock *nl_sk = NULL;
static unsigned char sensor_data[PAGE_SIZE] = {'\0'};

static int mon_nl_recv_msg(struct sk_buff *__skb)
{
	struct skb_buff *skb = skb_get(__skb);
	struct nlmsghdr *rx_nlh = nlmsg_hdr(skb);
	int rx_pid, rx_msg_size, rx_msg_type;
	struct sk_buff *skb_buf;
	unsigned char *rx_data = NULL;
	
	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
	if(rx_nlh->nlmsg_len >= NLMSG_SPACE(0)) {
		rx_pid = rx_nlh->nlmsg_pid;
		rx_data = NLMSG_DATA(rx_nlh);
		rx_msg_size = rx_nlh->nlmsg_len - NLMSG_SPACE(0);
		rx_msg_type = rx_nlh->nlmsg_type;
		switch((rx_msg_type && 0xff00) >> 8) {
			case NL_MSG_SENSOR:
				if (rx_msg_size < PAGE_SIZE) {
					//memcpy(sensor_data, rx_data, rx_msg_size);
					update_sensor_data(rx_data, rx_msg_size);
					printk(KERN_INFO "[MON] Received sensor information.\n");
				}
				break;
			default:
				printk(KERN_INFO "[MON] Received unknown information.\n");
				break;
		}
	}
	return 1;
}
/* from 3.6 */
//static struct netlink_kernel_cfg cfg = {
//	.input = mon_nl_recv_msg,
//};
int mon_nl_init(void) {
	nl_sk = netlink_kernel_create(&init_net, NETLINK_GENERIC, 0, mon_nl_recv_msg, NULL, THIS_MODULE);
	//nl_sk = netlink_kernel_create(&init_net, NETLINK_GENERIC, &cfg);
	if(!nl_sk){
		printk(KERN_ERR "[MON] Create netlink socket error!!!\n");
		return -10;
	}
	return 0;
}

void mon_nl_fini(void) {
	if(nl_sk) {
		netlink_kernel_release(nl_sk);
	}
}
