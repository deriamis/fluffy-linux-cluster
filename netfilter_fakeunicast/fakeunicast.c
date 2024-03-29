
/*
 * If a packet comes in on a link-layer multicast address, but
 * is a TCP packet, normally the kernel ignores it because that is
 * not generally valid.
 *
 * We want to override this behaviour, as CLUSTERIP does. This very
 * simple kernel module does this.
 *
 * This is a bit of a hack - we modify the skbuff to change the 
 * packet from being a multicast (which it really is) to tell the
 * kernel that it's actually a unicast (which it isn't) - but that's
 * better than having to patch the kernel.
 *
 * Of course we could be cleverer and only override it for selective
 * IP addresses and/or interfaces etc, but we aren't yet- we do it for
 * every single TCP frame that ever comes in. Hopefully this isn't a 
 * problem - I can't think of a reason that it would be.
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mark Robson <markxr@gmail.com>");
MODULE_DESCRIPTION("Netfilter module which pretends that multicast TCP frames are unicast");

/* The work comes in here from netfilter.c. */
static unsigned int
	fakeunicast_hook(unsigned int hook,
           struct sk_buff **pskb,
           const struct net_device *indev,
           const struct net_device *outdev,
           int (*okfn)(struct sk_buff *))
{
  /* Not IP - ignore. */
  struct iphdr *iph;
  if ((*pskb)->protocol != htons(ETH_P_IP)) {
	return NF_ACCEPT;
  }
  /* Work out if it's a TCP frame... */
  /* On later kernels, we really want: const struct iphdr *iph = ip_hdr(*pskb); */
  /* However this may work on earlier : */
  iph = (*pskb)->nh.iph;
  if (iph->protocol == IPPROTO_TCP) {
    /* If it's a multicast TCP frame, pretend it isn't. */
    if ((*pskb)->pkt_type == PACKET_MULTICAST) {
    	(*pskb)->pkt_type = PACKET_HOST;
    }
  }
  return NF_ACCEPT;
}

 static struct nf_hook_ops fakeunicast_ops = {
	.hook = fakeunicast_hook,
	.pf = PF_INET,
	.hooknum = NF_IP_LOCAL_IN,
	.priority = 0
 };

static const char * proc_entry_name = "fakeunicast";

static struct proc_dir_entry *procentry;

/* Borrowed from proc_misc.c */
static int proc_calc_metrics(char *page, char **start, off_t off,
           int count, int *eof, int len)
{
        if (len <= off+count) *eof = 1;
        *start = page + off;
        len -= off;
        if (len>count) len = count;
        if (len<0) len = 0;
        return len;
}

static int read_proc(char *page, char **start, off_t off,
                                 int count, int *eof, void *data)
{
	int len = sprintf(page,"Installed ok\n");
        return proc_calc_metrics(page, start, off, count, eof, len);
}


static int __init init(void)
{
	int ok;
	printk(KERN_NOTICE "fakeunicast module loaded\n");
	ok = nf_register_hook(&fakeunicast_ops);
	procentry = create_proc_read_entry(proc_entry_name,0,NULL,read_proc,NULL);
	return ok;
}

 static void __exit fini(void)
 {
 	if (procentry) {
	        remove_proc_entry(proc_entry_name, NULL);
	}
        nf_unregister_hook(&fakeunicast_ops);
 }

 module_init(init);
 module_exit(fini);

