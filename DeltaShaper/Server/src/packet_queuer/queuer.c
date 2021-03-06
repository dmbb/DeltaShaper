#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed by KERN_INFO */
#include <linux/init.h>    /* Needed for the macros */

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>

#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/ip.h>

static struct nf_hook_ops netfilter_ops_in;

struct sk_buff *sock_buff;
struct udphdr *udp_header;
struct tcphdr *tcp_header;
struct iphdr *ip_header;
char dst[16];

unsigned int hook_func(unsigned int hooknum,
  struct sk_buff *skb,
  const struct net_device *in,
  const struct net_device *out,
  int(*okfn)(struct sk_buff*)) {
    

    //Healthy Packet
    sock_buff = skb;
    if(!sock_buff){ 
      return NF_ACCEPT; 
    }

    //Non-ip packets are just accepted
    ip_header = ip_hdr(sock_buff);  
    if (!ip_header) { 
      printk(KERN_INFO "NOT an IP packet\n");
      return NF_ACCEPT; 
    }


    //If destined to veth1
    snprintf(dst, 16, "%pI4", &ip_header->daddr);

    if(!strcmp(dst,"10.10.10.10")){
      return NF_QUEUE;
    }else {
      return NF_ACCEPT;
    }
 
}

static int __init queuer_init(void) {

  printk(KERN_INFO "filter -- bind \n");

  netfilter_ops_in.hook     = hook_func;
  netfilter_ops_in.pf       = NFPROTO_IPV4;
  netfilter_ops_in.hooknum  = NF_INET_LOCAL_OUT;
  netfilter_ops_in.priority = NF_IP_PRI_FIRST;

  nf_register_hook(&netfilter_ops_in);

  return 0;
}

static void __exit queuer_exit(void) {
  printk(KERN_INFO "queuer -- clean up\n");

  nf_unregister_hook(&netfilter_ops_in);
}

module_init(queuer_init);
module_exit(queuer_exit);
