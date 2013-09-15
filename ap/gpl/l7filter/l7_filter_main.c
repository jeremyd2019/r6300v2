/* --------------------------------------------------------------------------------------------------------------
 * FILE NAME       l7_filter_main.c (for Linux Platform)
 * DATE            10/25/2007
 * AUTHOR/S        Max Ding
 * Description     Layer 7 filter
 * --------------------------------------------------------------------------------------------------------------
 */

//MODULE_AUTHOR("Matthew Strait <quadong@users.sf.net>, Ethan Sommer <sommere@users.sf.net>");
//MODULE_LICENSE("GPL");
//MODULE_DESCRIPTION("application layer match module");

/*******include start*******/
#ifdef __LINUX__
#include <linux/module.h>
#include <linux/if.h>
#include <linux/types.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/skbuff.h>
#include <linux/acosnatfw.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/mm.h>

#ifdef CONFIG_NETFILTER
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#endif /* CONFIG_NETFILTER */

#endif /* __LINUX__ */

/*#define L7_DEBUG_ON */

#include "regexp/regexp.c"
#include "l7_filter_main.h"

/*******define start*******/
#ifdef L7_DEBUG_ON
#define L7_DEBUG_PRI_0 0x00000000
#define L7_DEBUG_PRI_1 0x00000001
#define L7_DEBUG_PRI_2 0x00000002
#define L7_DEBUG_PRI_3 0x00000004
#define L7_DEBUG_PRI_4 0x00000008
#define L7_DEBUG_PRI_5 0x00000010
#define L7_DEBUG_PRI_6 0x00000020
#define L7_DEBUG_PRI_7 0x00000040
#define L7_DEBUG_PRI_a 0xffffffff
#define L7_PRINTK_DEBUG printk
#define L7_PRINTK_DEBUG_1 if (debug_pri & L7_DEBUG_PRI_1) printk
#define L7_PRINTK_DEBUG_2 if (debug_pri & L7_DEBUG_PRI_2) printk
#define L7_PRINTK_DEBUG_3 if (debug_pri & L7_DEBUG_PRI_3) printk
#define L7_PRINTK_DEBUG_4 if (debug_pri & L7_DEBUG_PRI_4) printk
#define L7_PRINTK_DEBUG_5 if (debug_pri & L7_DEBUG_PRI_5) printk
#define L7_PRINTK_DEBUG_6 if (debug_pri & L7_DEBUG_PRI_6) printk
#define L7_PRINTK_DEBUG_7 if (debug_pri & L7_DEBUG_PRI_7) printk
    
#else/* else for #ifdef L7_DEBUG_ON */

#define L7_PRINTK_DEBUG printk
#define L7_PRINTK_DEBUG_1 printk
#define L7_PRINTK_DEBUG_2 printk
#define L7_PRINTK_DEBUG_3 printk
#define L7_PRINTK_DEBUG_4 printk
#define L7_PRINTK_DEBUG_5 printk
#define L7_PRINTK_DEBUG_6 printk
#define L7_PRINTK_DEBUG_7 printk
#endif/* end for #ifdef L7_DEBUG_ON */

/*******global and static variables start*******/
#ifdef L7_DEBUG_ON
static int debug_pri = 0x00000001;
MODULE_PARM(debug_pri, "i");
#endif

/* Number of packets whose data we look at. */
static int num_packets = 20;

static char tolower(char c);

/* head of the link list of patterns */
static s_pattern_cache * first_pattern_cache = NULL;

#ifdef CONFIG_NETFILTER
int l7_filter_main(struct sk_buff *skb, int iDirection);
static unsigned int l7_filter_hook(unsigned int hook,
                                    struct sk_buff **skb,
                                    const struct net_device *in,
                                    const struct net_device *out,
                                    int (*okfn)(struct sk_buff *)) 
{
    int dir;

    if (hook == NF_IP_PRE_ROUTING) 
        dir = 0;
    else if (hook == NF_IP_POST_ROUTING)
        dir = 1;
    else
        return NF_ACCEPT;

    local_bh_disable();
    l7_filter_main(*skb, dir);
    local_bh_enable();

    return NF_ACCEPT;
}

static struct nf_hook_ops l7_filter_prerouting_ops = {
#ifdef AG_LINUX_26
    .owner          = THIS_MODULE,
#else
    .list           = {NULL, NULL},
#endif /* AG_LINUX_26 */
    .hook           = l7_filter_hook,
    .pf             = PF_INET,
    .hooknum        = NF_IP_PRE_ROUTING,
    .priority       = NF_IP_PRI_LAST,
};
static struct nf_hook_ops l7_filter_postrouting_ops = {
#ifdef AG_LINUX_26
    .owner          = THIS_MODULE,
#else
    .list           = {NULL, NULL},
#endif /* AG_LINUX_26 */
    .hook           = l7_filter_hook,
    .pf             = PF_INET,
    .hooknum        = NF_IP_POST_ROUTING,
    .priority       = NF_IP_PRI_LAST,
};
#endif /* CONFIG_NETFILTER */

/* Hard code for all expression of protocol. It's better to read them from *.pat files. */
static s_proto_regexp g_all_proto[] =
{
    {"bittorrent", L7_ENUM_BITTORRENT, 0, "^(\\x13bittorrent protocol|azver\\x01$|get /scrape\\?info_hash=)|d1:ad2:id20:|\\x08'7P\\)[RP]"},
    {"fasttrack", L7_ENUM_FASTTRACK, 0, "^get (/.download/[ -~]*|/.supernode[ -~]|/.status[ -~]|/.network[ -~]*|/.files|/.hash=[0-9a-f]*/[ -~]*) http/1.1|user-agent: kazaa|x-kazaa(-username|-network|-ip|-supernodeip|-xferid|-xferuid|tag)|^give [0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]?[0-9]?[0-9]?"},
    {"edonkey", L7_ENUM_EDONKEY, 0, "^[\\xc5\\xd4\\xe3-\\xe5].?.?.?.?([\\x01\\x02\\x05\\x14\\x15\\x16\\x18\\x19\\x1a\\x1b\\x1c\\x20\\x21\\x32\\x33\\x34\\x35\\x36\\x38\\x40\\x41\\x42\\x43\\x46\\x47\\x48\\x49\\x4a\\x4b\\x4c\\x4d\\x4e\\x4f\\x50\\x51\\x52\\x53\\x54\\x55\\x56\\x57\\x58[\\x60\\x81\\x82\\x90\\x91\\x93\\x96\\x97\\x98\\x99\\x9a\\x9b\\x9c\\x9e\\xa0\\xa1\\xa2\\xa3\\xa4]|\\x59................?[ -~]|\\x96....$)"},
    {"gnutella", L7_ENUM_GNUTELLA, 0, "^(gnd[\\x01\\x02]?.?.?\\x01|gnutella connect/[012]\\.[0-9]\\x0d\\x0a|get /uri-res/n2r\\?urn:sha1:|get /.*user-agent: (gtk-gnutella|bearshare|mactella|gnucleus|gnotella|limewire|imesh)|get /.*content-type: application/x-gnutella-packets|giv [0-9]*:[0-9a-f]*/|queue [0-9a-f]* [1-9][0-9]?[0-9]?\\.[1-9][0-9]?[0-9]?\\.[1-9][0-9]?[0-9]?\\.[1-9][0-9]?[0-9]?:[1-9][0-9]?[0-9]?[0-9]?|gnutella.*content-type: application/x-gnutella|...................?lime)"},
    {"skypetoskype", L7_ENUM_SKYPETOSKYPE, 0, "^..\\x02............."},
    {"skypeout", L7_ENUM_SKYPEOUT, 0, "^(\\x01.?.?.?.?.?.?.?.?\\x01|\\x02.?.?.?.?.?.?.?.?\\x02|\\x03.?.?.?.?.?.?.?.?\\x03|\\x04.?.?.?.?.?.?.?.?\\x04|\\x05.?.?.?.?.?.?.?.?\\x05|\\x06.?.?.?.?.?.?.?.?\\x06|\\x07.?.?.?.?.?.?.?.?\\x07|\\x08.?.?.?.?.?.?.?.?\\x08|\\x09.?.?.?.?.?.?.?.?\\x09|\\x0a.?.?.?.?.?.?.?.?\\x0a|\\x0b.?.?.?.?.?.?.?.?\\x0b|\\x0c.?.?.?.?.?.?.?.?\\x0c|\\x0d.?.?.?.?.?.?.?.?\\x0d|\\x0e.?.?.?.?.?.?.?.?\\x0e|\\x0f.?.?.?.?.?.?.?.?\\x0f|\\x10.?.?.?.?.?.?.?.?\\x10|\\x11.?.?.?.?.?.?.?.?\\x11|\\x12.?.?.?.?.?.?.?.?\\x12\
|\\x13.?.?.?.?.?.?.?.?\\x13|\\x14.?.?.?.?.?.?.?.?\\x14|\\x15.?.?.?.?.?.?.?.?\\x15|\\x16.?.?.?.?.?.?.?.?\\x16|\\x17.?.?.?.?.?.?.?.?\\x17|\\x18.?.?.?.?.?.?.?.?\\x18|\\x19.?.?.?.?.?.?.?.?\\x19|\\x1a.?.?.?.?.?.?.?.?\\x1a|\\x1b.?.?.?.?.?.?.?.?\\x1b|\\x1c.?.?.?.?.?.?.?.?\\x1c|\\x1d.?.?.?.?.?.?.?.?\\x1d|\\x1e.?.?.?.?.?.?.?.?\\x1e|\\x1f.?.?.?.?.?.?.?.?\\x1f|\\x20.?.?.?.?.?.?.?.?\\x20|\\x21.?.?.?.?.?.?.?.?\\x21|\\x22.?.?.?.?.?.?.?.?\\x22|\\x23.?.?.?.?.?.?.?.?\\x23|\\$.?.?.?.?.?.?.?.?\\$|\\x25.?.?.?.?.?.?.?.?\\x25\
|\\x26.?.?.?.?.?.?.?.?\\x26|\\x27.?.?.?.?.?.?.?.?\\x27|\\(.?.?.?.?.?.?.?.?\\(|\\).?.?.?.?.?.?.?.?\\)|\\*.?.?.?.?.?.?.?.?\\*|\\+.?.?.?.?.?.?.?.?\\+|\\x2c.?.?.?.?.?.?.?.?\\x2c|\\x2d.?.?.?.?.?.?.?.?\\x2d|\\..?.?.?.?.?.?.?.?\\.|\\x2f.?.?.?.?.?.?.?.?\\x2f|\\x30.?.?.?.?.?.?.?.?\\x30|\\x31.?.?.?.?.?.?.?.?\\x31|\\x32.?.?.?.?.?.?.?.?\\x32|\\x33.?.?.?.?.?.?.?.?\\x33|\\x34.?.?.?.?.?.?.?.?\\x34|\\x35.?.?.?.?.?.?.?.?\\x35|\\x36.?.?.?.?.?.?.?.?\\x36|\\x37.?.?.?.?.?.?.?.?\\x37|\\x38.?.?.?.?.?.?.?.?\\x38\
|\\x39.?.?.?.?.?.?.?.?\\x39|\\x3a.?.?.?.?.?.?.?.?\\x3a|\\x3b.?.?.?.?.?.?.?.?\\x3b|\\x3c.?.?.?.?.?.?.?.?\\x3c|\\x3d.?.?.?.?.?.?.?.?\\x3d|\\x3e.?.?.?.?.?.?.?.?\\x3e|\\?.?.?.?.?.?.?.?.?\\?|\\x40.?.?.?.?.?.?.?.?\\x40|\\x41.?.?.?.?.?.?.?.?\\x41|\\x42.?.?.?.?.?.?.?.?\\x42|\\x43.?.?.?.?.?.?.?.?\\x43|\\x44.?.?.?.?.?.?.?.?\\x44|\\x45.?.?.?.?.?.?.?.?\\x45|\\x46.?.?.?.?.?.?.?.?\\x46|\\x47.?.?.?.?.?.?.?.?\\x47|\\x48.?.?.?.?.?.?.?.?\\x48|\\x49.?.?.?.?.?.?.?.?\\x49|\\x4a.?.?.?.?.?.?.?.?\\x4a|\\x4b.?.?.?.?.?.?.?.?\\x4b\
|\\x4c.?.?.?.?.?.?.?.?\\x4c|\\x4d.?.?.?.?.?.?.?.?\\x4d|\\x4e.?.?.?.?.?.?.?.?\\x4e|\\x4f.?.?.?.?.?.?.?.?\\x4f|\\x50.?.?.?.?.?.?.?.?\\x50|\\x51.?.?.?.?.?.?.?.?\\x51|\\x52.?.?.?.?.?.?.?.?\\x52|\\x53.?.?.?.?.?.?.?.?\\x53|\\x54.?.?.?.?.?.?.?.?\\x54|\\x55.?.?.?.?.?.?.?.?\\x55|\\x56.?.?.?.?.?.?.?.?\\x56|\\x57.?.?.?.?.?.?.?.?\\x57|\\x58.?.?.?.?.?.?.?.?\\x58|\\x59.?.?.?.?.?.?.?.?\\x59|\\x5a.?.?.?.?.?.?.?.?\\x5a|\\[.?.?.?.?.?.?.?.?\\[|\\\\.?.?.?.?.?.?.?.?\\\\|\\].?.?.?.?.?.?.?.?\\]|\\^.?.?.?.?.?.?.?.?\\^\
|\\x5f.?.?.?.?.?.?.?.?\\x5f|\\x60.?.?.?.?.?.?.?.?\\x60|\\x61.?.?.?.?.?.?.?.?\\x61|\\x62.?.?.?.?.?.?.?.?\\x62|\\x63.?.?.?.?.?.?.?.?\\x63|\\x64.?.?.?.?.?.?.?.?\\x64|\\x65.?.?.?.?.?.?.?.?\\x65|\\x66.?.?.?.?.?.?.?.?\\x66|\\x67.?.?.?.?.?.?.?.?\\x67|\\x68.?.?.?.?.?.?.?.?\\x68|\\x69.?.?.?.?.?.?.?.?\\x69|\\x6a.?.?.?.?.?.?.?.?\\x6a|\\x6b.?.?.?.?.?.?.?.?\\x6b|\\x6c.?.?.?.?.?.?.?.?\\x6c|\\x6d.?.?.?.?.?.?.?.?\\x6d|\\x6e.?.?.?.?.?.?.?.?\\x6e|\\x6f.?.?.?.?.?.?.?.?\\x6f|\\x70.?.?.?.?.?.?.?.?\\x70|\\x71.?.?.?.?.?.?.?.?\\x71\
|\\x72.?.?.?.?.?.?.?.?\\x72|\\x73.?.?.?.?.?.?.?.?\\x73|\\x74.?.?.?.?.?.?.?.?\\x74|\\x75.?.?.?.?.?.?.?.?\\x75|\\x76.?.?.?.?.?.?.?.?\\x76|\\x77.?.?.?.?.?.?.?.?\\x77|\\x78.?.?.?.?.?.?.?.?\\x78|\\x79.?.?.?.?.?.?.?.?\\x79|\\x7a.?.?.?.?.?.?.?.?\\x7a|\\{.?.?.?.?.?.?.?.?\\{|\\|.?.?.?.?.?.?.?.?\\||\\}.?.?.?.?.?.?.?.?\\}|\\x7e.?.?.?.?.?.?.?.?\\x7e|\\x7f.?.?.?.?.?.?.?.?\\x7f|\\x80.?.?.?.?.?.?.?.?\\x80|\\x81.?.?.?.?.?.?.?.?\\x81|\\x82.?.?.?.?.?.?.?.?\\x82|\\x83.?.?.?.?.?.?.?.?\\x83|\\x84.?.?.?.?.?.?.?.?\\x84|\\x85.?.?.?.?.?.?.?.?\\x85\
|\\x86.?.?.?.?.?.?.?.?\\x86|\\x87.?.?.?.?.?.?.?.?\\x87|\\x88.?.?.?.?.?.?.?.?\\x88|\\x89.?.?.?.?.?.?.?.?\\x89|\\x8a.?.?.?.?.?.?.?.?\\x8a|\\x8b.?.?.?.?.?.?.?.?\\x8b|\\x8c.?.?.?.?.?.?.?.?\\x8c|\\x8d.?.?.?.?.?.?.?.?\\x8d|\\x8e.?.?.?.?.?.?.?.?\\x8e|\\x8f.?.?.?.?.?.?.?.?\\x8f|\\x90.?.?.?.?.?.?.?.?\\x90|\\x91.?.?.?.?.?.?.?.?\\x91|\\x92.?.?.?.?.?.?.?.?\\x92|\\x93.?.?.?.?.?.?.?.?\\x93|\\x94.?.?.?.?.?.?.?.?\\x94|\\x95.?.?.?.?.?.?.?.?\\x95|\\x96.?.?.?.?.?.?.?.?\\x96|\\x97.?.?.?.?.?.?.?.?\\x97|\\x98.?.?.?.?.?.?.?.?\\x98|\\x99.?.?.?.?.?.?.?.?\\x99\
|\\x9a.?.?.?.?.?.?.?.?\\x9a|\\x9b.?.?.?.?.?.?.?.?\\x9b|\\x9c.?.?.?.?.?.?.?.?\\x9c|\\x9d.?.?.?.?.?.?.?.?\\x9d|\\x9e.?.?.?.?.?.?.?.?\\x9e|\\x9f.?.?.?.?.?.?.?.?\\x9f|\\xa0.?.?.?.?.?.?.?.?\\xa0|\\xa1.?.?.?.?.?.?.?.?\\xa1|\\xa2.?.?.?.?.?.?.?.?\\xa2|\\xa3.?.?.?.?.?.?.?.?\\xa3|\\xa4.?.?.?.?.?.?.?.?\\xa4|\\xa5.?.?.?.?.?.?.?.?\\xa5|\\xa6.?.?.?.?.?.?.?.?\\xa6|\\xa7.?.?.?.?.?.?.?.?\\xa7|\\xa8.?.?.?.?.?.?.?.?\\xa8|\\xa9.?.?.?.?.?.?.?.?\\xa9|\\xaa.?.?.?.?.?.?.?.?\\xaa|\\xab.?.?.?.?.?.?.?.?\\xab|\\xac.?.?.?.?.?.?.?.?\\xac|\\xad.?.?.?.?.?.?.?.?\\xad\
|\\xae.?.?.?.?.?.?.?.?\\xae|\\xaf.?.?.?.?.?.?.?.?\\xaf|\\xb0.?.?.?.?.?.?.?.?\\xb0|\\xb1.?.?.?.?.?.?.?.?\\xb1|\\xb2.?.?.?.?.?.?.?.?\\xb2|\\xb3.?.?.?.?.?.?.?.?\\xb3|\\xb4.?.?.?.?.?.?.?.?\\xb4|\\xb5.?.?.?.?.?.?.?.?\\xb5|\\xb6.?.?.?.?.?.?.?.?\\xb6|\\xb7.?.?.?.?.?.?.?.?\\xb7|\\xb8.?.?.?.?.?.?.?.?\\xb8|\\xb9.?.?.?.?.?.?.?.?\\xb9|\\xba.?.?.?.?.?.?.?.?\\xba|\\xbb.?.?.?.?.?.?.?.?\\xbb|\\xbc.?.?.?.?.?.?.?.?\\xbc|\\xbd.?.?.?.?.?.?.?.?\\xbd|\\xbe.?.?.?.?.?.?.?.?\\xbe|\\xbf.?.?.?.?.?.?.?.?\\xbf|\\xc0.?.?.?.?.?.?.?.?\\xc0|\\xc1.?.?.?.?.?.?.?.?\\xc1\
|\\xc2.?.?.?.?.?.?.?.?\\xc2|\\xc3.?.?.?.?.?.?.?.?\\xc3|\\xc4.?.?.?.?.?.?.?.?\\xc4|\\xc5.?.?.?.?.?.?.?.?\\xc5|\\xc6.?.?.?.?.?.?.?.?\\xc6|\\xc7.?.?.?.?.?.?.?.?\\xc7|\\xc8.?.?.?.?.?.?.?.?\\xc8|\\xc9.?.?.?.?.?.?.?.?\\xc9|\\xca.?.?.?.?.?.?.?.?\\xca|\\xcb.?.?.?.?.?.?.?.?\\xcb|\\xcc.?.?.?.?.?.?.?.?\\xcc|\\xcd.?.?.?.?.?.?.?.?\\xcd|\\xce.?.?.?.?.?.?.?.?\\xce|\\xcf.?.?.?.?.?.?.?.?\\xcf|\\xd0.?.?.?.?.?.?.?.?\\xd0|\\xd1.?.?.?.?.?.?.?.?\\xd1|\\xd2.?.?.?.?.?.?.?.?\\xd2|\\xd3.?.?.?.?.?.?.?.?\\xd3|\\xd4.?.?.?.?.?.?.?.?\\xd4|\\xd5.?.?.?.?.?.?.?.?\\xd5\
|\\xd6.?.?.?.?.?.?.?.?\\xd6|\\xd7.?.?.?.?.?.?.?.?\\xd7|\\xd8.?.?.?.?.?.?.?.?\\xd8|\\xd9.?.?.?.?.?.?.?.?\\xd9|\\xda.?.?.?.?.?.?.?.?\\xda|\\xdb.?.?.?.?.?.?.?.?\\xdb|\\xdc.?.?.?.?.?.?.?.?\\xdc|\\xdd.?.?.?.?.?.?.?.?\\xdd|\\xde.?.?.?.?.?.?.?.?\\xde|\\xdf.?.?.?.?.?.?.?.?\\xdf|\\xe0.?.?.?.?.?.?.?.?\\xe0|\\xe1.?.?.?.?.?.?.?.?\\xe1|\\xe2.?.?.?.?.?.?.?.?\\xe2|\\xe3.?.?.?.?.?.?.?.?\\xe3|\\xe4.?.?.?.?.?.?.?.?\\xe4|\\xe5.?.?.?.?.?.?.?.?\\xe5|\\xe6.?.?.?.?.?.?.?.?\\xe6|\\xe7.?.?.?.?.?.?.?.?\\xe7|\\xe8.?.?.?.?.?.?.?.?\\xe8|\\xe9.?.?.?.?.?.?.?.?\\xe9\
|\\xea.?.?.?.?.?.?.?.?\\xea|\\xeb.?.?.?.?.?.?.?.?\\xeb|\\xec.?.?.?.?.?.?.?.?\\xec|\\xed.?.?.?.?.?.?.?.?\\xed|\\xee.?.?.?.?.?.?.?.?\\xee|\\xef.?.?.?.?.?.?.?.?\\xef|\\xf0.?.?.?.?.?.?.?.?\\xf0|\\xf1.?.?.?.?.?.?.?.?\\xf1|\\xf2.?.?.?.?.?.?.?.?\\xf2|\\xf3.?.?.?.?.?.?.?.?\\xf3|\\xf4.?.?.?.?.?.?.?.?\\xf4|\\xf5.?.?.?.?.?.?.?.?\\xf5|\\xf6.?.?.?.?.?.?.?.?\\xf6|\\xf7.?.?.?.?.?.?.?.?\\xf7|\\xf8.?.?.?.?.?.?.?.?\\xf8|\\xf9.?.?.?.?.?.?.?.?\\xf9|\\xfa.?.?.?.?.?.?.?.?\\xfa|\\xfb.?.?.?.?.?.?.?.?\\xfb|\\xfc.?.?.?.?.?.?.?.?\\xfc|\\xfd.?.?.?.?.?.?.?.?\\xfd\
|\\xfe.?.?.?.?.?.?.?.?\\xfe|\\xff.?.?.?.?.?.?.?.?\\xff)"},
    {"netgeareva", L7_ENUM_NETGEAREVA, 0, "^...\\x53\\x4a\\x61\\x6d"}, /* add by pingod, 07/12/2008*/
    //{"ftp", 7, 0, "^220[\\x09-\\x0d -~]*ftp"},
    //{"http", 8, 0, "http/(0\\.9|1\\.0|1\\.1) [1-5][0-9][0-9] [\\x09-\\x0d -~]*(connection:|content-type:|content-length:|date:)|post [\\x09-\\x0d -~]* http/[01]\\.[019]"},
    //{"msnmessenger", 9, 0, "ver [0-9]+ msnp[1-9][0-9]? [\\x09-\\x0d -~]*cvr0\\x0d\\x0a$|usr 1 [!-~]+ [0-9. ]+\\x0d\\x0a$|ans 1 [!-~]+ [0-9. ]+\\x0d\\x0a$"},
    //{"xunlei", 10, 0, "^[()]...?.?.?(reg|get|query)"},
    //{"ares", 11, 0, "^\\x03[]Z].?.?\\x05$"},
};

/* temporary buffer for pre-processing expression data */
static char g_exp[APP_DATA_BUF_MAX_LEN];

/* temporary buffer for converting app data into lowercase */
static char g_app_data[APP_DATA_BUF_MAX_LEN];

#ifdef L7_DEBUG_ON
static int all_packet_in = 0;
static int all_packet_out = 0;
static int packet_has_pri_in = 0;
static int packet_has_pri_out = 0;
#endif


/*  added start pling 11/08/2007 */
#define QOS_PRIORITY_HIGHEST    3
#define QOS_PRIORITY_HIGH       2
#define QOS_PRIORITY_NORMAL     1
#define QOS_PRIORITY_LOW        0
#define QOS_PRIORITY_UNSPECIFIED -1

static int qos_l7_apps_priority[L7_ENUM_TOTAL + 1];
extern char qos_l7_appls[1024];     

static void load_apps_priority(void)
{
/*   char qos_l7_appls[1024];*/
    char *temp;
    int  apps, priority, i;

    /* Set default apps priority */
    for (i=0; i<=L7_ENUM_TOTAL; i++)
        qos_l7_apps_priority[i] = QOS_PRIORITY_UNSPECIFIED;
    
/*   strcpy(qos_l7_appls, nvram_safe_get("qos_l7_apps"));  weal */

    temp = strtok(qos_l7_appls, " ");
    while (temp)
    {
        sscanf(temp, "%d:%d", &apps, &priority);

        /* Do Sanity check */
        if (apps >= 1 && apps <= L7_ENUM_TOTAL &&
            priority >= QOS_PRIORITY_LOW && priority <= QOS_PRIORITY_HIGHEST)
        {
            qos_l7_apps_priority[apps] = priority;
        }

        temp = strtok(NULL, " ");
    }
}

#define QOS_DSCP_VALUE_HIGHEST  (0x38 << 2)
#define QOS_DSCP_VALUE_HIGH     (0x28 << 2)
#define QOS_DSCP_VALUE_NORMAL   (0x0  << 2)
#define QOS_DSCP_VALUE_LOW      (0x10 << 2)

unsigned char prio_to_dscp(unsigned char prio)
{
    if (prio == QOS_PRIORITY_HIGHEST)
        return QOS_DSCP_VALUE_HIGHEST;
    else
    if (prio == QOS_PRIORITY_HIGH)
        return QOS_DSCP_VALUE_HIGH;
    else
    if (prio == QOS_PRIORITY_LOW)
        return QOS_DSCP_VALUE_LOW;
    else
        return QOS_DSCP_VALUE_NORMAL;
}

UINT16 CalcChecksum(unsigned char *pbData, int iLength)
{
	int nLeft = iLength % 2; /* this addr has odd byte? */
    UINT16 wChecksum;
    int iIndex;
	UINT32 dwSum = 0;

    for (iIndex=0; iIndex < iLength-1; iIndex+=2)
    {
        dwSum += (pbData[iIndex] << 8) | (pbData[iIndex+1]);
    }

    if (nLeft != 0)
        dwSum += (pbData[iLength-1] << 8);


	dwSum = (dwSum >> 16) + (dwSum & 0xffff);     /* add hi 16 to low 16 */
	dwSum += (dwSum >> 16);                     /* add carry */
	wChecksum = ~dwSum;                          /* truncate to 16 bits */
	return (wChecksum);

}

static int modify_dscp(struct sk_buff *skb, int apps)
{
    int pkt_prio;
    int l7_prio;
    int i;

    pkt_prio = skb->cb[sizeof(skb->cb) - 3];
    l7_prio  = qos_l7_apps_priority[apps];

    //printk("#### Got L7apps %d, orig prio %d, l7 prio %d\n", apps, pkt_prio, l7_prio);

    if (l7_prio > pkt_prio)
    {
        unsigned char *ipHdr;
        unsigned int ipHdrLen;
        UINT16 csum;

#if 0
        printk("skb->data: ");
        
        for (i=0; i<20; i++)
            printk("%02X ", (unsigned char)skb->data[i]);
        printk("\n");
#endif   
        /* Get start of IP header & TCP header */
        ipHdr = &(skb->data[0]);
        ipHdrLen = (ipHdr[0] & 0x0F) * 4;

        /* Modify the DSCP value */
        ipHdr[1] = prio_to_dscp(l7_prio);
        
        /* Recompute TCP checksum */
        ipHdr[10] = 0;
        ipHdr[11] = 0;
        csum = CalcChecksum(ipHdr, ipHdrLen);
        ipHdr[10] = (csum & 0xFF00) >> 8;
        ipHdr[11] = (csum & 0xFF);

       // printk("!!!!!! Promote L7apps %d prio from %d to %d\n", apps, pkt_prio, l7_prio);
    }
    return 0;

}
/*  added end pling 11/08/2007 */

/*******function start*******/

unsigned char layer7_scan_all(struct sk_buff *skb, char *app_data, int appdatalen)
{
    s_pattern_cache *node = first_pattern_cache;
    int str_len = 0;
    int i = 0;
    
#ifdef L7_DEBUG_ON
    L7_PRINTK_DEBUG_3("layer7_scan_all start\n");
#endif
    
    str_len = ((sizeof(g_app_data)-1) > appdatalen) ? appdatalen : (sizeof(g_app_data)-1);
    for (i=0; i<str_len; i++)
        g_app_data[i] = tolower(app_data[i]);
    g_app_data[i] = 0;
    
    while (node != NULL)
    {
        if (regexec(node->pattern, g_app_data))
        {
#ifdef L7_DEBUG_ON
            L7_PRINTK_DEBUG_7("layer7 filter match: expression: %s\n", node->regex_string);
            L7_PRINTK_DEBUG_1("layer7 filter match: protocol: %s\n", node->proto_name);
#endif
            return node->proto_enum;
        }
        
        node = node->next;
    }
    return 0;//dismatch
}
/* Returns offset the into the skb->data that the application data starts */
static int app_data_offset(const struct sk_buff *skb)
{
	/* In case we are ported somewhere (ebtables?) where skb->nh.iph 
	isn't set, this can be gotten from 4*(skb->data[0] & 0x0f) as well. */
#ifdef LINUX26
    int ip_hl = 4 * ip_hdr(skb)->ihl;
#else /* LINUX26 */
	int ip_hl = 4*skb->nh.iph->ihl;
#endif

#ifdef LINUX26
	if ( ip_hdr(skb)->protocol == IPPROTO_TCP ) 
#else /* LINUX26 */
	if ( skb->nh.iph->protocol == IPPROTO_TCP ) 
#endif /* LINUX26 */
	{
		/* 12 == offset into TCP header for the header length field. 
		Can't get this with skb->h.th->doff because the tcphdr 
		struct doesn't get set when routing (this is confirmed to be 
		true in Netfilter as well as QoS.) */
		int tcp_hl = 4*(skb->data[ip_hl + 12] >> 4);

		return ip_hl + tcp_hl;
	} 
#ifdef LINUX26
	else if ( ip_hdr(skb)->protocol == IPPROTO_UDP  ) 
#else /* LINUX26 */
	else if ( skb->nh.iph->protocol == IPPROTO_UDP  ) 
#endif /* LINUX26 */
	{
		return ip_hl + 8; /* UDP header is always 8 bytes */
	} 
#ifdef LINUX26
	else if ( ip_hdr(skb)->protocol == IPPROTO_ICMP ) 
#else /* LINUX26 */
	else if ( skb->nh.iph->protocol == IPPROTO_ICMP ) 
#endif /* LINUX26 */
	{
		return ip_hl + 8; /* ICMP header is 8 bytes */
	} 
	else 
	{
		if (net_ratelimit()) 
			L7_PRINTK_DEBUG_3(KERN_ERR "layer7: tried to handle unknown protocol!\n");
		return ip_hl + 8; /* something reasonable */
	}
}

int l7_filter_main(struct sk_buff *skb, int iDirection)
{
    unsigned char   *app_data;  
    unsigned int    appdatalen;
    T_ConnEntry     *pConn;
    unsigned char   proto_enum = 0;
    int             iModifyType = 0;
#ifdef L7_DEBUG_ON
    int             i = 0;
    int             iLen = sizeof(g_all_proto)/sizeof(g_all_proto[0]);
#endif
    
#ifdef L7_DEBUG_ON
    L7_PRINTK_DEBUG_2("l7_filter_main start skb: %x, all_in out: %d %d, has_pri_in out: %d %d, in or out: %d\n", 
        skb, all_packet_in, all_packet_out, packet_has_pri_in, packet_has_pri_out, iDirection);
    if (0 == ((all_packet_out+all_packet_in)&0xff))//print per 256 packets
        L7_PRINTK_DEBUG_1("l7_filter_main start per 256, skb: %x, all_in out: %d %d, has_pri_in out: %d %d, in or out: %d\n", 
            skb, all_packet_in, all_packet_out, packet_has_pri_in, packet_has_pri_out, iDirection);
    if (0 == ((all_packet_out+all_packet_in)&0x03ff))//list identified packets status of every proto per 1024 packets
    {
        L7_PRINTK_DEBUG_1("*********************** start ************************\n");
        L7_PRINTK_DEBUG_1("l7_filter_main per 1024, all_out: %d, has_pri_out: %d\n", 
            all_packet_out, packet_has_pri_out);
        for (i=0; i<iLen; i++)
            L7_PRINTK_DEBUG_1("Proto %s identified %d packets\n", g_all_proto[i].proto_name, g_all_proto[i].proto_packet_count);
        L7_PRINTK_DEBUG_1("************************ end *************************\n");
    }
    if (iDirection)
        all_packet_out++;
    else
        all_packet_in++;
#endif

    if (NULL == skb)
        return L7_ERROR;
        
    skb->cb[sizeof(skb->cb)-4] = L7_ENUM_INIT;//initial value: -1(0xff)
    
#ifdef LINUX26
    pConn = FindConnByHash(ip_hdr(skb), &iModifyType, iDirection);
#else /* LINUX26 */
    pConn = FindConnByHash(skb->nh.iph, &iModifyType, iDirection);
#endif /* LINUX26 */
    
    if (NULL == pConn)
    {
#ifdef L7_DEBUG_ON
        if (iDirection)
            L7_PRINTK_DEBUG_2("l7_filter_main FindConnByHash fail.\n");
#endif
        return L7_SUCCESS;
    }

#ifdef L7_DEBUG_ON
    L7_PRINTK_DEBUG_3("l7_filter_main pConn: %x, pConn->packet_count: %d\n", pConn, pConn->packet_count);
#endif
    if (pConn->proto_enum != 0)
    {
        if (iDirection)
        {
#ifdef L7_DEBUG_ON
            packet_has_pri_out++;
            g_all_proto[pConn->proto_enum-1].proto_packet_count++;
#endif
            skb->cb[sizeof(skb->cb)-4] = pConn->proto_enum;
            if (skb->cb[sizeof(skb->cb)-5])
                modify_dscp(skb, pConn->proto_enum);
        }
#ifdef L7_DEBUG_ON
        else
            packet_has_pri_in++;
        L7_PRINTK_DEBUG_3("l7_filter_main pConn->proto_enum: %d\n", pConn->proto_enum);
#endif
        return L7_SUCCESS;
    }
    
    if (pConn->packet_count > num_packets)
    {
#ifdef L7_DEBUG_ON
        L7_PRINTK_DEBUG_3("too many packets, l7_filter_main pConn->packet_count: %d\n", pConn->packet_count);
#endif
        if (iDirection)
        {
            skb->cb[sizeof(skb->cb)-4] = pConn->proto_enum;
            if (skb->cb[sizeof(skb->cb)-5])
                modify_dscp(skb, pConn->proto_enum);
        }
            
        return L7_SUCCESS;
    }
    pConn->packet_count++;

    if (skb_is_nonlinear(skb))
    {
#ifdef LINUX26
        if (skb_linearize(skb) != 0)
#else /* LINUX26 */
        if (skb_linearize(skb, GFP_ATOMIC) != 0)
#endif /* LINUX26 */
        {
            if (net_ratelimit()) 
                L7_PRINTK_DEBUG_1(KERN_ERR "layer7: failed to linearize packet, bailing.\n");
            return L7_SUCCESS;
        }
    }
    
    /* now that the skb is linearized, it's safe to set these. */
    app_data = skb->data + app_data_offset(skb);
    appdatalen = skb->tail - app_data;

    proto_enum = layer7_scan_all(skb, app_data, appdatalen);

    if (proto_enum == L7_ENUM_NETGEAREVA)
    {
        if ((pConn->bProtocol != UDP_PROTOCOL)
            || ((pConn->bNatType != AG_PORT_FORWARDING) 
                  && ((ntohs(pConn->wDestPort) < EVA_PORT_START) || (ntohs(pConn->wDestPort) > EVA_PORT_END)))
            || ((pConn->bNatType == AG_PORT_FORWARDING) 
                  && ((ntohs(pConn->wSourcePort) < EVA_PORT_START) || (ntohs(pConn->wSourcePort) > EVA_PORT_END))))
        {
            proto_enum = 0;
        }
    }

    if (proto_enum != 0)
    {
#ifdef L7_DEBUG_ON
        L7_PRINTK_DEBUG_1("l7_filter_main identify the conn %x, in %dth packet.\n", pConn, pConn->packet_count);
#endif
        if (iDirection)
        {
#ifdef L7_DEBUG_ON
            packet_has_pri_out++;
            g_all_proto[proto_enum-1].proto_packet_count++;
#endif
            skb->cb[sizeof(skb->cb)-4] = proto_enum;
            if (skb->cb[sizeof(skb->cb)-5])
                modify_dscp(skb, pConn->proto_enum);
        }
#ifdef L7_DEBUG_ON
        else
            packet_has_pri_in++;
#endif
        pConn->proto_enum = proto_enum;
        return L7_SUCCESS;
    }
    return L7_SUCCESS;
}

static int isxdigit(char c)
{
    if ((c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F'))
        return 1;
    else
        return 0;
}

static char tolower(char c)
{
    if (c >= 'A' && c <= 'Z')
        c += 0x20;
    return c;
}

static int hex2dec(char c)
{
    switch (c)
    {
        case '0' ... '9':
            return c - '0';
        case 'a' ... 'f':
            return c - 'a' + 10;
        case 'A' ... 'F':
            return c - 'A' + 10;
        default:
#ifdef L7_DEBUG_ON
            L7_PRINTK_DEBUG_2("hex2dec: bad value!\n");
#endif
            return 0;
    }
}

int pre_process(char *str_out, char *str_in, int str_out_len)
{
    char *result = str_out;
    char *s = str_in;
    int sindex = 0, rindex = 0;
    
    while ( sindex < strlen(s) && (rindex + 1 < str_out_len))
    {
        if ( sindex + 3 < strlen(s) &&
            s[sindex] == '\\' && s[sindex+1] == 'x' && 
            isxdigit(s[sindex + 2]) && isxdigit(s[sindex + 3]) ) 
        {
            /* carefully remember to call tolower here... */
            result[rindex] = tolower( hex2dec(s[sindex + 2])*16 +
                                      hex2dec(s[sindex + 3] ) );

            switch ( result[rindex] )
            {
                case 0x24:
                case 0x28:
                case 0x29:
                case 0x2a:
                case 0x2b:
                case 0x2e:
                case 0x3f:
                case 0x5b:
                case 0x5c:
                case 0x5d:
                case 0x5e:
                case 0x7c:
#ifdef L7_DEBUG_ON
                    L7_PRINTK_DEBUG_1("Warning: layer7 regexp contains a control character, %c, in hex (\\x%c%c).\n"
                        "I recommend that you write this as %c or \\%c, depending on what you meant.\n",
                        result[rindex], s[sindex + 2], s[sindex + 3], result[rindex], result[rindex]);
#endif
                    break;
                case 0x00:
#ifdef L7_DEBUG_ON
                    L7_PRINTK_DEBUG_1("Warning: null (\\x00) in layer7 regexp.  A null terminates the regexp string!\n");
#endif
                    break;
                default:
                    break;
            }

            sindex += 3; /* 4 total */
        }
        else
            result[rindex] = tolower(s[sindex]);

        sindex++; 
        rindex++;
    }
    result[rindex] = '\0';
    
    if (rindex > 0)
        return 0;
    else
        return 1;
}

int load_all_pattern(void)
{
    /* establish the link list of patterns*/
    s_pattern_cache * node = first_pattern_cache;
    s_pattern_cache * tmp;
    unsigned int len;
    unsigned int num_proto = sizeof(g_all_proto)/sizeof(s_proto_regexp);
    int i = 0;
    
#ifdef L7_DEBUG_ON
    L7_PRINTK_DEBUG_1("l7 filter, load_all_pattern start, num_proto: %d\n", num_proto);
#endif
    
    for (i=0; i<num_proto; i++)
    {
#ifdef L7_DEBUG_ON
        L7_PRINTK_DEBUG_2("load_all_pattern for loop: %d, proto_regexp: %s,\n sizeof(s_pattern_cache): %d.\n", 
            i, g_all_proto[i].proto_regexp, sizeof(s_pattern_cache));
#endif
        if (0 != pre_process(g_exp, g_all_proto[i].proto_regexp, sizeof(g_exp)))
            continue;
        
        /* Be paranoid about running out of memory to avoid list corruption. */
        tmp = kmalloc(sizeof(s_pattern_cache), GFP_ATOMIC);
        if (!tmp)
        {
            L7_PRINTK_DEBUG_1("layer7: out of memory in compile_and_cache, bailing. 1\n");
            if (net_ratelimit()) 
                L7_PRINTK_DEBUG_1(KERN_ERR "layer7: out of memory in compile_and_cache, bailing.\n");
            return NULL;
        }
        
        tmp->regex_string   = kmalloc(strlen(g_exp) + 1, GFP_ATOMIC);
        if (NULL == tmp->regex_string)
        {
            L7_PRINTK_DEBUG_1("layer7: out of memory in compile_and_cache, bailing. 2\n");
            if (net_ratelimit()) 
                L7_PRINTK_DEBUG_1(KERN_ERR "layer7: out of memory in compile_and_cache, bailing.\n");
            kfree(tmp);
            return NULL;
        }
        
        tmp->pattern        = regcomp(g_exp, &len);
        if (NULL == tmp->pattern)
        {
            L7_PRINTK_DEBUG_1("layer7: out of memory in compile_and_cache, bailing. 3\n");
            if (net_ratelimit()) 
                L7_PRINTK_DEBUG_1(KERN_ERR "layer7: out of memory in compile_and_cache, bailing.\n");
            kfree(tmp->regex_string);
            kfree(tmp);
            return NULL;
        }
        
        strcpy(tmp->regex_string, g_exp);
        tmp->next           = NULL;
        tmp->proto_enum     = g_all_proto[i].proto_enum;
        tmp->proto_name     = g_all_proto[i].proto_name;
        
#ifdef L7_DEBUG_ON
        L7_PRINTK_DEBUG_2("regexp: %s\n", tmp->proto_name);
        L7_PRINTK_DEBUG_2("tmp->pattern->regstart: %d\n", tmp->pattern->regstart);
        L7_PRINTK_DEBUG_2("tmp->pattern->reganch: %d\n", tmp->pattern->reganch);
        if (tmp->pattern->regmust != NULL)
            L7_PRINTK_DEBUG_2("tmp->pattern->regmust: %s\n", tmp->pattern->regmust);
        L7_PRINTK_DEBUG_2("tmp->pattern->regmlen: %d\n", tmp->pattern->regmlen);
        L7_PRINTK_DEBUG_2("tmp->pattern->program[0]: %d\n", tmp->pattern->program[0]);
#endif/* end of #ifdef L7_DEBUG_ON */
        if (NULL == node)
        {
            node = tmp;
            first_pattern_cache = tmp;
        }
        else
        {
            node->next = tmp;
            node = tmp;
        }
    }
    
    return 1;
}

void free_all_pattern(void)
{
    s_pattern_cache *node = first_pattern_cache;
    s_pattern_cache *tmp;
    
    while (node != NULL)
    {
        if (node->regex_string != NULL)
            kfree(node->regex_string);
        if (node->pattern != NULL)
            kfree(node->pattern);
        
        tmp = node;
        node = node->next;
        kfree(tmp);
    }
}

static int __init init(void)
{
    int ret = 0;
    
#ifdef CONFIG_NETFILTER
    ret = nf_register_hook(&l7_filter_prerouting_ops);
    if (ret < 0) {
        printk("<0>Error registering l7 filter prerouting.\n");
        nf_unregister_hook(&l7_filter_prerouting_ops);
        return ret;
    }
    ret = nf_register_hook(&l7_filter_postrouting_ops);
    if (ret < 0) {
        printk("<0>Error registering l7 filter postrouting.\n");
        return ret;
    }
#else /* CONFIG_NETFILTER */
    acosL7Filter_register_hook(l7_filter_main);
#endif /* CONFIG_NETFILTER */

    load_all_pattern();
    
    load_apps_priority();       /*  added pling 11/08/2007 */
    
    return ret;
}

static void __exit fini(void)
{
#ifdef CONFIG_NETFILTER
    nf_unregister_hook(&l7_filter_prerouting_ops);
    nf_unregister_hook(&l7_filter_postrouting_ops);
#else
    acosL7Filter_register_hook(NULL);
#endif /* CONFIG_NETFILTER */

    free_all_pattern();
}

module_init(init);
module_exit(fini);

EXPORT_SYMBOL(l7_filter_main);
