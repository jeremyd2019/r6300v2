
#ifndef _ACCESSCONTROL_H_
#define _ACCESSCONTROL_H_

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
#define MAX_DEVICE_NAME   32 




/*Fxcn added start, dennis,02/16/2012 @access control*/
/*
typedef struct AccessControlTable{
       unsigned char AttachMac[6];
       int enable;
}T_AccessControlTable;
*/

typedef struct access_control_mac_list
{
    unsigned char mac[6];
    int status;
    char dev_name[MAX_DEVICE_NAME];
    int conn_type;
}T_AccessControlTable;

/*Fxcn added end, dennis,02/16/2012 @access control*/



#if 0
#define AccessControlDEBUG(fmt, args...)      printk("<0>MultiSsidControlDEBUG: " fmt, ##args)
#define AccessControlERROR       printk("<0>MultiSsidControlERROR: " fmt, ##args)
#else
#define AccessControlDEBUG(fmt, args...)
#define AccessControlERROR
#endif



#endif 
