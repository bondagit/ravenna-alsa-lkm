//==========================================
// interface.c, required interface for every kernel module
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/types.h>

// netfilter (nf)
#include <linux/skbuff.h>
#include <linux/netfilter_ipv4.h> /*NF_IP_PRE_FIRST*/
#include <linux/netdevice.h>

#include <linux/if_ether.h>


#include <asm-generic/div64.h>


#include "module_main.h"
#include "module_netlink.h"
#include "module_timer.h"


#include <net/ip.h>
#include <net/udp.h>
#include <net/route.h>
#include <net/checksum.h>


//MODULE_LICENSE("Proprietary"); // the license type -- this affects runtime behavior
MODULE_LICENSE("GPL"); // need it for: hrtimer
MODULE_AUTHOR("Merging Technologies"); // visible when you use modinfo
MODULE_DESCRIPTION("Merging Technologies RAVENNA ALSA driver."); // see modinfo
MODULE_VERSION("0.2");
MODULE_SUPPORTED_DEVICE("{{ALSA,Merging Ravenna}}");


static struct nf_hook_ops nf_ho;         // netfilter struct holding set of netfilter hook function options
static int hooked = 0;


/** @brief function to be called by hook
 */
unsigned int nf_hook_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
    int err = 0;
    if (!skb)
    {
        printk(KERN_ALERT "sock buffer null\n");
    	return NF_ACCEPT;
    }

    if (hooked == 0)
    {
        printk(KERN_INFO "nf_hook_func first message received\n");
        hooked = 1;
    }

    ///////// DEBUG
    {
        struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb);    //grab network header using accessor
        if (!ip_header)
        {
            printk(KERN_ALERT "sock header null\n");
            return NF_ACCEPT;
        }
        if (ip_header->protocol==IPPROTO_UDP)
        {
            struct udphdr *udp_header;
            udp_header = (struct udphdr *)skb_transport_header(skb);  //grab transport header
            if (!udp_header)
            {
                printk(KERN_ALERT "sock header null\n");
                return NF_ACCEPT;
            }
            //printk(KERN_INFO "got udp packet len = %hu\n", udp_header->len);     //log we’ve got udp packet to /var/log/messages
            //printk(KERN_INFO "got udp packet len = %u\n", skb->len);     //log we’ve got udp packet to /var/log/messages

            /*TOFILL the RTP stream IP address*/
            //if (*(unsigned char*)&ip_header->daddr == 0xEF/*239.x.y.z*/)
            if (*(unsigned char*)&ip_header->daddr == 0xE0/*224.x.y.z*/)
            {
                unsigned char* data = NULL;
                int data_len = -1;

                if (skb->next || skb->prev)
                {
                    printk(KERN_INFO "Got mutiple packet\n");
                }
                data_len = skb->len - sizeof(struct iphdr) - sizeof(struct udphdr);
                if (data_len < 0)
                {
                    printk(KERN_INFO "User data size problem\n");
                    return NF_ACCEPT;
                }
                data = skb->data + skb->len - data_len;
                //if (*data != 0x80)
                //{
                //    printk(KERN_INFO "Unexpected data (%x)\n", *data);
                //    return NF_ACCEPT;
                //}
            }
        }
    }
    ////////// ~DEBUG

    //if (skb_is_nonlinear(skb))
    //{
    //    printk(KERN_INFO "skb is not linear\n");
    //}

    err = skb_linearize(skb);
    if (err < 0)
	{
        printk(KERN_ALERT "skb_linearize error %d\n", err);
		return NF_DROP;
    }


    switch (nf_rx_packet(skb_mac_header(skb), skb->len + ETH_HLEN, in->name))
    {
        case 0:
            return NF_DROP;
        case 1:
            return NF_ACCEPT;
        default:
            printk(KERN_ALERT "nf_rx_packet unknown return code\n");
    }
    return NF_ACCEPT;
}

void __stack_chk_fail(void)
{

}

static inline uint64_t llu_div(uint64_t numerator, uint64_t denominator)
{
    uint64_t result = numerator;
    do_div(result, denominator);

    // TODO make it work
    if (denominator > (uint32_t)~0)
    {
        //printk(KERN_ERR, "divisor superior to 2^32 (%llu) the result is wrong !", denominator);
    }
    //printk(KERN_INFO "DIV %llu div %llu = %llu (%llu)\n", dividend, divisor, __res);
    return result;
}

static inline int64_t lld_div(int64_t numerator, int64_t denominator)
{
    bool neg = ((numerator < 0) ^ (denominator < 0)) != 0;
    uint64_t num_to_res_u = numerator >= 0 ? (uint64_t)numerator : (uint64_t)-numerator;
    uint64_t denominator_u = denominator >= 0 ? denominator : -denominator;
    int64_t result = 0;

    // TODO make it work
    if (denominator > (uint32_t)~0)
    {
        //printk(KERN_ERR, "signed divisor superior to 2^32 (%lld) the result is wrong !", denominator);
    }

    do_div(num_to_res_u, denominator_u);
    //if (denominator > 0xffffffff)
    //{
    //    printk(KERN_ERR, "divisor superior to 2^32 (%llu) the result is wrong !", divisor);
    //}
    //printk(KERN_INFO "DIV %llu div %llu = %llu (%llu)\n", dividend, divisor, __res);
    if (neg)
    {
        result = -num_to_res_u;
        //printk(KERN_INFO "NEG DIV %lld div %lld = %lld\n", numerator, denominator, result);
    }
    else
    {
        result = num_to_res_u;
        //printk(KERN_INFO "DIV %lld div %lld = %lld\n", numerator, denominator, result);
    }

    /// sign check
    //if (numerator < 0 && denominator > 0 && result > 0)
    //    printk("__aeabi_ldivmod sign issue 1");
    //if (numerator > 0 && denominator < 0 && result > 0)
    //    printk("__aeabi_ldivmod sign issue 2");
    //if (numerator < 0 && denominator < 0 && result < 0)
    //    printk("__aeabi_ldivmod sign issue 3");

    return result;
}

static inline uint64_t llu_mod(uint64_t numerator, uint64_t denominator)
{
    uint64_t rest = 0;
    rest = do_div(numerator, denominator);
    //printk(KERN_INFO "MOD %llu mod %llu = %llu\n", dividend, divisor, __rest);
    return rest;
}

/// on ARM7 (marvell toolset) 32bit the following function have to be define
uint64_t __aeabi_uldivmod(uint64_t numerator, uint64_t denominator)
{
    return llu_div(numerator, denominator);
}
/// on ARM7 (marvell toolset) 32bit the following function have to be define
int64_t __aeabi_ldivmod(int64_t numerator, int64_t denominator)
{
    return lld_div(numerator, denominator);
}

/// on x86 32bit the following function have to be define
uint64_t __udivdi3(uint64_t numerator, uint64_t denominator)
{
    return llu_div(numerator, denominator);
}

/// on x86 32bit the following function have to be define
int64_t __divdi3(int64_t numerator, int64_t denominator)
{
    return lld_div(numerator, denominator);
}

/// on x86 32bit the following function have to be define
uint64_t __umoddi3(uint64_t numerator, uint64_t denominator)
{
    return llu_mod(numerator, denominator);
}

static int __init merging_alsa_mod_init(void)
{
    int err = 0;
	if(err < 0)
        goto _failed;
	nf_hook_fct((void*)nf_hook_func, (void*)&nf_ho);
	err = setup_netlink();
    if(err < 0)
    {
        goto _failed;
    }
	err = init_driver();
	if(err < 0)
	{
        cleanup_netlink();
        goto _failed;
    }
	err = init_clock_timer();
	if(err < 0)
	{
        cleanup_netlink();
        destroy_driver();
        goto _failed;
    }
	printk(KERN_INFO "Merging Ravenna ALSA module installed\n");

	return 0;
_failed:
    printk(KERN_ERR "Merging Ravenna ALSA module installation failed\n");
    return err;
}

static void __exit merging_alsa_mod_exit(void)
{
    kill_clock_timer();
    cleanup_netlink();
    destroy_driver();
	printk(KERN_INFO "Merging Ravenna ALSA module removed\n");
}

module_init(merging_alsa_mod_init);
module_exit(merging_alsa_mod_exit);
