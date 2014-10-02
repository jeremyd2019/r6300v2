
#ifndef _MULTISSIDCONTROL_H_
#define _MULTISSIDCONTROL_H_

#ifndef uint16
#define uint16 unsigned short
#endif
#ifndef uint8
#define uint8 unsigned char
#endif
#ifndef uint32
#define uint32 unsigned int
#endif
#ifndef int32
#define int32 int
#endif
#ifndef int8
#define int8 char
#endif

#ifndef int16
#define int16 short
#endif

#define MS_ACCEPT       0
#define MS_DROP         1

#define DHCP_SERVER_PORT  67
#define DHCP_CLIENT_PORT  68
#define HTTP_PORT         80 /*  added, zacker, 08/11/2009, @mbssid_filter */

/*  add start, Tony W.Y. Wang, 12/22/2009 @block FTP and Samba access */
#define FTP_PORT1        20
#define FTP_PORT2        21
#define SAMBA_PORT1      137
#define SAMBA_PORT2      138
#define SAMBA_PORT3      139
#define SAMBA_PORT4      445    /*stanley add 01/13/2010 add samba port*/
#define SOAP_API_PORT    5000   /* pling added 10/22/2013, SOAP API port */

/*  added start, Wins, 03/18/2011, @AP_MODE */
#if defined(AP_MODE)             
#define WAN_IFNAME          "eth0"
#define WL0_GUEST1_IFNAME   "wl0.1"
#define WL0_GUEST2_IFNAME   "wl0.2"
#define WL0_GUEST3_IFNAME   "wl0.3"
#if (defined INCLUDE_DUAL_BAND)
#define WL1_GUEST1_IFNAME   "wl1.1"
#define WL1_GUEST2_IFNAME   "wl1.2"
#define WL1_GUEST3_IFNAME   "wl1.3"
#endif
#endif /* AP_MODE */
/*  added end, Wins, 03/18/2011, @AP_MODE */

/*  add end, Tony W.Y. Wang, 12/22/2009 */
typedef struct MulitSsidControlProfile {
       char IfName[8];
       int enable;
       unsigned int FilterIP; /*  added, zacker, 08/11/2009, @mbssid_filter */
       /*  added start, Wins, 03/18/2011, @AP_MODE */
#if defined(AP_MODE)
       int apMode;
#endif /* AP_MODE */
       /*  added end, Wins, 03/18/2011, @AP_MODE */
}T_MSsidCtlProfile;


#if 0
#define MultiSsidControlDEBUG(fmt, args...)      printk("<0>MultiSsidControlDEBUG: " fmt, ##args)
#define MultiSsidControlERROR       printk("<0>MultiSsidControlERROR: " fmt, ##args)
#else
#define MultiSsidControlDEBUG(fmt, args...)
#define MultiSsidControlERROR
#endif



#endif 
