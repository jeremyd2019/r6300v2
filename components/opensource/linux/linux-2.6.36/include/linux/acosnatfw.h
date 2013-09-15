#ifndef __LINUX_ACOSNATFW_H
#define __LINUX_ACOSNATFW_H

/******  Add, Tim Liu, 10/24/2005 *******/
#ifdef __KERNEL__
#include <linux/init.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/net.h>
//#include <linux/if.h>
//#include <linux/wait.h>
//#include <linux/list.h>
#endif

/* Responses from hook functions. */
#define ACOSNAT_DROP        0
#define ACOSNAT_ACCEPT      1
#define ACOSNAT_QUEUE       2

#define ACOSNAT_IP_PRE_ROUTING	0
#define ACOSNAT_IP_POST_ROUTING	4

#ifdef CONFIG_ACOSNAT

extern void acosNat_init(void);

#define ACOSNAT_MAX_HOOKS 5

typedef unsigned int acosNat_hookfn(unsigned int hooknum,
			       struct sk_buff *skb,
			       const struct net_device *in,
			       const struct net_device *out,
			       int (*okfn)(struct sk_buff *));

/* Function to register/unregister hook points. */
int acosNat_register_hook(acosNat_hookfn *reg, unsigned int hooknum);
void acosNat_unregister_hook(unsigned int hooknum);

extern acosNat_hookfn *acosNat_hooks[ACOSNAT_MAX_HOOKS];

int acosNat_hook_slow(int pf, unsigned int hook, struct sk_buff *skb,
		 struct net_device *indev,
		 struct net_device *outdev,
		 int (*okfn)(struct sk_buff *));

int acosNat_IpFragInput(struct sk_buff *skb);
int acosNat_IpFragOutput(struct sk_buff *skb);
    
#define ACOSNAT_HOOK(pf, hook, skb, indev, outdev, okfn)			\
((acosNat_hooks[(hook)] == NULL)					\
 ? NF_HOOK((pf), (hook), (skb), (indev), (outdev), (okfn))						\
 : acosNat_hook_slow((pf), (hook), (skb), (indev), (outdev), (okfn)))

//#ifdef CONFIG_L7FILTER
/* add start, Max Ding, 10/18/2007 */
typedef unsigned int acosL7Filter_hookfn(struct sk_buff *skb, int iDirection);
extern acosL7Filter_hookfn *acosL7Filter_hook_printf;
int acosNat_hook(acosL7Filter_hookfn *pHook);
/* add end, Max Ding, 10/18/2007 */
//#endif

#else /* !CONFIG_ACOSNAT */
#define ACOSNAT_HOOK(pf, hook, skb, indev, outdev, okfn) NF_HOOK((pf), (hook), (skb), (indev), (outdev), (okfn))
#endif /*CONFIG_ACOSNAT*/

#endif /*__LINUX_ACOSNATFW_H*/
