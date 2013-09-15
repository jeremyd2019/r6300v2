/***************************************************************************
#***
#***    Copyright 2005  Hon Hai Precision Ind. Co. Ltd.
#***    All Rights Reserved.
#***    No portions of this material shall be reproduced in any form without the
#***    written permission of Hon Hai Precision Ind. Co. Ltd.
#***
#***    All information contained in this document is Hon Hai Precision Ind.  
#***    Co. Ltd. company private, proprietary, and trade secret property and 
#***    are protected by international intellectual property laws and treaties.
#***
#****************************************************************************/
#ifndef _ACOSNVRAMCONFIG_H
#define _ACOSNVRAMCONFIG_H

#ifdef __cplusplus
extern "C"
{                               /* Assume C declarations for C++ */
#endif                          /* __cplusplus */

#include "acosTypes.h"

/* definitions */
#define ACOSNVRAM_MAX_TAG_LEN     32
#define ACOSNVRAM_MAX_VAL_LEN     512
#define ACOSNVRAM_MAX_FVAL_LEN    500   /* maximum length formatted string */
/* acos configuration error number */
#define ACOSNVRAM_SUCCESS         0
#define ACOSNVRAM_FAIL            1
#define ACOSNVRAM_TAGNOTFOUND     2
#define ACOSNVRAM_RESOURCE        3
#define ACOSNVRAM_TAGTOOLONG      4
#define ACOSNVRAM_VALTOOLONG      5

/* ambit added start, bright wang, 03/08/2004 */
#define ENCRYPT_START_TAG           "<en>"
#define ENCRYPT_END_TAG             "</en>"
#define DECRYPT_START_TAG           "<de>"
#define DECRYPT_END_TAG             "</de>"
/* ambit added end, bright wang, 03/08/2004 */

#define ACOSNVRAM_TAG_TM_WOL            "TM_WOL_ENABLE"    
/* NVRAM Parameters */
#define ACOSNVRAM_TAG_VERSION           "system_nvram_version"
#define ACOSNVRAM_TAG_WAN_IFS           "system_wan_ifs"
#define ACOSNVRAM_TAG_LAN_IFS           "system_lan_ifs"
#define ACOSNVRAM_TAG_BRIDGE_IFS        "system_bridge_ifs"
#define ACOSNVRAM_TAG_NAT_IFS           "system_nat_ifs"
/* Ambit added start, Jacky Yu 2003/12/22 */
#define ACOSNVRAM_TAG_NAT_INBOUNDNUM    "system_nat_inboundnum"
/* Ambit added end, Jacky Yu 2003/12/22 */
/* Ambit added start, Jacky Yu, 2004/02/26 */
#define ACOSNVRAM_TAG_RTSPALG_TIMEOUT    "system_rtspalg_timeout"
#define ACOSNVRAM_TAG_RTSPALG_SERVERPORT "system_rtspalg_serverport"
/* Ambit added end, Jacky Yu, 2004/02/26 */
#define ACOSNVRAM_TAG_DHCPC_IFS         "system_dhcpc_ifs"
#define ACOSNVRAM_TAG_DHCPS_IFS         "system_dhcps_ifs"
#define ACOSNVRAM_TAG_PPPOE_IFS         "system_pppoe_ifs"

#define ACOSNVRAM_TAG_LAN_IP_ADDR       "system_lan_ipaddr"     /* LAN IP address */
#define ACOSNVRAM_TAG_LAN_NET_MASK      "system_lan_netmask"    /* LAN subnet mask */
    /*#define ACOSNVRAM_TAG_LAN_MAC_ADDR      "system_lan_hwaddr" *//* LAN MAC address */

#define ACOSNVRAM_TAG_WAN_PROTO         "system_wan_ipconfig"   /* WAN protocol, "dhcp", "static" or "pppoe" */
#define ACOSNVRAM_TAG_WAN_IP_ADDR       "system_wan_ipaddr"     /* WAN IP address */
#define ACOSNVRAM_TAG_WAN_NET_MASK      "system_wan_netmask"    /* WAN subnet mask */
    /*#define ACOSNVRAM_TAG_WAN_MAC_ADDR      "system_wan_hwaddr" *//* WAN MAC address */
#define ACOSNVRAM_TAG_WAN_GATEWAY       "system_wan_gateway"    /* WAN default gateway */
#define ACOSNVRAM_TAG_WAN_DNS           "system_wan_dns"        /* WAN DNS */
#define ACOSNVRAM_TAG_WAN_WINS          "system_wan_wins"       /* WAN WINS */
#define ACOSNVRAM_TAG_WAN_HOSTNAME      "system_hostname"       /* WAN hostname */
#define ACOSNVRAM_TAG_WAN_DOMAIN        "system_wan_domain"     /* WAN Domain name */
#define ACOSNVRAM_TAG_FIREWALL          "system_firewall"       /* Firewall enable or disable */
/* Ambit added start, grace kuo, 08/06/2003*/
#define ACOSNVRAM_TAG_CA_NAME           "system_ca_name"        /* Call agent name */
#define ACOSNVRAM_TAG_CA_IP_ADDR        "system_ca_ipaddr"      /* Call agent IP address */
/* Ambit added end, grace kuo, 08/06/2003*/
#define ACOSNVRAM_TAG_TFTP_SERVER       "system_tftp_server"    /* tftp server ip address */

#define ACOSNVRAM_TAG_COUNTRYCODE       "system_countrycode"    /* country code for wireless device */

//Ambit added start, nathan, 2004
#define ACOSNVRAM_TAG_NAPT_MODE         "system_napt_mode"      /* NAPT modes */
#define ACOSNVRAM_TAG_SRV_MODE          "system_srv_mode"       /* Service modes */
//Ambit added end, nathan, 2004

#define ACOSNVRAM_TAG_DHCPS_STATUS      "dhcps_status"  /* Enable / Disable DHPCS, "dhcp" or "static" */
#define ACOSNVRAM_TAG_DHCPS_STARTIP     "dhcps_startip" /* First IP Address in pool */
#define ACOSNVRAM_TAG_DHCPS_ENDIP       "dhcps_endip"   /* Last IP Address in pool */
#define ACOSNVRAM_TAG_DHCPS_MODE        "dhcps_mode"    /* "auto" or "manual" */
#define ACOSNVRAM_TAG_DHCPS_DF_GW       "dhcps_df_gw"   /* Default gateway */
#define ACOSNVRAM_TAG_DHCPS_DNS         "dhcps_dns"     /* DNS */
#define ACOSNVRAM_TAG_DHCPS_WINS        "dhcps_wins"    /* WINS server */
#define ACOSNVRAM_TAG_DHCPS_DOMAIN      "dhcps_domain"  /* Domain name */
#define ACOSNVRAM_TAG_DHCPS_LEASE_TIME  "dhcps_lease_time"      /* Lease time */
#define ACOSNVRAM_TAG_DHCPS_RESERVED_IP "dhcps_reserved_ip"     /* Reserved IP address */
#define ACOSNVRAM_TAG_DHCPS_RESERVED_MAC "dhcps_reserved_mac"   /* MAC address to get the reserved IP */

#define ACOSNVRAM_TAG_DHCPC_CLID        "dhcpc_clid"    /* DHCP client ID */
#define ACOSNVRAM_TAG_DHCPC_OPT         "dhcpc_opts"    /* DHCP Options */

#define ACOSNVRAM_TAG_PPPOE_VIRTUAL_IFS "pppoe_virtual_ifs"     /* PPPOE virtual interface name */
#define ACOSNVRAM_TAG_PPPOE_USERNAME    "pppoe_username"        /* PPPOE username */
#define ACOSNVRAM_TAG_PPPOE_PASSWD      "pppoe_passwd"  /* PPPOE password */
#define ACOSNVRAM_TAG_PPPOE_MTU         "pppoe_mtu"     /* PPPOE MTU size */
#define ACOSNVRAM_TAG_PPPOE_MRU         "pppoe_mru"     /* PPPOE MRU size */
#define ACOSNVRAM_TAG_PPPOE_IDLETIME    "pppoe_idletime"        /* PPPOE idle time */
#define ACOSNVRAM_TAG_PPPOE_KEEPALIVE   "pppoe_keepalive"       /* PPPOE keep alive function, "0" or "1" */
#define ACOSNVRAM_TAG_PPPOE_AUTHTYPE    "pppoe_authtype"        /* PPP authentication type */
#define ACOSNVRAM_TAG_PPPOE_SERVICENAME "pppoe_servicename"     /* PPPOE service name */
#define ACOSNVRAM_TAG_PPPOE_ACNAME      "pppoe_acname"  /* PPPOE AC name */
#define ACOSNVRAM_TAG_PPPOE_DOD         "pppoe_dod"     /* PPPOE dial on demand */
#define ACOSNVRAM_TAG_PPPOE_SESSIONID   "pppoe_session_id"      /* PPPoE previous session ID */
#define ACOSNVRAM_TAG_PPPOE_SERVER_MAC  "pppoe_server_mac"      /* PPPoE previous server Mac */

#define ACOSNVRAM_TAG_NAT_ENABLE        "nat_enable"    /* Enable/disable NAT, 0=disable,1=enable */
#define ACOSNVRAM_TAG_TCP_FORWARD       "nat_forward_tcp"       /* TCP port forwarding rules */
#define ACOSNVRAM_TAG_UDP_FORWARD       "nat_forward_udp"       /* UDP port forwarding rules */
#define ACOSNVRAM_TAG_DMZ_ADDR          "nat_dmz_ipaddr"        /* DMZ address */
#define ACOSNVRAM_TAG_STATIC_ROUTE      "nat_static_route"      /* Static routing table */

#define ACOSNVRAM_TAG_RIP_ENABLE        "rip_enable"    /* Enable / disable RIP */
#define ACOSNVRAM_TAG_RIP_FILTER_ENABLE "rip_filter_enable"     /* Enable / disable RIP filter */
#define ACOSNVRAM_TAG_RIP_MULTICAST     "rip_multicast" /* RIP multicast */
#define ACOSNVRAM_TAG_RIP_VERSION       "rip_version"   /* RIP version */

#define ACOSNVRAM_TAG_LAN_IP_FILTER     "filter_ip"     /* LAN IP filters */
#define ACOSNVRAM_TAG_LAN_TCP_FILTER    "filter_tcp"    /* LAN TCP filters */
#define ACOSNVRAM_TAG_LAN_UDP_FILTER    "filter_udp"    /* LAN UDP filters */
#define ACOSNVRAM_TAG_LAN_MAC_FILTER    "filter_mac"    /* LAN MAC filters */

    /*#define ACOSNVRAM_TAG_RESTORE_DEFAULT   "restore_defaults" *//* Set to "1" to restore factory defaults */

#define ACOSNVRAM_TAG_SYSLOG_SERVER     "system_syslog_server"  /* Syslog server */
#define ACOSNVRAM_TAG_NTP_SERVER        "system_ntp_server"     /* SNTP server */
#define ACOSNVRAM_TAG_TIMEZONE          "system_time_zone"      /* Timezone */
#define ACOSNVRAM_TAG_PERIOD            "sntp_period"   /* sntp get NTP server time period */
#define ACOSNVRAM_TAG_FASTUDP_FLAG      "fastudp_flag"  /* fastudp flag */

#define ACOSNVRAM_TAG_HTTP_NAME1         "httpd_username1"      /* WEB UI login name */
#define ACOSNVRAM_TAG_HTTP_PASSWD1       "httpd_password1"      /* WEB UI password */
#define ACOSNVRAM_TAG_HTTP_NAME2         "httpd_username2"      /* WEB UI login name */
#define ACOSNVRAM_TAG_HTTP_PASSWD2       "httpd_password2"      /* WEB UI password */
#define ACOSNVRAM_TAG_HTTP_NAME3         "httpd_username3"      /* WEB UI login name */
#define ACOSNVRAM_TAG_HTTP_PASSWD3       "httpd_password3"      /* WEB UI password */
#define ACOSNVRAM_TAG_HTTP_NAME4         "httpd_username4"      /* WEB UI login name */
#define ACOSNVRAM_TAG_HTTP_PASSWD4       "httpd_password4"      /* WEB UI password */

/* Ambit add start for-CLI user tags, Castro Huang, 2003/10/06 */
#define ACOSNVRAM_TAG_CLI_SUPER          "cli_super_username"
#define ACOSNVRAM_TAG_CLI_SUPER_PWD      "cli_super_passwd"
#define ACOSNVRAM_TAG_CLI_SUPER_RMTPWD   "cli_super_rmtpwd"
#define ACOSNVRAM_TAG_CLI_USR1           "cli_user_01"
#define ACOSNVRAM_TAG_CLI_USR1_PWD       "cli_user_01_passwd"
#define ACOSNVRAM_TAG_CLI_USR1_RMTPWD    "cli_user_01_rmtpwd"
#define ACOSNVRAM_TAG_CLI_USR2           "cli_user_02"
#define ACOSNVRAM_TAG_CLI_USR2_PWD       "cli_user_02_passwd"
#define ACOSNVRAM_TAG_CLI_USR2_RMTPWD    "cli_user_02_rmtpwd"
#define ACOSNVRAM_TAG_CLI_USR3           "cli_user_03"
#define ACOSNVRAM_TAG_CLI_USR3_PWD       "cli_user_03_passwd"
#define ACOSNVRAM_TAG_CLI_USR3_RMTPWD    "cli_user_03_rmtpwd"

/* Ambit add start for- cli session timeout, Castro Huang, 2004/04/27 */
#define ACOSNVRAM_TAG_CLI_SESS_TIMEOUT   "cli_sess_timeout"
#define ACOSNVRAM_TAG_CLI_RMT_TIMEOUT    "cli_rmt_timeout"
#define ACOSNVRAM_TAG_CLI_CONSOLE_LOCK   "cli_console_lock"
/* Ambit add end for- cli session timeout, Castro Huang, 2004/04/27 */

    /* #define ACOSNVRAM_TAG_USER_NAME          "cli_user_username" *//* CLI user-level username */
    /* #define ACOSNVRAM_TAG_USER_PASSWD        "cli_user_passwd"      *//* CLI user-level password */
    /* #define ACOSNVRAM_TAG_SUPER_NAME         "cli_super_username"   *//* CLI supervisor-level username */
    /* #define ACOSNVRAM_TAG_SUPER_PASSWD       "cli_super_passwd"     *//* CLI supervisor-level password */

#define ACOSNVRAM_TAG_SSH_TYPE           "SSH_TYPE"     /* SSH_THROUGH_TELNET */
#define ACOSNVRAM_TAG_SSH_LISTEN_PORT    "SSH_LISTEN_PORT"
#define ACOSNVRAM_TAG_SSH_HOST_KEY       "SSH_HOST_KEY"
/* Ambit add end for-CLI user tags, Castro Huang, 2003/10/06 */


/* Ambit add start, Peter Chen, 12/26/2003 */
/* Reason: Save static routes to nvram */
#define ACOSNVRAM_TAG_ROUTE_NUM       	 "route_num"    /* total static route number */
#define ACOSNVRAM_TAG_ROUTE_TAG_NUM      "route_tag_num"        /* total tag number in use   */
#define ACOSNVRAM_TAG_ROUTE_ENTRY0       "route_entry0" /* static route index  0     */
#define ACOSNVRAM_TAG_ROUTE_ENTRY1       "route_entry1" /* static route index  1     */
#define ACOSNVRAM_TAG_ROUTE_ENTRY2       "route_entry2" /* static route index  2     */
#define ACOSNVRAM_TAG_ROUTE_ENTRY3       "route_entry3" /* static route index  3     */
#define ACOSNVRAM_TAG_ROUTE_ENTRY4       "route_entry4" /* static route index  4     */
#define ACOSNVRAM_TAG_ROUTE_ENTRY5       "route_entry5" /* static route index  5     */
#define ACOSNVRAM_TAG_ROUTE_ENTRY6       "route_entry6" /* static route index  6     */
#define ACOSNVRAM_TAG_ROUTE_ENTRY7       "route_entry7" /* static route index  7     */
#define ACOSNVRAM_TAG_ROUTE_ENTRY8       "route_entry8" /* static route index  8     */
#define ACOSNVRAM_TAG_ROUTE_ENTRY9       "route_entry9" /* static route index  9     */
/* Ambit add end, Peter Chen, 12/26/2003 */

/* 802.11b parameters */
#define ACOSNVRAM_TAG_WLB_NAME          "wlb_name"
#define ACOSNVRAM_TAG_WLB_SSID          "wlb_ssid"
#define ACOSNVRAM_TAG_WLB_AUTH_ALG      "wlb_auth"
#define ACOSNVRAM_TAG_WLB_CHANNEL       "wlb_channel"
#define ACOSNVRAM_TAG_WLB_PREAMBLE      "wlb_preamble"
#define ACOSNVRAM_TAG_WLB_BASIC_RATE    "wlb_basic_rate"
#define ACOSNVRAM_TAG_WLB_SUPPORT_RATE  "wlb_sup_rate"
#define ACOSNVRAM_TAG_WLB_TX_RATE       "wlb_tx_rate"
#define ACOSNVRAM_TAG_WLB_FRAG          "wlb_frag"
#define ACOSNVRAM_TAG_WLB_RTS           "wlb_rts"
#define ACOSNVRAM_TAG_WLB_DTIM          "wlb_dtim"
#define ACOSNVRAM_TAG_WLB_BEACON        "wlb_bcn"
#define ACOSNVRAM_TAG_WLB_PRIVACY       "wlb_wep"
#define ACOSNVRAM_TAG_WLB_EXCL_NOWEP    "wlb_excl_nowep"
#define ACOSNVRAM_TAG_WLB_KEYID         "wlb_key"
#define ACOSNVRAM_TAG_WLB_KEYID_64      "wlb_key_64"
#define ACOSNVRAM_TAG_WLB_KEYID_128     "wlb_key_128"
#define ACOSNVRAM_TAG_WLB_KEY64_1       "wlb_key64_1"
#define ACOSNVRAM_TAG_WLB_KEY64_2       "wlb_key64_2"
#define ACOSNVRAM_TAG_WLB_KEY64_3       "wlb_key64_3"
#define ACOSNVRAM_TAG_WLB_KEY64_4       "wlb_key64_4"
#define ACOSNVRAM_TAG_WLB_KEY128_1      "wlb_key128_1"
#define ACOSNVRAM_TAG_WLB_KEY128_2      "wlb_key128_2"
#define ACOSNVRAM_TAG_WLB_KEY128_3      "wlb_key128_3"
#define ACOSNVRAM_TAG_WLB_KEY128_4      "wlb_key128_4"
#define ACOSNVRAM_TAG_WLB_ACL_ENABLE    "wlb_acl_enable"
#define ACOSNVRAM_TAG_WLB_MAC           "wlb_mac"
#define ACOSNVRAM_TAG_WLB_ICC           "wlb_icc"
#define ACOSNVRAM_TAG_WLB_COUNTRY       "wlb_country"
#define ACOSNVRAM_TAG_WLB_AP_DENSITY    "wlb_ap_density"
#define ACOSNVRAM_TAG_WLB_AUTO_CHANNEL  "wlb_auto_channel"
#define ACOSNVRAM_TAG_WLB_ENH_SECURITY  "wlb_enh_security"
#define ACOSNVRAM_TAG_WLB_ENABLE_BRIDGE "wlb_enable_bridge"
#define ACOSNVRAM_TAG_WLB_WANLINK       "wlb_wanlink"
#define ACOSNVRAM_TAG_WLB_TXPOWER       "wlb_txpower"
#define ACOSNVRAM_TAG_WLB_TXPOWER_OFFSET "wlb_txpower_offset"
#define ACOSNVRAM_TAG_WLB_CLIENTTHRES   "wlb_clientthres"
#define ACOSNVRAM_TAG_WLB_TRAFFICTHRES  "wlb_trafficthres"

#define ACOSNVRAM_TAG_WLB_MODE          "wlb_mode"
#define ACOSNVRAM_TAG_WLB_WDS_MAC       "wlb_wds_mac"

#define ACOSNVRAM_TAG_WDS_PORT_ENABLE_1 "wlb_wds_port_enable_1"
#define ACOSNVRAM_TAG_WDS_PORT_ENABLE_2 "wlb_wds_port_enable_2"
#define ACOSNVRAM_TAG_WDS_PORT_ENABLE_3 "wlb_wds_port_enable_3"
#define ACOSNVRAM_TAG_WDS_PORT_ENABLE_4 "wlb_wds_port_enable_4"
#define ACOSNVRAM_TAG_WDS_PORT_ENABLE_5 "wlb_wds_port_enable_5"
#define ACOSNVRAM_TAG_WDS_PORT_ENABLE_6 "wlb_wds_port_enable_6"
#define ACOSNVRAM_TAG_WDS_AP_MAC_1      "wlb_wds_ap_mac_1"
#define ACOSNVRAM_TAG_WDS_AP_MAC_2      "wlb_wds_ap_mac_2"
#define ACOSNVRAM_TAG_WDS_AP_MAC_3      "wlb_wds_ap_mac_3"
#define ACOSNVRAM_TAG_WDS_AP_MAC_4      "wlb_wds_ap_mac_4"
#define ACOSNVRAM_TAG_WDS_AP_MAC_5      "wlb_wds_ap_mac_5"
#define ACOSNVRAM_TAG_WDS_AP_MAC_6      "wlb_wds_ap_mac_6"

#define ACOSNVRAM_TAG_SNMP_GET_COMM_NAME        "snmp_get_comm_name"
#define ACOSNVRAM_TAG_SNMP_SET_COMM_NAME        "snmp_set_comm_name"

#define ACOSNVRAM_TAG_RADIUS_AUTH_MODE   "radius_auth_mode"
#define ACOSNVRAM_TAG_RADIUS_AUTH_IP     "radius_auth_ip"
#define ACOSNVRAM_TAG_RADIUS_AUTH_PORT   "radius_auth_port"
#define ACOSNVRAM_TAG_RADIUS_AUTH_SECRET "radius_auth_secret"
#define ACOSNVRAM_TAG_RADIUS_ACCT_IP     "radius_acct_ip"
#define ACOSNVRAM_TAG_RADIUS_ACCT_PORT   "radius_acct_port"
#define ACOSNVRAM_TAG_RADIUS_ACCT_SECRET "radius_acct_secret"
#define ACOSNVRAM_TAG_RADIUS_DWEP        "radius_dwep"

#define ACOSNVRAM_TAG_HTTP_RMEANBlE     "httpd_rmenable"        /* Enable/ Disable remote management */
#define ACOSNVRAM_TAG_HTTP_RMPORT       "httpd_rmport"  /* Remote management port number */
#define ACOSNVRAM_TAG_HTTP_RMSTARTIP    "httpd_rmstartip"       /* Remote management allowed ip range start */
#define ACOSNVRAM_TAG_HTTP_RMENDIP      "httpd_rmendip" /* Remote management allowed ip range end */

/* Ambit add start, Peter Chen, 03/11/2004 */
#define ACOSNVRAM_TAG_UPNP_EANBlE       "upnp_enable"   /* UPnP enable/disable */
#define ACOSNVRAM_TAG_UPNP_PERIOD       "upnp_period"   /* UPnP advertisement period */
#define ACOSNVRAM_TAG_UPNP_TTL          "upnp_ttl"      /* UPnP advertisement TTL */
/* Ambit add end, Peter Chen, 03/11/2004 */
/* Ambit added start, Eddic, 03/16/2004 */
#define ACOSNVRAM_TAG_UPNP_PORT         "upnp_port"     /* UPnP TCP port for HTTP GET, GENA, and SOAP */
#define ACOSNVRAM_TAG_UPNP_IFENABLE     "upnp_ifenable" /* Enable/Disable UPnP support for the specified interface */
/* Ambit added end, Eddic, 03/16/2004 */
/* Ambit added start, Eddic, 05/17/2004 */
#define ACOSNVRAM_TAG_UPNP_ALGPORTNUM		"upnp_algport_num"      /* Number of ALG port related to UPnP */
#define ACOSNVRAM_TAG_UPNP_ALGPORT_PREFIX	"upnp_algport"  /* ALG port related to UPnP */
#define ACOSNVRAM_TAG_UPNP_ALGPROT_PREFIX	"upnp_algport_pro"      /* ALG port protocol related to UPnP */
/* Ambit added end, Eddic, 05/17/2004 */

    extern int acosNvramConfig_init (IN char *pcFsMount);
    extern char *acosNvramConfig_get(const char *name);
    /* wklin added start */
    extern char *acosNvramConfig_bget(const char *name, char *buf, int buf_len);
    extern char *acosNvramConfig_get_decode(const char *name);
    extern int acosNvramConfig_set_encode(const char *name, const char *value);
    extern int acosNvramConfig_read_decode(IN char *pcTagName, OUT char *pcValue,
                                     IN int iMaxBufSize);
    /* wklin added end */
    extern int acosNvramConfig_set (const char *name, const char *value);
    extern int acosNvramConfig_unset (const char *name);
    extern int acosNvramConfig_match (char *name, char *match);
    extern int acosNvramConfig_invmatch (char *name, char *invmatch);

    extern int acosNvramConfig_read (IN char *pcTagName, OUT char *pcValue,
                                     IN int iMaxBufSize);
    extern int acosNvramConfig_readAsInt (IN char *pcTagName,
                                          OUT int *pcValue,
                                          IN int iMaxBufSize);
    extern int acosNvramConfig_readFile (IN char *pcTagName,
                                         OUT char *pcValue,
                                         IN int iMaxBufSize);
    extern int acosNvramConfig_readDefault (IN char *pcTagName,
                                            OUT char *pcValue,
                                            IN int iMaxBufSize);
    extern int acosNvramConfig_write (IN char *pcTagName, IN char *pcValue,
                                      IN BOOL blSaveImmediately);
    extern int acosNvramConfig_writeAsInt (IN char *pcTagName, IN int iValue,
                                           IN BOOL blSaveImmediately);
    extern int acosNvramConfig_save (void);

    /*  added start Peter Ling 12/05/2005 */
    /* used for config backup and restore */
    extern int acosNvramConfig_readflash (char *buf);
    extern int acosNvramConfig_writeflash (char *buf, int size);
    /*  added end Peter Ling 12/05/2005 */
#if (defined U12H187)
    extern void acosNvramConfig_setPAParam_RU(void);
#endif
/******************The naming rule of config tag name *************************
*** 1. <module name or grpou name>_<config name>
*** e.g. "system_lan_ipaddr"
*** e.g. "system_wan_ipaddr"
*** e.g. "dhcpserver_lease_time"
*** e.g. "dhcpserver_reservedip"
*** The config items those are not belong to any module please classify them to "system".
*** 2. The length of TAG name and VALUE name can not exceed 32 and 512 characters
*** 3. The written config data are saved to separated config files
*** e.g. % system.cfg
***    system_lan_ipaddr=192.168.1.1 255.255.255.0
***    system_web_loginname=admin
***    system_web_password=password
***            ...
*** e.g. % dhcpserver.cfg
***    dhcpserver_leasetime=86400
***    dhcpserver_reservedip_1=00:11:22:33:44:55 192.168.1.10
***    dhcpserver_reservedip_2=00:11:22:33:44:66 192.168.1.11
*******************************************************************************/
    extern int acosNvramConfig_readHuge (char *pTagName, char *pBuf,
                                         int *piBufLen);
    extern int acosNvramConfig_writeHuge (char *pTagName, char *pBuf,
                                          int iBufLen);
    extern int acosNvramConfig_updateProfile (char *pBuf);
    extern int acosNvramConfig_loadFactoryDefault (char *pFileName);
    extern int acosNvramConfig_cmiGetVersion (char *pstrVersion,
                                              int iBufSize);
    extern int acosNvramConfig_loadTagDefault (IN char *pTagName,
                                               OUT char *pTagValue,
                                               IN int iMaxBufSize);
/* ambit added start, bright wang, 03/08/2004 */
    extern int acosNvramConfig_readEncryption (IN char *pTagName,
                                               OUT char *pValue,
                                               IN int iMaxValBufSize);
    extern int acosNvramConfig_writeEncryption (IN char *pTagName,
                                                IN const char *pValue,
                                                IN BOOL blSaveImmediately);
/* ambit added end, bright wang, 03/08/2004 */
#ifndef foreach

/* Copy each token in wordlist delimited by space into word */
#define foreach(word, wordlist, next) \
	for (next = &wordlist[strspn(wordlist, " ")], \
	     strncpy(word, next, sizeof(word)), \
	     word[strcspn(word, " ")] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strchr(next, ' '); \
	     strlen(word); \
	     next = next ? &next[strspn(next, " ")] : "", \
	     strncpy(word, next, sizeof(word)), \
	     word[strcspn(word, " ")] = '\0', \
	     word[sizeof(word) - 1] = '\0', \
	     next = strchr(next, ' '))

#endif
#ifdef __cplusplus
}
#endif

extern int acosNvramConfig_setPAParam_RU2(int enable);

/*  add start, FredPeng, 03/18/2009 */
extern int WAN_ith_CONFIG_SET_AS_STR(int wanIdx, char *name, char *value);
/*  add end, FredPeng, 03/18/2009 */

#endif                          /* _ACOSNVRAMCONFIG_H */
