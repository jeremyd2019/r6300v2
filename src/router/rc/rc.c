/*
 * Router rc control script
 *
 * Copyright (C) 2012, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: rc.c 348281 2012-08-01 06:17:58Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h> /* for open */
#include <string.h>
#include <sys/klog.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/utsname.h> /* for uname */
#include <net/if_arp.h>
#include <dirent.h>

#include <epivers.h>
#include <router_version.h>
#include <mtd.h>
#include <shutils.h>
#include <rc.h>
#include <netconf.h>
#include <nvparse.h>
#include <bcmdevs.h>
#include <bcmparams.h>
#include <bcmnvram.h>
#include <wlutils.h>
#include <ezc.h>
#include <pmon.h>
#if defined(__CONFIG_WAPI__) || defined(__CONFIG_WAPI_IAS__)
#include <wapi_utils.h>
#endif /* __CONFIG_WAPI__ || __CONFIG_WAPI_IAS__ */

/*  added start, zacker, 09/17/2009, @wps_led */
#include <fcntl.h>
#include <wps_led.h>
/*  added end, zacker, 09/17/2009, @wps_led */

/*fxcn added by dennis start,05/03/2012, fixed guest network can't reconnect issue*/
#define MAX_BSSID_NUM       4
#define MIN_BSSID_NUM       2
/*fxcn added by dennis end,05/03/2012, fixed guest network can't reconnect issue*/


#ifdef __CONFIG_NAT__
static void auto_bridge(void);
#endif	/* __CONFIG_NAT__ */

#include <sys/sysinfo.h> /*  wklin added */
#ifdef __CONFIG_EMF__
extern void load_emf(void);
#endif /* __CONFIG_EMF__ */

static void restore_defaults(void);
static void sysinit(void);
static void rc_signal(int sig);
/*  added start, Wins, 05/16/2011, @RU_IPTV */
#if defined(CONFIG_RUSSIA_IPTV)
static int is_russia_specific_support (void);
#endif /* CONFIG_RUSSIA_IPTV */
/*  added end, Wins, 05/16/2011, @RU_IPTV */

extern struct nvram_tuple router_defaults[];

#define RESTORE_DEFAULTS() \
	(!nvram_match("restore_defaults", "0") || nvram_invmatch("os_name", "linux"))

#define RESTORE_DEFAULTS() (!nvram_match("restore_defaults", "0") || nvram_invmatch("os_name", "linux"))

/* for WCN support,  added start by EricHuang, 12/13/2006 */
static void convert_wlan_params(void)
{
    int config_flag = 0; /*  add, Tony W.Y. Wang, 01/06/2010 */

    /*  added start pling 03/05/2010 */
    /* Added for dual band WPS req. for WNDR3400 */
#define MAX_SSID_LEN    32
    char wl0_ssid[64], wl1_ssid[64];

    /* first check how we arrived here?
     * 1. or by "add WPS client" in unconfigured state.
     * 2. by external register configure us, 
     */
    strcpy(wl0_ssid, nvram_safe_get("wl0_ssid"));
    strcpy(wl1_ssid, nvram_safe_get("wl1_ssid"));
    /*  modified, Tony W.Y. Wang, 03/24/2010 @WPS random ssid setting */
    if (!nvram_match("wps_start", "none") || nvram_match("wps_pbc_conn_success", "1"))
    {
        /* case 1 above, either via pbc, or gui */
        /* In this case, the WPS set both SSID to be
         *  either "NTRG-2.4G_xxx" or "NTRG-5G_xxx".
         * We need to set proper SSID for each radio.
         */
#define RANDOM_SSID_2G  "NTGR-2.4G_"
#define RANDOM_SSID_5G  "NTGR-5G_"

        /*  modified start pling 05/23/2012 */
        /* Fix a issue where 2.4G radio is disabled, 
         * router uses incorrect random ssid */
        /* if (strncmp(wl0_ssid, RANDOM_SSID_2G, strlen(RANDOM_SSID_2G)) == 0) */
                if (strncmp(wl0_ssid, RANDOM_SSID_2G, strlen(RANDOM_SSID_2G)) == 0 && !nvram_match("wl0_radio","0"))
        /*  modified end pling 05/23/2012 */
        {
            printf("Random ssid 2.4G\n");
            /* Set correct ssid for 5G */
            sprintf(wl1_ssid, "%s%s", RANDOM_SSID_5G, 
                    &wl0_ssid[strlen(RANDOM_SSID_2G)]);
            nvram_set("wl1_ssid", wl1_ssid);
        }
        else
        if (strncmp(wl1_ssid, RANDOM_SSID_5G, strlen(RANDOM_SSID_5G)) == 0 && !nvram_match("wl1_radio","0"))
        {
            printf("Random ssid 5G\n");
            /* Set correct ssid for 2.4G */
            sprintf(wl0_ssid, "%s%s", RANDOM_SSID_2G, 
                    &wl1_ssid[strlen(RANDOM_SSID_5G)]);
            nvram_set("wl0_ssid", wl0_ssid);
        
            /*  added start pling 05/23/2012 */
            /* Fix a issue where 2.4G radio is disabled, 
             * router uses incorrect random ssid/passphrase.
             */
            if (nvram_match("wl0_radio","0"))
                nvram_set("wl0_wpa_psk", nvram_get("wl1_wpa_psk"));
            /*  added end pling 05/23/2012 */
        }
        nvram_unset("wps_pbc_conn_success");
    }
    else
    {
        /* case 2 */
        /* now check whether external register is from:
         * 1. UPnP,
         * 2. 2.4GHz radio
         * 3. 5GHz radio
         */
        if (nvram_match("wps_is_upnp", "1"))
        {
            /* Case 1: UPnP: wired registrar */
            /* SSID for both interface should be same already.
             * So nothing to do.
             */
            printf("Wired External registrar!\n");
        }
        else
        if (nvram_match("wps_currentRFband", "1"))
        {
            /* Case 2: 2.4GHz radio */
            /* Need to add "-5G" to the SSID of the 5GHz band */
            char ssid_suffix[] = "-5G";
            if (MAX_SSID_LEN - strlen(wl0_ssid) >= strlen(ssid_suffix))
            {
                printf("2.4G Wireless External registrar 1!\n");
                /* SSID is not long, so append suffix to wl1_ssid */
                sprintf(wl1_ssid, "%s%s", wl0_ssid, ssid_suffix);
            }
            else
            {
                printf("2.4G Wireless External registrar 2!\n");
                /* SSID is too long, so replace last few chars of ssid
                 * with suffix
                 */
                strcpy(wl1_ssid, wl0_ssid);
                strcpy(&wl1_ssid[MAX_SSID_LEN - strlen(ssid_suffix)], ssid_suffix);
            }
            if (strlen(wl1_ssid) > MAX_SSID_LEN)
                printf("Error wl1_ssid too long (%d)!\n", strlen(wl1_ssid));

            nvram_set("wl1_ssid", wl1_ssid);
        }
        else
        if (nvram_match("wps_currentRFband", "2"))
        {
            /* Case 3: 5GHz radio */
            /* Need to add "-2.4G" to the SSID of the 2.4GHz band */
            char ssid_suffix[] = "-2.4G";

            if (MAX_SSID_LEN - strlen(wl1_ssid) >= strlen(ssid_suffix))
            {
                printf("5G Wireless External registrar 1!\n");
                /* SSID is not long, so append suffix to wl1_ssid */
                sprintf(wl0_ssid, "%s%s", wl1_ssid, ssid_suffix);
            }
            else
            {
                printf("5G Wireless External registrar 2!\n");
                /* Replace last few chars ssid with suffix */
                /* SSID is too long, so replace last few chars of ssid
                 * with suffix
                 */
                strcpy(wl0_ssid, wl1_ssid);
                strcpy(&wl0_ssid[MAX_SSID_LEN - strlen(ssid_suffix)], ssid_suffix);
            }
            nvram_set("wl0_ssid", wl0_ssid);
        }
        else
            printf("Error! unknown external register!\n");
    }
    /*  added end pling 03/05/2010 */

    nvram_set("wla_ssid", nvram_safe_get("wl0_ssid"));
    nvram_set("wla_temp_ssid", nvram_safe_get("wl0_ssid"));

    if ( strncmp(nvram_safe_get("wl0_akm"), "psk psk2", 7) == 0 )
    {
        nvram_set("wla_secu_type", "WPA-AUTO-PSK");
        nvram_set("wla_temp_secu_type", "WPA-AUTO-PSK");
        nvram_set("wla_passphrase", nvram_safe_get("wl0_wpa_psk"));

        /* If router changes from 'unconfigured' to 'configured' state by
         * adding a WPS client, the wsc_randomssid and wsc_randomkey will
         * be set. In this case, router should use mixedmode security.
         */

        if (!nvram_match("wps_randomssid", "") ||
            !nvram_match("wps_randomkey", ""))
        {
            nvram_set("wla_secu_type", "WPA-AUTO-PSK");
            nvram_set("wla_temp_secu_type", "WPA-AUTO-PSK");

            nvram_set("wl0_akm", "psk psk2 ");
            nvram_set("wl0_crypto", "tkip+aes");

            nvram_set("wps_mixedmode", "2");
            //nvram_set("wps_randomssid", "");
            //nvram_set("wps_randomkey", "");
            config_flag = 1;
            /* Since we changed to mixed mode, 
             * so we need to disable WDS if it is already enabled
             */
            if (nvram_match("wla_wds_enable", "1"))
            {
                nvram_set("wla_wds_enable",  "0");
                nvram_set("wl0_wds", "");
                nvram_set("wl0_mode", "ap");
            }
        }
        else
        {
            /*  added start pling 02/25/2007 */
            /* Disable WDS if it is already enabled */
            if (nvram_match("wla_wds_enable", "1"))
            {
                nvram_set("wla_wds_enable",  "0");
                nvram_set("wl0_wds", "");
                nvram_set("wl0_mode", "ap");
            }
            /*  added end pling 02/25/2007 */
        }
    }
    else if ( strncmp(nvram_safe_get("wl0_akm"), "psk2", 4) == 0 )
    {
        nvram_set("wla_secu_type", "WPA2-PSK");
        nvram_set("wla_temp_secu_type", "WPA2-PSK");
        nvram_set("wla_passphrase", nvram_safe_get("wl0_wpa_psk"));


        /* If router changes from 'unconfigured' to 'configured' state by
         * adding a WPS client, the wsc_randomssid and wsc_randomkey will
         * be set. In this case, router should use mixedmode security.
         */
        /*  added start pling 06/15/2010 */
        if (nvram_match("wl0_crypto", "tkip"))
        {
            /* DTM fix: 
             * Registrar may set to WPA2-PSK TKIP mode.
             * In this case, don't try to modify the
             * security type.
            */
            nvram_unset("wps_mixedmode");
        }
        else
        /*  added end pling 06/15/2010 */
        if (!nvram_match("wps_randomssid", "") ||
            !nvram_match("wps_randomkey", ""))
        {
            nvram_set("wla_secu_type", "WPA-AUTO-PSK");
            nvram_set("wla_temp_secu_type", "WPA-AUTO-PSK");

            nvram_set("wl0_akm", "psk psk2 ");
            nvram_set("wl0_crypto", "tkip+aes");

            nvram_set("wps_mixedmode", "2");
            //nvram_set("wps_randomssid", "");
            //nvram_set("wps_randomkey", "");
            config_flag = 1;
            /* Since we changed to mixed mode, 
             * so we need to disable WDS if it is already enabled
             */
            if (nvram_match("wla_wds_enable", "1"))
            {
                nvram_set("wla_wds_enable",  "0");
                nvram_set("wl0_wds", "");
                nvram_set("wl0_mode", "ap");
            }
        }
        else
        {
            /*  added start pling 02/25/2007 */
            /* Disable WDS if it is already enabled */
            if (nvram_match("wla_wds_enable", "1"))
            {
                nvram_set("wla_wds_enable",  "0");
                nvram_set("wl0_wds", "");
                nvram_set("wl0_mode", "ap");
            }
            /*  added end pling 02/25/2007 */
        }
    }
    else if ( strncmp(nvram_safe_get("wl0_akm"), "psk", 3) == 0 )
    {
        nvram_set("wla_secu_type", "WPA-PSK");
        nvram_set("wla_temp_secu_type", "WPA-PSK");
        nvram_set("wla_passphrase", nvram_safe_get("wl0_wpa_psk"));

        /* If router changes from 'unconfigured' to 'configured' state by
         * adding a WPS client, the wsc_randomssid and wsc_randomkey will
         * be set. In this case, router should use mixedmode security.
         */
        /*  added start pling 06/15/2010 */
        if (nvram_match("wl0_crypto", "aes"))
        {
            /* DTM fix: 
             * Registrar may set to WPA-PSK AES mode.
             * In this case, don't try to modify the
             * security type.
            */
            nvram_unset("wps_mixedmode");
        }
        else
        /*  added end pling 06/15/2010 */
        if (!nvram_match("wps_randomssid", "") ||
            !nvram_match("wps_randomkey", ""))
        {
            /*  add start, Tony W.Y. Wang, 11/30/2009 */
            /* WiFi TKIP changes for WNDR3400*/
            /*
            When external registrar configures our router as WPA-PSK [TKIP], security, 
            we auto change the wireless mode to Up to 54Mbps. This should only apply to
            router when router is in "WPS Unconfigured" state.
            */
            nvram_set("wla_mode",  "g and b");

            /* Disable 11n support, copied from bcm_wlan_util.c */
            acosNvramConfig_set("wl_nmode", "0");
            acosNvramConfig_set("wl0_nmode", "0");

            acosNvramConfig_set("wl_gmode", "1");
            acosNvramConfig_set("wl0_gmode", "1");

            /* Set bandwidth to 20MHz */
#if ( !(defined BCM4718) && !(defined BCM4716) && !(defined R6300v2) && !defined(R6250))
            acosNvramConfig_set("wl_nbw", "20");
            acosNvramConfig_set("wl0_nbw", "20");
#endif
        
            acosNvramConfig_set("wl_nbw_cap", "0");
            acosNvramConfig_set("wl0_nbw_cap", "0");

            /* Disable extension channel */
            acosNvramConfig_set("wl_nctrlsb", "none");
            acosNvramConfig_set("wl0_nctrlsb", "none");

            /* Now set the security */
            nvram_set("wla_secu_type", "WPA-PSK");
            nvram_set("wla_temp_secu_type", "WPA-PSK");

            nvram_set("wl0_akm", "psk ");
            nvram_set("wl0_crypto", "tkip");

            /*
            nvram_set("wla_secu_type", "WPA-AUTO-PSK");
            nvram_set("wla_temp_secu_type", "WPA-AUTO-PSK");

            nvram_set("wl0_akm", "psk psk2 ");
            nvram_set("wl0_crypto", "tkip+aes");
            */
            /*  add end, Tony W.Y. Wang, 11/30/2009 */
            nvram_set("wps_mixedmode", "1");
            //nvram_set("wps_randomssid", "");
            //nvram_set("wps_randomkey", "");
            config_flag = 1;
            /* Since we changed to mixed mode, 
             * so we need to disable WDS if it is already enabled
             */
            if (nvram_match("wla_wds_enable", "1"))
            {
                nvram_set("wla_wds_enable",  "0");
                nvram_set("wl0_wds", "");
                nvram_set("wl0_mode", "ap");
            }
        }
    }
    else if ( strncmp(nvram_safe_get("wl0_wep"), "enabled", 7) == 0 )
    {
        int key_len=0;
        if ( strncmp(nvram_safe_get("wl0_auth"), "1", 1) == 0 ) /*shared mode*/
        {
            nvram_set("wla_auth_type", "sharedkey");
            nvram_set("wla_temp_auth_type", "sharedkey");
        }
        else
        {
            nvram_set("wla_auth_type", "opensystem");
            nvram_set("wla_temp_auth_type", "opensystem");
        }
        
        nvram_set("wla_secu_type", "WEP");
        nvram_set("wla_temp_secu_type", "WEP");
        nvram_set("wla_defaKey", "0");
        nvram_set("wla_temp_defaKey", "0");
        /*  add start by aspen Bai, 02/24/2009 */
        /*
        nvram_set("wla_key1", nvram_safe_get("wl_key1"));
        nvram_set("wla_temp_key1", nvram_safe_get("wl_key1"));
        
        printf("wla_wep_length: %d\n", strlen(nvram_safe_get("wl_key1")));
        
        key_len = atoi(nvram_safe_get("wl_key1"));
        */
        nvram_set("wla_key1", nvram_safe_get("wl0_key1"));
        nvram_set("wla_temp_key1", nvram_safe_get("wl0_key1"));
        
        printf("wla_wep_length: %d\n", strlen(nvram_safe_get("wl0_key1")));
        
        key_len = strlen(nvram_safe_get("wl0_key1"));
        /*  add end by aspen Bai, 02/24/2009 */
        if (key_len==5 || key_len==10)
        {
            nvram_set("wla_wep_length", "1");
        }
        else
        {
            nvram_set("wla_wep_length", "2");
        }
        /*  add start by aspen Bai, 02/24/2009 */
        if (key_len==5 || key_len==13)
        {
            char HexKeyArray[32];
            char key[32], tmp[32];
            int i;
            
            strcpy(key, nvram_safe_get("wl0_key1"));
            memset(HexKeyArray, 0, sizeof(HexKeyArray));
            for (i=0; i<key_len; i++)
            {
                sprintf(tmp, "%02X", (unsigned char)key[i]);
                strcat(HexKeyArray, tmp);
            }
            printf("ASCII WEP key (%s) convert -> HEX WEP key (%s)\n", key, HexKeyArray);
            
            nvram_set("wla_key1", HexKeyArray);
            nvram_set("wla_temp_key1", HexKeyArray);
        }
        /*  add end by aspen Bai, 02/24/2009 */
    }
    else
    {
        nvram_set("wla_secu_type", "None");
        nvram_set("wla_temp_secu_type", "None");
        nvram_set("wla_passphrase", "");
    }
    /*  add start, Tony W.Y. Wang, 11/23/2009 */
    nvram_set("wlg_ssid", nvram_safe_get("wl1_ssid"));
    nvram_set("wlg_temp_ssid", nvram_safe_get("wl1_ssid"));

    if ( strncmp(nvram_safe_get("wl1_akm"), "psk psk2", 7) == 0 )
    {
        nvram_set("wlg_secu_type", "WPA-AUTO-PSK");
        nvram_set("wlg_temp_secu_type", "WPA-AUTO-PSK");
        nvram_set("wlg_passphrase", nvram_safe_get("wl1_wpa_psk"));

        /* If router changes from 'unconfigured' to 'configured' state by
         * adding a WPS client, the wsc_randomssid and wsc_randomkey will
         * be set. In this case, router should use mixedmode security.
         */

        if (!nvram_match("wps_randomssid", "") ||
            !nvram_match("wps_randomkey", ""))
        {
            nvram_set("wlg_secu_type", "WPA-AUTO-PSK");
            nvram_set("wlg_temp_secu_type", "WPA-AUTO-PSK");

            nvram_set("wl1_akm", "psk psk2 ");
            nvram_set("wl1_crypto", "tkip+aes");

            nvram_set("wps_mixedmode", "2");
            //nvram_set("wps_randomssid", "");
            //nvram_set("wps_randomkey", "");
            config_flag = 1;
            /* Since we changed to mixed mode, 
             * so we need to disable WDS if it is already enabled
             */
            if (nvram_match("wlg_wds_enable", "1"))
            {
                nvram_set("wlg_wds_enable",  "0");
                nvram_set("wl1_wds", "");
                nvram_set("wl1_mode", "ap");
            }
        }
        else
        {
            /*  added start pling 02/25/2007 */
            /* Disable WDS if it is already enabled */
            if (nvram_match("wlg_wds_enable", "1"))
            {
                nvram_set("wlg_wds_enable",  "0");
                nvram_set("wl1_wds", "");
                nvram_set("wl1_mode", "ap");
            }
            /*  added end pling 02/25/2007 */
        }
    }
    else if ( strncmp(nvram_safe_get("wl1_akm"), "psk2", 4) == 0 )
    {
        nvram_set("wlg_secu_type", "WPA2-PSK");
        nvram_set("wlg_temp_secu_type", "WPA2-PSK");
        nvram_set("wlg_passphrase", nvram_safe_get("wl1_wpa_psk"));


        /* If router changes from 'unconfigured' to 'configured' state by
         * adding a WPS client, the wsc_randomssid and wsc_randomkey will
         * be set. In this case, router should use mixedmode security.
         */

        /*  added start pling 06/15/2010 */
        if (nvram_match("wl1_crypto", "tkip"))
        {
            /* DTM fix: 
             * Registrar may set to WPA2-PSK TKIP mode.
             * In this case, don't try to modify the
             * security type.
            */
            nvram_unset("wps_mixedmode");
        }
        else
        /*  added end pling 06/15/2010 */
        if (!nvram_match("wps_randomssid", "") ||
            !nvram_match("wps_randomkey", ""))
        {
            nvram_set("wlg_secu_type", "WPA-AUTO-PSK");
            nvram_set("wlg_temp_secu_type", "WPA-AUTO-PSK");

            nvram_set("wl1_akm", "psk psk2 ");
            nvram_set("wl1_crypto", "tkip+aes");

            nvram_set("wps_mixedmode", "2");
            //nvram_set("wps_randomssid", "");
            //nvram_set("wps_randomkey", "");
            config_flag = 1;
            /* Since we changed to mixed mode, 
             * so we need to disable WDS if it is already enabled
             */
            if (nvram_match("wlg_wds_enable", "1"))
            {
                nvram_set("wlg_wds_enable",  "0");
                nvram_set("wl1_wds", "");
                nvram_set("wl1_mode", "ap");
            }
        }
        else
        {
            /*  added start pling 02/25/2007 */
            /* Disable WDS if it is already enabled */
            if (nvram_match("wlg_wds_enable", "1"))
            {
                nvram_set("wlg_wds_enable",  "0");
                nvram_set("wl1_wds", "");
                nvram_set("wl1_mode", "ap");
            }
            /*  added end pling 02/25/2007 */
        }
    }
    else if ( strncmp(nvram_safe_get("wl1_akm"), "psk", 3) == 0 )
    {
        nvram_set("wlg_secu_type", "WPA-PSK");
        nvram_set("wlg_temp_secu_type", "WPA-PSK");
        nvram_set("wlg_passphrase", nvram_safe_get("wl1_wpa_psk"));

        /* If router changes from 'unconfigured' to 'configured' state by
         * adding a WPS client, the wsc_randomssid and wsc_randomkey will
         * be set. In this case, router should use mixedmode security.
         */

        /*  added start pling 06/15/2010 */
        if (nvram_match("wl1_crypto", "aes"))
        {
            /* DTM fix: 
             * Registrar may set to WPA-PSK AES mode.
             * In this case, don't try to modify the
             * security type.
            */
            nvram_unset("wps_mixedmode");
        }
        else
        /*  added end pling 06/15/2010 */
        if (!nvram_match("wps_randomssid", "") ||
            !nvram_match("wps_randomkey", ""))
        {
            /*  add start, Tony W.Y. Wang, 11/30/2009 */
            /* WiFi TKIP changes for WNDR3400*/
            /*
            When external registrar configures our router as WPA-PSK [TKIP], security, 
            we auto change the wireless mode to Up to 54Mbps. This should only apply to
            router when router is in "WPS Unconfigured" state.
            */
            nvram_set("wlg_mode",  "g and b");

            /* Disable 11n support, copied from bcm_wlan_util.c */
            acosNvramConfig_set("wl1_nmode", "0");

            acosNvramConfig_set("wl1_gmode", "1");

            /* Set bandwidth to 20MHz */
#if ( !(defined BCM4718) && !(defined BCM4716) && !(defined R6300v2) && !defined(R6250))
            acosNvramConfig_set("wl1_nbw", "20");
#endif
        
            acosNvramConfig_set("wl1_nbw_cap", "0");

            /* Disable extension channel */
            acosNvramConfig_set("wl1_nctrlsb", "none");

            /* Now set the security */
            nvram_set("wlg_secu_type", "WPA-PSK");
            nvram_set("wlg_temp_secu_type", "WPA-PSK");

            nvram_set("wl1_akm", "psk ");
            nvram_set("wl1_crypto", "tkip");
            /*
            nvram_set("wlg_secu_type", "WPA-AUTO-PSK");
            nvram_set("wlg_temp_secu_type", "WPA-AUTO-PSK");

            nvram_set("wl1_akm", "psk psk2 ");
            nvram_set("wl1_crypto", "tkip+aes");
            */
            /*  add end, Tony W.Y. Wang, 11/30/2009 */
            nvram_set("wps_mixedmode", "1");
            //nvram_set("wps_randomssid", "");
            //nvram_set("wps_randomkey", "");
            config_flag = 1;
            /* Since we changed to mixed mode, 
             * so we need to disable WDS if it is already enabled
             */
            if (nvram_match("wlg_wds_enable", "1"))
            {
                nvram_set("wlg_wds_enable",  "0");
                nvram_set("wl1_wds", "");
                nvram_set("wl1_mode", "ap");
            }
        }
    }
    else if ( strncmp(nvram_safe_get("wl1_wep"), "enabled", 7) == 0 )
    {
        int key_len=0;
        if ( strncmp(nvram_safe_get("wl1_auth"), "1", 1) == 0 ) /*shared mode*/
        {
            nvram_set("wlg_auth_type", "sharedkey");
            nvram_set("wlg_temp_auth_type", "sharedkey");
        }
        else
        {
            nvram_set("wlg_auth_type", "opensystem");
            nvram_set("wlg_temp_auth_type", "opensystem");
        }
        
        nvram_set("wlg_secu_type", "WEP");
        nvram_set("wlg_temp_secu_type", "WEP");
        nvram_set("wlg_defaKey", "0");
        nvram_set("wlg_temp_defaKey", "0");
        /*  add start by aspen Bai, 02/24/2009 */
        /*
        nvram_set("wla_key1", nvram_safe_get("wl_key1"));
        nvram_set("wla_temp_key1", nvram_safe_get("wl_key1"));
        
        printf("wla_wep_length: %d\n", strlen(nvram_safe_get("wl_key1")));
        
        key_len = atoi(nvram_safe_get("wl_key1"));
        */
        nvram_set("wlg_key1", nvram_safe_get("wl1_key1"));
        nvram_set("wlg_temp_key1", nvram_safe_get("wl1_key1"));
        
        printf("wlg_wep_length: %d\n", strlen(nvram_safe_get("wl1_key1")));
        
        key_len = strlen(nvram_safe_get("wl1_key1"));
        /*  add end by aspen Bai, 02/24/2009 */
        if (key_len==5 || key_len==10)
        {
            nvram_set("wlg_wep_length", "1");
        }
        else
        {
            nvram_set("wlg_wep_length", "2");
        }
        /*  add start by aspen Bai, 02/24/2009 */
        if (key_len==5 || key_len==13)
        {
            char HexKeyArray[32];
            char key[32], tmp[32];
            int i;
            
            strcpy(key, nvram_safe_get("wl1_key1"));
            memset(HexKeyArray, 0, sizeof(HexKeyArray));
            for (i=0; i<key_len; i++)
            {
                sprintf(tmp, "%02X", (unsigned char)key[i]);
                strcat(HexKeyArray, tmp);
            }
            printf("ASCII WEP key (%s) convert -> HEX WEP key (%s)\n", key, HexKeyArray);
            
            nvram_set("wlg_key1", HexKeyArray);
            nvram_set("wlg_temp_key1", HexKeyArray);
        }
        /*  add end by aspen Bai, 02/24/2009 */
    }
    else
    {
        nvram_set("wlg_secu_type", "None");
        nvram_set("wlg_temp_secu_type", "None");
        nvram_set("wlg_passphrase", "");
    }
    
    if (config_flag == 1)
    {
        //nvram_set("wps_randomssid", "");
        //nvram_set("wps_randomkey", "");
        nvram_set("wl0_wps_config_state", "1");
        nvram_set("wl1_wps_config_state", "1");
    }
    /*  add end, Tony W.Y. Wang, 11/23/2009 */
    nvram_set("allow_registrar_config", "0");  /*  added pling, 05/16/2007 */

    /*  added start pling 02/25/2008 */
    /* 'wl_unit' is changed to "0.-1" after Vista configure router (using Borg DTM1.3 patch).
     * This will make WPS fail to work on the correct interface.
     * Set it back to "0" if it is not.
     */
    if (!nvram_match("wl_unit", "0"))
        nvram_set("wl_unit", "0");
    /*  added end pling 02/25/2008 */
}
/*  added end by EricHuang, 12/13/2006 */

/*  added start wklin, 11/02/2006 */
static void save_wlan_time(void)
{
    struct sysinfo info;
    char command[128];
    sysinfo(&info);
    sprintf(command, "echo %lu > /tmp/wlan_time", info.uptime);
    system(command);
    return;
}
/*  added end, wklin, 11/02/2006 */

/*  added start, zacker, 01/13/2012, @iptv_igmp */
#ifdef CONFIG_RUSSIA_IPTV
static int config_iptv_params(void)
{
    unsigned int iptv_bridge_intf = 0x00;
    char vlan1_ports[16] = "";
    char vlan_iptv_ports[16] = "";
    /*added by dennis start,05/04/2012,for guest network reconnect issue*/
    char br0_ifnames[64]="";
    char if_name[16]="";
    char wl_param[16]="";
    char command[128]="";
    int i = 0;
    /*added by dennis end,05/04/2012,for guest network reconnect issue*/
    

    if (nvram_match(NVRAM_IPTV_ENABLED, "1"))
    {
        char iptv_intf[32];

        strcpy(iptv_intf, nvram_safe_get(NVRAM_IPTV_INTF));
        sscanf(iptv_intf, "0x%02X", &iptv_bridge_intf);
    }

    /*  modified start pling 04/03/2012 */
    /* Swap LAN1 ~ LAN4 due to reverse labeling */
    if (iptv_bridge_intf & IPTV_LAN1)
        strcat(vlan_iptv_ports, "3 ");
    else
        strcat(vlan1_ports, "3 ");

    if (iptv_bridge_intf & IPTV_LAN2)   /*  modified pling 02/09/2012, fix a typo */
        strcat(vlan_iptv_ports, "2 ");
    else
        strcat(vlan1_ports, "2 ");

    if (iptv_bridge_intf & IPTV_LAN3)
        strcat(vlan_iptv_ports, "1 ");
    else
        strcat(vlan1_ports, "1 ");
    
    if (iptv_bridge_intf & IPTV_LAN4)
        strcat(vlan_iptv_ports, "0 ");
    else
        strcat(vlan1_ports, "0 ");
    /*  modified end pling 04/03/2012 */

    strcat(vlan1_ports, "5*");
    nvram_set("vlan1ports", vlan1_ports);

    /* build vlan3 for IGMP snooping on IPTV ports */
    if (strlen(vlan_iptv_ports))
    {
        strcat(vlan_iptv_ports, "5");
        nvram_set("vlan3ports", vlan_iptv_ports);
        nvram_set("vlan3hwname", nvram_safe_get("vlan2hwname"));
    }
    else
    {
        nvram_unset("vlan3ports");
        nvram_unset("vlan3hwname");
    }

    if (iptv_bridge_intf & IPTV_MASK)
    {
        char lan_ifnames[32] = "vlan1 ";
        char wan_ifnames[32] = "vlan2 ";
    
#ifdef __CONFIG_IGMP_SNOOPING__
        /* always enable snooping for IPTV */
        nvram_set("emf_enable", "1");
#endif

        /* always build vlan2 and br1 and enable vlan tag output for all vlan */
        nvram_set("vlan2ports", "4 5");

        /* build vlan3 for IGMP snooping on IPTV ports */
        if (strlen(vlan_iptv_ports))
            strcat(wan_ifnames, "vlan3 ");

        if (iptv_bridge_intf & IPTV_WLAN1)
            strcat(wan_ifnames, "eth1 ");
        else
            strcat(lan_ifnames, "eth1 ");

        if (iptv_bridge_intf & IPTV_WLAN2)
            strcat(wan_ifnames, "eth2 ");
        else
            strcat(lan_ifnames, "eth2 ");

        

        //nvram_set("lan_ifnames", lan_ifnames);
        strcpy(br0_ifnames,lan_ifnames);
        nvram_set("wan_ifnames", wan_ifnames);
        nvram_set("lan1_ifnames", wan_ifnames);

        nvram_set("wan_ifname", "br1");
        nvram_set("lan1_ifname", "br1");
    }
    else
    {
        
        //nvram_set("lan_ifnames", "vlan1 eth1 eth2 wl0.1");
        /*modified by dennis start, 05/03/2012,fixed guest network cannot reconnect issue*/
        strcpy(br0_ifnames,"vlan1 eth1 eth2");       
        /*modified by dennis end, 05/03/2012,fixed guest network cannot reconnect issue*/
        nvram_set("lan1_ifnames", "");
        nvram_set("lan1_ifname", "");

#ifdef __CONFIG_IGMP_SNOOPING__
        if (nvram_match("emf_enable", "1"))
        {
            nvram_set("vlan2ports", "4 5");
            nvram_set("wan_ifnames", "vlan2 ");
            nvram_set("wan_ifname", "vlan2");
        }
        else
#endif
        {
            nvram_set("vlan2ports", "4 5u");
            nvram_set("wan_ifnames", "eth0 ");
            nvram_set("wan_ifname", "eth0");

        }
    }

     /*added by dennis start, 05/03/2012,fixed guest network cannot reconnect issue*/
     for(i = MIN_BSSID_NUM; i <= MAX_BSSID_NUM; i++){
        sprintf(wl_param, "%s_%d", "wla_sec_profile_enable", i);     
        if(nvram_match(wl_param, "1")){
            sprintf(if_name, "wl0.%d", i-1);
            strcat(br0_ifnames, " ");
            strcat(br0_ifnames, if_name);
        }
     }

     for(i = MIN_BSSID_NUM; i <= MAX_BSSID_NUM; i++){
         sprintf(wl_param, "%s_%d", "wlg_sec_profile_enable", i);        
         if(nvram_match(wl_param, "1")){
             sprintf(if_name, "wl1.%d", i-1);
             strcat(br0_ifnames, " ");
             strcat(br0_ifnames, if_name);
         }
     }
     nvram_set("lan_ifnames", br0_ifnames);
    /*added by dennis start, 05/03/2012,fixed guest network cannot reconnect issue*/
	/*  added start pling 08/17/2012 */
    /* Fix: When IPTV is enabled, WAN interface is "br1".
     * This can cause CTF/pktc to work abnormally.
     * So bypass CTF/pktc altogether */
    if (nvram_match(NVRAM_IPTV_ENABLED, "1"))
        eval("et", "robowr", "0xFFFF", "0xFB", "1");
    else
        eval("et", "robowr", "0xFFFF", "0xFB", "0");
    /*  added end pling 08/17/2012 */
    return 0;
}

static int active_vlan(void)
{
    char buf[128];
    unsigned char mac[ETHER_ADDR_LEN];
    char eth0_mac[32];

    strcpy(eth0_mac, nvram_safe_get("et0macaddr"));
    ether_atoe(eth0_mac, mac);

    /* Set MAC address byte 0 */
    sprintf(buf, "et robowr 0x%04X 0x%02X 0x%02X%02X", VCFG_PAGE, VCFG_REG, MAC_BYTE0, mac[0]);
    system(buf);
    /* Set MAC address byte 1 */
    sprintf(buf, "et robowr 0x%04X 0x%02X 0x%02X%02X", VCFG_PAGE, VCFG_REG, MAC_BYTE1, mac[1]);
    system(buf);
    /* Set MAC address byte 2 */
    sprintf(buf, "et robowr 0x%04X 0x%02X 0x%02X%02X", VCFG_PAGE, VCFG_REG, MAC_BYTE2, mac[2]);
    system(buf);
    /* Set MAC address byte 3 */
    sprintf(buf, "et robowr 0x%04X 0x%02X 0x%02X%02X", VCFG_PAGE, VCFG_REG, MAC_BYTE3, mac[3]);
    system(buf);
    /* Set MAC address byte 4 */
    sprintf(buf, "et robowr 0x%04X 0x%02X 0x%02X%02X", VCFG_PAGE, VCFG_REG, MAC_BYTE4, mac[4]);
    system(buf);
    /* Set MAC address byte 5 */
    sprintf(buf, "et robowr 0x%04X 0x%02X 0x%02X%02X", VCFG_PAGE, VCFG_REG, MAC_BYTE5, mac[5]);
    system(buf);
    /* Issue command to activate new vlan configuration. */
    sprintf(buf, "et robowr 0x%04X 0x%02X 0x%02X00", VCFG_PAGE, VCFG_REG, SET_VLAN);
    system(buf);

    return 0;
}
#endif

#if (defined INCLUDE_QOS) || (defined __CONFIG_IGMP_SNOOPING__)
/* these settings are for BCM53115S switch */
static int config_switch_reg(void)
{
    if (
#if (defined __CONFIG_IGMP_SNOOPING__)
        nvram_match("emf_enable", "1") ||
#endif
#if defined(CONFIG_RUSSIA_IPTV)
		nvram_match("iptv_enabled", "1") ||
#endif
        (nvram_match("qos_enable", "1") 
        && !nvram_match("wla_repeater", "1")
#if (defined INCLUDE_DUAL_BAND)
        && !nvram_match("wlg_repeater", "1")
#endif
        && !nvram_match("qos_port", "")))
    {
        /* Enables the receipt of unicast, multicast and broadcast on IMP port */
        system("et robowr 0x00 0x08 0x1C");
        /* Enable Frame-managment mode */
        system("et robowr 0x00 0x0B 0x07");
        /* Enable management port */
        system("et robowr 0x02 0x00 0x80");
#ifdef BCM5301X           
        /*Enable BRCM header for port 5*/
        system("et robowr 0x02 0x03 0x02");  
#endif        
        /* CRC bypass and auto generation */
        system("et robowr 0x34 0x06 0x11");
#if (defined __CONFIG_IGMP_SNOOPING__)
        if (nvram_match("emf_enable", "1"))
        {
            /* Set IMP port default tag id */
            system("et robowr 0x34 0x20 0x02");
            /* Enable IPMC bypass V fwdmap */
            system("et robowr 0x34 0x01 0x2E");
            /* Set Multiport address enable */
            system("et robowr 0x04 0x0E 0x0AAA");
        }
#endif
        /* Turn on the flags for kernel space (et/emf/igs) handling */
        system("et robowr 0xFFFF 0xFE 0x03");
    }
    else
    {
        system("et robowr 0x00 0x08 0x00");
        system("et robowr 0x00 0x0B 0x06");
        system("et robowr 0x02 0x00 0x00");
#ifdef BCM5301X          
        /*Enable BRCM header for port 8*/
        system("et robowr 0x02 0x03 0x01"); 
#endif        
        system("et robowr 0x34 0x06 0x10");
#if (defined __CONFIG_IGMP_SNOOPING__)
        system("et robowr 0x34 0x20 0x02");
        system("et robowr 0x34 0x01 0x0E");
        system("et robowr 0x04 0x0E 0x0000");
#endif
        if (nvram_match("qos_enable", "1"))
            system("et robowr 0xFFFF 0xFE 0x01");
        else if (!nvram_match("qos_port", ""))
            system("et robowr 0xFFFF 0xFE 0x02");
        else
            system("et robowr 0xFFFF 0xFE 0x00");
    }

    return 0;
}
/*  added end, zacker, 01/13/2012, @iptv_igmp */

/*  modified start, zacker, 01/13/2012, @iptv_igmp */
static void config_switch(void)
{
    /* BCM5325 & BCM53115 switch request to change these vars
     * to output ethernet port tag/id in packets.
     */
    struct nvram_tuple generic[] = {
        { "wan_ifname", "eth0", 0 },
        { "wan_ifnames", "eth0 ", 0 },
        { "vlan1ports", "0 1 2 3 5*", 0 },
        { "vlan2ports", "4 5u", 0 },
        { 0, 0, 0 }
    };

    struct nvram_tuple vlan[] = {
        { "wan_ifname", "vlan2", 0 },
        { "wan_ifnames", "vlan2 ", 0 },
        { "vlan1ports", "0 1 2 3 5*", 0 },
        { "vlan2ports", "4 5", 0 },
        { 0, 0, 0 }
    };

    struct nvram_tuple *u = generic;
    int commit = 0;

    if (nvram_match("emf_enable", "1")) {
        u = vlan;
    }

    /* don't need vlan in repeater mode */
    if (nvram_match("wla_repeater", "1")
#if (defined INCLUDE_DUAL_BAND)
        || nvram_match("wlg_repeater", "1")
#endif
        ) {
        u = generic;
    }

    for ( ; u && u->name; u++) {
        if (strcmp(nvram_safe_get(u->name), u->value)) {
            commit = 1;
            nvram_set(u->name, u->value);
        }
    }

    if (commit) {
        cprintf("Commit new ethernet config...\n");
        nvram_commit();
        commit = 0;
    }
}
#endif
/*  modified end, zacker, 01/13/2012, @iptv_igmp */

/*  modified start, zacker, 01/04/2011 */
static int should_stop_wps(void)
{
    /* WPS LED OFF */
    if (nvram_match("wla_wlanstate","Disable")
#if (defined INCLUDE_DUAL_BAND)
        && nvram_match("wlg_wlanstate","Disable")
#endif
       )
        return WPS_LED_STOP_RADIO_OFF;

    /* WPS LED quick blink for 5sec */
    if (nvram_match("wps_mode", "disabled")
        || nvram_match("wla_repeater", "1")
#if (defined INCLUDE_DUAL_BAND)
        || nvram_match("wlg_repeater", "1")
#endif
       )
        return WPS_LED_STOP_DISABLED;

    /* WPS LED original action */
    return WPS_LED_STOP_NO;
}

static int is_secure_wl(void)
{
    /* for ACR5500 , there is only on WiFi LED for WPS */
#if defined(R6300v2) || defined(R6250)

    if (acosNvramConfig_match("wla_wlanstate","Disable")
        && acosNvramConfig_match("wlg_wlanstate","Disable") )
        return 0;

    return 1;
#else    
    
    if (   (!acosNvramConfig_match("wla_secu_type", "None")
            && acosNvramConfig_match("wla_wlanstate","Enable"))
#if (defined INCLUDE_DUAL_BAND)
        || (!acosNvramConfig_match("wlg_secu_type", "None")
            && acosNvramConfig_match("wlg_wlanstate","Enable"))
#endif
        )
        return 1;

    return 0;
#endif /* defined(R6300v2) */    
}

/*  added start, Wins, 04/20/2011 @RU_IPTV */
#ifdef CONFIG_RUSSIA_IPTV
static int is_russia_specific_support (void)
{
    int result = 0;
    char sku_name[8];

    /* Router Spec v2.0:                                                        *
     *   Case 1: RU specific firmware.                                          *
     *   Case 2: single firmware & region code is RU.                           *
     *   Case 3: WW firmware & GUI language is Russian.                         *
     *   Case 4: single firmware & region code is WW & GUI language is Russian. *
     * Currently, new built firmware will be single firmware.                   */
    strcpy(sku_name, nvram_get("sku_name"));
    if (!strcmp(sku_name, "RU"))
    {
        /* Case 2: single firmware & region code is RU. */
        /* Region is RU (0x0005) */
        result = 1;
    }
    else if (!strcmp(sku_name, "WW"))
    {
        /* Region is WW (0x0002) */
        char gui_region[16];
        strcpy(gui_region, nvram_get("gui_region"));
        if (!strcmp(gui_region, "Russian"))
        {
            /* Case 4: single firmware & region code is WW & GUI language is Russian */
            /* GUI language is Russian */
            result = 1;
        }
    }

    return result;
}
#endif /* CONFIG_RUSSIA_IPTV */
/*  added end, Wins, 04/20/2011 @RU_IPTV */

static int send_wps_led_cmd(int cmd, int arg)
{
    int ret_val=0;
    int fd;

    fd = open(DEV_WPS_LED, O_RDWR);
    if (fd < 0) 
        return -1;

    if (is_secure_wl())
        arg = 1;
    else
        arg = 0;

    switch (should_stop_wps())
    {
        case WPS_LED_STOP_RADIO_OFF:
            cmd = WPS_LED_BLINK_OFF;
            break;
            
        case WPS_LED_STOP_DISABLED:
            if (cmd == WPS_LED_BLINK_NORMAL)
                cmd = WPS_LED_BLINK_QUICK;
            break;
            
        case WPS_LED_STOP_NO:
        default:
            break;
    }

    ret_val = ioctl(fd, cmd, arg);
    close(fd);

    return ret_val;
}
/*  modified end, zacker, 01/04/2011 */

static int
build_ifnames(char *type, char *names, int *size)
{
	char name[32], *next;
	int len = 0;
	int s;

	/* open a raw scoket for ioctl */
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return -1;

	/*
	 * go thru all device names (wl<N> il<N> et<N> vlan<N>) and interfaces to
	 * build an interface name list in which each i/f name coresponds to a device
	 * name in device name list. Interface/device name matching rule is device
	 * type dependant:
	 *
	 *	wl:	by unit # provided by the driver, for example, if eth1 is wireless
	 *		i/f and its unit # is 0, then it will be in the i/f name list if
	 *		wl0 is in the device name list.
	 *	il/et:	by mac address, for example, if et0's mac address is identical to
	 *		that of eth2's, then eth2 will be in the i/f name list if et0 is
	 *		in the device name list.
	 *	vlan:	by name, for example, vlan0 will be in the i/f name list if vlan0
	 *		is in the device name list.
	 */
	foreach(name, type, next) {
		struct ifreq ifr;
		int i, unit;
		char var[32], *mac;
		unsigned char ea[ETHER_ADDR_LEN];

		/* vlan: add it to interface name list */
		if (!strncmp(name, "vlan", 4)) {
			/* append interface name to list */
			len += snprintf(&names[len], *size - len, "%s ", name);
			continue;
		}

		/* others: proceed only when rules are met */
		for (i = 1; i <= DEV_NUMIFS; i ++) {
			/* ignore i/f that is not ethernet */
			ifr.ifr_ifindex = i;
			if (ioctl(s, SIOCGIFNAME, &ifr))
				continue;
			if (ioctl(s, SIOCGIFHWADDR, &ifr))
				continue;
			if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
				continue;
			if (!strncmp(ifr.ifr_name, "vlan", 4))
				continue;

			/* wl: use unit # to identify wl */
			if (!strncmp(name, "wl", 2)) {
				if (wl_probe(ifr.ifr_name) ||
				    wl_ioctl(ifr.ifr_name, WLC_GET_INSTANCE, &unit, sizeof(unit)) ||
				    unit != atoi(&name[2]))
					continue;
			}
			/* et/il: use mac addr to identify et/il */
			else if (!strncmp(name, "et", 2) || !strncmp(name, "il", 2)) {
				snprintf(var, sizeof(var), "%smacaddr", name);
				if (!(mac = nvram_get(var)) || !ether_atoe(mac, ea) ||
				    bcmp(ea, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN))
					continue;
			}
			/* mac address: compare value */
			else if (ether_atoe(name, ea) &&
				!bcmp(ea, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN))
				;
			/* others: ignore */
			else
				continue;

			/* append interface name to list */
			len += snprintf(&names[len], *size - len, "%s ", ifr.ifr_name);
		}
	}

	close(s);

	*size = len;
	return 0;
}

#ifdef __CONFIG_WSCCMD__
static void rc_randomKey()
{
	char random_ssid[33] = {0};
	unsigned char random_key[65] = {0};
	int i = 0;
	unsigned short key_length;

	RAND_bytes((unsigned char *)&key_length, sizeof(key_length));
	key_length = ((((long)key_length + 56791 )*13579)%8) + 8;

	/* random a ssid */
	sprintf(random_ssid, "Broadcom_");

	while (i < 6) {
		RAND_bytes(&random_ssid[9 + i], 1);
		if ((islower(random_ssid[9 + i]) || isdigit(random_ssid[9 + i])) && (random_ssid[9 + i] < 0x7f)) {
			i++;
		}
	}

	nvram_set("wl_ssid", random_ssid);

	i = 0;
	/* random a key */
	while (i < key_length) {
		RAND_bytes(&random_key[i], 1);
		if ((islower(random_key[i]) || isdigit(random_key[i])) && (random_key[i] < 0x7f)) {
			i++;
		}
	}
	random_key[key_length] = 0;

	nvram_set("wl_wpa_psk", random_key);

	/* Set default config to wps-psk, tkip */
	nvram_set("wl_akm", "psk ");
	nvram_set("wl_auth", "0");
	nvram_set("wl_wep", "disabled");
	nvram_set("wl_crypto", "tkip");

}
static void
wps_restore_defaults(void)
{
	/* cleanly up nvram for WPS */
	nvram_unset("wps_seed");
	nvram_unset("wps_config_state");
	nvram_unset("wps_addER");
	nvram_unset("wps_device_pin");
	nvram_unset("wps_pbc_force");
	nvram_unset("wps_config_command");
	nvram_unset("wps_proc_status");
	nvram_unset("wps_status");
	nvram_unset("wps_method");
	nvram_unset("wps_proc_mac");
	nvram_unset("wps_sta_pin");
	nvram_unset("wps_currentband");
	nvram_unset("wps_restart");
	nvram_unset("wps_event");

	nvram_unset("wps_enr_mode");
	nvram_unset("wps_enr_ifname");
	nvram_unset("wps_enr_ssid");
	nvram_unset("wps_enr_bssid");
	nvram_unset("wps_enr_wsec");

	nvram_unset("wps_unit");
}
#endif /* __CONFIG_WPS__ */

static void
virtual_radio_restore_defaults(void)
{
	char tmp[100], prefix[] = "wlXXXXXXXXXX_mssid_";
	int i, j;

	nvram_unset("unbridged_ifnames");
	nvram_unset("ure_disable");

	/* Delete dynamically generated variables */
	for (i = 0; i < MAX_NVPARSE; i++) {
		sprintf(prefix, "wl%d_", i);
		nvram_unset(strcat_r(prefix, "vifs", tmp));
		nvram_unset(strcat_r(prefix, "ssid", tmp));
		nvram_unset(strcat_r(prefix, "guest", tmp));
		nvram_unset(strcat_r(prefix, "ure", tmp));
		nvram_unset(strcat_r(prefix, "ipconfig_index", tmp));
		nvram_unset(strcat_r(prefix, "nas_dbg", tmp));
		sprintf(prefix, "lan%d_", i);
		nvram_unset(strcat_r(prefix, "ifname", tmp));
		nvram_unset(strcat_r(prefix, "ifnames", tmp));
		nvram_unset(strcat_r(prefix, "gateway", tmp));
		nvram_unset(strcat_r(prefix, "proto", tmp));
		nvram_unset(strcat_r(prefix, "ipaddr", tmp));
		nvram_unset(strcat_r(prefix, "netmask", tmp));
		nvram_unset(strcat_r(prefix, "lease", tmp));
		nvram_unset(strcat_r(prefix, "stp", tmp));
		nvram_unset(strcat_r(prefix, "hwaddr", tmp));
		sprintf(prefix, "dhcp%d_", i);
		nvram_unset(strcat_r(prefix, "start", tmp));
		nvram_unset(strcat_r(prefix, "end", tmp));

		/* clear virtual versions */
		for (j = 0; j < 16; j++) {
			sprintf(prefix, "wl%d.%d_", i, j);
			nvram_unset(strcat_r(prefix, "ssid", tmp));
			nvram_unset(strcat_r(prefix, "ipconfig_index", tmp));
			nvram_unset(strcat_r(prefix, "guest", tmp));
			nvram_unset(strcat_r(prefix, "closed", tmp));
			nvram_unset(strcat_r(prefix, "wpa_psk", tmp));
			nvram_unset(strcat_r(prefix, "auth", tmp));
			nvram_unset(strcat_r(prefix, "wep", tmp));
			nvram_unset(strcat_r(prefix, "auth_mode", tmp));
			nvram_unset(strcat_r(prefix, "crypto", tmp));
			nvram_unset(strcat_r(prefix, "akm", tmp));
			nvram_unset(strcat_r(prefix, "hwaddr", tmp));
			nvram_unset(strcat_r(prefix, "bss_enabled", tmp));
			nvram_unset(strcat_r(prefix, "bss_maxassoc", tmp));
			nvram_unset(strcat_r(prefix, "wme_bss_disable", tmp));
			nvram_unset(strcat_r(prefix, "ifname", tmp));
			nvram_unset(strcat_r(prefix, "unit", tmp));
			nvram_unset(strcat_r(prefix, "ap_isolate", tmp));
			nvram_unset(strcat_r(prefix, "macmode", tmp));
			nvram_unset(strcat_r(prefix, "maclist", tmp));
			nvram_unset(strcat_r(prefix, "maxassoc", tmp));
			nvram_unset(strcat_r(prefix, "mode", tmp));
			nvram_unset(strcat_r(prefix, "radio", tmp));
			nvram_unset(strcat_r(prefix, "radius_ipaddr", tmp));
			nvram_unset(strcat_r(prefix, "radius_port", tmp));
			nvram_unset(strcat_r(prefix, "radius_key", tmp));
			nvram_unset(strcat_r(prefix, "key", tmp));
			nvram_unset(strcat_r(prefix, "key1", tmp));
			nvram_unset(strcat_r(prefix, "key2", tmp));
			nvram_unset(strcat_r(prefix, "key3", tmp));
			nvram_unset(strcat_r(prefix, "key4", tmp));
			nvram_unset(strcat_r(prefix, "wpa_gtk_rekey", tmp));
			nvram_unset(strcat_r(prefix, "nas_dbg", tmp));
		}
	}
}

#ifdef __CONFIG_NAT__
static void
auto_bridge(void)
{

	struct nvram_tuple generic[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "eth0 eth2 eth3 eth4", 0 },
		{ "wan_ifname", "eth1", 0 },
		{ "wan_ifnames", "eth1", 0 },
		{ 0, 0, 0 }
	};
#ifdef __CONFIG_VLAN__
	struct nvram_tuple vlan[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "vlan0 eth1 eth2 eth3", 0 },
		{ "wan_ifname", "vlan1", 0 },
		{ "wan_ifnames", "vlan1", 0 },
		{ 0, 0, 0 }
	};
#endif	/* __CONFIG_VLAN__ */
	struct nvram_tuple dyna[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ 0, 0, 0 }
	};
	struct nvram_tuple generic_auto_bridge[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "eth0 eth1 eth2 eth3 eth4", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ 0, 0, 0 }
	};
#ifdef __CONFIG_VLAN__
	struct nvram_tuple vlan_auto_bridge[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "vlan0 vlan1 eth1 eth2 eth3", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ 0, 0, 0 }
	};
#endif	/* __CONFIG_VLAN__ */

	struct nvram_tuple dyna_auto_bridge[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ 0, 0, 0 }
	};

	struct nvram_tuple *linux_overrides;
	struct nvram_tuple *t, *u;
	int auto_bridge = 0, i;
#ifdef __CONFIG_VLAN__
	uint boardflags;
#endif	/* __CONFIG_VLAN_ */
	char *landevs, *wandevs;
	char lan_ifnames[128], wan_ifnames[128];
	char dyna_auto_ifnames[128];
	char wan_ifname[32], *next;
	int len;
	int ap = 0;

	printf(" INFO : enter function auto_bridge()\n");

	if (!strcmp(nvram_safe_get("auto_bridge_action"), "1")) {
		auto_bridge = 1;
		cprintf("INFO: Start auto bridge...\n");
	} else {
		nvram_set("router_disable_auto", "0");
		cprintf("INFO: Start non auto_bridge...\n");
	}

	/* Delete dynamically generated variables */
	if (auto_bridge) {
		char tmp[100], prefix[] = "wlXXXXXXXXXX_";
		for (i = 0; i < MAX_NVPARSE; i++) {

			del_filter_client(i);
			del_forward_port(i);
#if !defined(AUTOFW_PORT_DEPRECATED)
			del_autofw_port(i);
#endif

			snprintf(prefix, sizeof(prefix), "wan%d_", i);
			for (t = router_defaults; t->name; t ++) {
				if (!strncmp(t->name, "wan_", 4))
					nvram_unset(strcat_r(prefix, &t->name[4], tmp));
			}
		}
	}

	/*
	 * Build bridged i/f name list and wan i/f name list from lan device name list
	 * and wan device name list. Both lan device list "landevs" and wan device list
	 * "wandevs" must exist in order to preceed.
	 */
	if ((landevs = nvram_get("landevs")) && (wandevs = nvram_get("wandevs"))) {
		/* build bridged i/f list based on nvram variable "landevs" */
		len = sizeof(lan_ifnames);
		if (!build_ifnames(landevs, lan_ifnames, &len) && len)
			dyna[1].value = lan_ifnames;
		else
			goto canned_config;
		/* build wan i/f list based on nvram variable "wandevs" */
		len = sizeof(wan_ifnames);
		if (!build_ifnames(wandevs, wan_ifnames, &len) && len) {
			dyna[3].value = wan_ifnames;
			foreach(wan_ifname, wan_ifnames, next) {
				dyna[2].value = wan_ifname;
				break;
			}
		}
		else
			ap = 1;

		if (auto_bridge)
		{
			printf("INFO: lan_ifnames=%s\n", lan_ifnames);
			printf("INFO: wan_ifnames=%s\n", wan_ifnames);
			sprintf(dyna_auto_ifnames, "%s %s", lan_ifnames, wan_ifnames);
			printf("INFO: dyna_auto_ifnames=%s\n", dyna_auto_ifnames);
			dyna_auto_bridge[1].value = dyna_auto_ifnames;
			linux_overrides = dyna_auto_bridge;
			printf("INFO: linux_overrides=dyna_auto_bridge \n");
		}
		else
		{
			linux_overrides = dyna;
			printf("INFO: linux_overrides=dyna \n");
		}

	}
	/* override lan i/f name list and wan i/f name list with default values */
	else {
canned_config:
#ifdef __CONFIG_VLAN__
		boardflags = strtoul(nvram_safe_get("boardflags"), NULL, 0);
		if (boardflags & BFL_ENETVLAN) {
			if (auto_bridge)
			{
				linux_overrides = vlan_auto_bridge;
				printf("INFO: linux_overrides=vlan_auto_bridge \n");
			}
			else
			{
				linux_overrides = vlan;
				printf("INFO: linux_overrides=vlan \n");
			}
		} else {
#endif	/* __CONFIG_VLAN__ */
			if (auto_bridge)
			{
				linux_overrides = generic_auto_bridge;
				printf("INFO: linux_overrides=generic_auto_bridge \n");
			}
			else
			{
				linux_overrides = generic;
				printf("INFO: linux_overrides=generic \n");
			}
#ifdef __CONFIG_VLAN__
		}
#endif	/* __CONFIG_VLAN__ */
	}

		for (u = linux_overrides; u && u->name; u++) {
			nvram_set(u->name, u->value);
			printf("INFO: action nvram_set %s, %s\n", u->name, u->value);
			}

	/* Force to AP */
	if (ap)
		nvram_set("router_disable", "1");

	if (auto_bridge) {
		printf("INFO: reset auto_bridge flag.\n");
		nvram_set("auto_bridge_action", "0");
	}

	nvram_commit();
	cprintf("auto_bridge done\n");
}

#endif	/* __CONFIG_NAT__ */


static void
upgrade_defaults(void)
{
	char temp[100];
	int i;
	bool bss_enabled = TRUE;
	char *val;

	/* Check whether upgrade is required or not
	 * If lan1_ifnames is not found in NVRAM , upgrade is required.
	 */
	if (!nvram_get("lan1_ifnames") && !RESTORE_DEFAULTS()) {
		cprintf("NVRAM upgrade required.  Starting.\n");

		if (nvram_match("ure_disable", "1")) {
			nvram_set("lan1_ifname", "br1");
			nvram_set("lan1_ifnames", "wl0.1 wl0.2 wl0.3 wl1.1 wl1.2 wl1.3");
		}
		else {
			nvram_set("lan1_ifname", "");
			nvram_set("lan1_ifnames", "");
			for (i = 0; i < 2; i++) {
				snprintf(temp, sizeof(temp), "wl%d_ure", i);
				if (nvram_match(temp, "1")) {
					snprintf(temp, sizeof(temp), "wl%d.1_bss_enabled", i);
					nvram_set(temp, "1");
				}
				else {
					bss_enabled = FALSE;
					snprintf(temp, sizeof(temp), "wl%d.1_bss_enabled", i);
					nvram_set(temp, "0");
				}
			}
		}
		if (nvram_get("lan1_ipaddr")) {
			nvram_set("lan1_gateway", nvram_get("lan1_ipaddr"));
		}

		for (i = 0; i < 2; i++) {
			snprintf(temp, sizeof(temp), "wl%d_bss_enabled", i);
			nvram_set(temp, "1");
			snprintf(temp, sizeof(temp), "wl%d.1_guest", i);
			if (nvram_match(temp, "1")) {
				nvram_unset(temp);
				if (bss_enabled) {
					snprintf(temp, sizeof(temp), "wl%d.1_bss_enabled", i);
					nvram_set(temp, "1");
				}
			}

			snprintf(temp, sizeof(temp), "wl%d.1_net_reauth", i);
			val = nvram_get(temp);
			if (!val || (*val == 0))
				nvram_set(temp, nvram_default_get(temp));

			snprintf(temp, sizeof(temp), "wl%d.1_wpa_gtk_rekey", i);
			val = nvram_get(temp);
			if (!val || (*val == 0))
				nvram_set(temp, nvram_default_get(temp));
		}

		nvram_commit();

		cprintf("NVRAM upgrade complete.\n");
	}
}

static void
restore_defaults(void)
{
#if 0 /*  wklin removed start, 10/22/2008 */
	struct nvram_tuple generic[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "eth0 eth2 eth3 eth4", 0 },
		{ "wan_ifname", "eth1", 0 },
		{ "wan_ifnames", "eth1", 0 },
		{ "lan1_ifname", "br1", 0 },
		{ "lan1_ifnames", "wl0.1 wl0.2 wl0.3 wl1.1 wl1.2 wl1.3", 0 },
		{ 0, 0, 0 }
	};
#ifdef __CONFIG_VLAN__
	struct nvram_tuple vlan[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "vlan0 eth1 eth2 eth3", 0 },
		{ "wan_ifname", "vlan1", 0 },
		{ "wan_ifnames", "vlan1", 0 },
		{ "lan1_ifname", "br1", 0 },
		{ "lan1_ifnames", "wl0.1 wl0.2 wl0.3 wl1.1 wl1.2 wl1.3", 0 },
		{ 0, 0, 0 }
	};
#endif	/* __CONFIG_VLAN__ */
	struct nvram_tuple dyna[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ "lan1_ifname", "br1", 0 },
		{ "lan1_ifnames", "wl0.1 wl0.2 wl0.3 wl1.1 wl1.2 wl1.3", 0 },
		{ 0, 0, 0 }
	};
#endif /* 0 */ /*  wklin removed end, 10/22/2008 */

	/* struct nvram_tuple *linux_overrides; *//*  wklin removed, 10/22/2008 */
	struct nvram_tuple *t, *u;
	int restore_defaults, i;
#ifdef __CONFIG_VLAN__
	uint boardflags;
#endif	/* __CONFIG_VLAN_ */ 
    /*   wklin removed start, 10/22/2008*/
    /*
	char *landevs, *wandevs;
	char lan_ifnames[128], wan_ifnames[128];
	char wan_ifname[32], *next;
	int len;
	int ap = 0;
    */
    /*  wklin removed end, 10/22/2008 */
#ifdef TRAFFIC_MGMT
	int j;
#endif  /* TRAFFIC_MGMT */

	/* Restore defaults if told to or OS has changed */
	restore_defaults = RESTORE_DEFAULTS();

	if (restore_defaults)
		cprintf("Restoring defaults...");

	/* Delete dynamically generated variables */
	if (restore_defaults) {
		char tmp[100], prefix[] = "wlXXXXXXXXXX_";
		for (i = 0; i < MAX_NVPARSE; i++) {
#ifdef __CONFIG_NAT__
			del_filter_client(i);
			del_forward_port(i);
#if !defined(AUTOFW_PORT_DEPRECATED)
			del_autofw_port(i);
#endif
#endif	/* __CONFIG_NAT__ */
			snprintf(prefix, sizeof(prefix), "wl%d_", i);
			for (t = router_defaults; t->name; t ++) {
				if (!strncmp(t->name, "wl_", 3))
					nvram_unset(strcat_r(prefix, &t->name[3], tmp));
			}
#ifdef __CONFIG_NAT__
			snprintf(prefix, sizeof(prefix), "wan%d_", i);
			for (t = router_defaults; t->name; t ++) {
				if (!strncmp(t->name, "wan_", 4))
					nvram_unset(strcat_r(prefix, &t->name[4], tmp));
			}
#endif	/* __CONFIG_NAT__ */
#ifdef TRAFFIC_MGMT
			snprintf(prefix, sizeof(prefix), "wl%d_", i);
			for (j = 0; j < MAX_NUM_TRF_MGMT_RULES; j++) {
				del_trf_mgmt_port(prefix, j);
			}
#endif  /* TRAFFIC_MGMT */
		}
#ifdef __CONFIG_WSCCMD__
		wps_restore_defaults();
#endif /* __CONFIG_WSCCMD__ */
#ifdef __CONFIG_WAPI_IAS__
		nvram_unset("as_mode");
#endif /* __CONFIG_WAPI_IAS__ */

		virtual_radio_restore_defaults();
	}

#if 0 /*  removed start, wklin, 10/22/2008, we don't need this */
	/* 
	 * Build bridged i/f name list and wan i/f name list from lan device name list
	 * and wan device name list. Both lan device list "landevs" and wan device list
	 * "wandevs" must exist in order to preceed.
	 */
	if ((landevs = nvram_get("landevs")) && (wandevs = nvram_get("wandevs"))) {
		/* build bridged i/f list based on nvram variable "landevs" */
		len = sizeof(lan_ifnames);
		if (!build_ifnames(landevs, lan_ifnames, &len) && len)
			dyna[1].value = lan_ifnames;
		else
			goto canned_config;
		/* build wan i/f list based on nvram variable "wandevs" */
		len = sizeof(wan_ifnames);
		if (!build_ifnames(wandevs, wan_ifnames, &len) && len) {
			dyna[3].value = wan_ifnames;
			foreach(wan_ifname, wan_ifnames, next) {
				dyna[2].value = wan_ifname;
				break;
			}
		}
		else
			ap = 1;
		linux_overrides = dyna;
	}
	/* override lan i/f name list and wan i/f name list with default values */
	else {
canned_config:
#ifdef __CONFIG_VLAN__
		boardflags = strtoul(nvram_safe_get("boardflags"), NULL, 0);
		if (boardflags & BFL_ENETVLAN)
			linux_overrides = vlan;
		else
#endif	/* __CONFIG_VLAN__ */
			linux_overrides = generic;
	}

	/* Check if nvram version is set, but old */
	if (nvram_get("nvram_version")) {
		int old_ver, new_ver;

		old_ver = atoi(nvram_get("nvram_version"));
		new_ver = atoi(NVRAM_SOFTWARE_VERSION);
		if (old_ver < new_ver) {
			cprintf("NVRAM: Updating from %d to %d\n", old_ver, new_ver);
			nvram_set("nvram_version", NVRAM_SOFTWARE_VERSION);
		}
	}
#endif /* 0 */ /*  removed end, wklin, 10/22/2008 */

	/* Restore defaults */
	for (t = router_defaults; t->name; t++) {
		if (restore_defaults || !nvram_get(t->name)) {
#if 0 /*  removed, wklin, 10/22/2008 , no overrides */
			for (u = linux_overrides; u && u->name; u++) {
				if (!strcmp(t->name, u->name)) {
					nvram_set(u->name, u->value);
					break;
				}
			}
			if (!u || !u->name)
#endif
			nvram_set(t->name, t->value);
			//if(nvram_get(t->name))
			//	cprintf("%s:%s\n", t->name, nvram_get(t->name));
		}
	}

#ifdef __CONFIG_WSCCMD__
	//rc_randomKey();
#endif
	/* Force to AP */
#if 0 /*  wklin removed, 10/22/2008 */
	if (ap)
		nvram_set("router_disable", "1");
#endif

	/* Always set OS defaults */
	nvram_set("os_name", "linux");
	nvram_set("os_version", ROUTER_VERSION_STR);
	nvram_set("os_date", __DATE__);
	/* Always set WL driver version! */
	nvram_set("wl_version", EPI_VERSION_STR);

	nvram_set("is_modified", "0");
	nvram_set("ezc_version", EZC_VERSION_STR);

	if (restore_defaults) {
	    
	    /*  removed start, Tony W.Y. Wang, 04/06/2010 */
#if 0
	    /*  add start by aspen Bai, 02/12/2009 */
		nvram_unset("pa2gw0a0");
		nvram_unset("pa2gw1a0");
		nvram_unset("pa2gw2a0");
		nvram_unset("pa2gw0a1");
		nvram_unset("pa2gw1a1");
		nvram_unset("pa2gw2a1");
#ifdef FW_VERSION_NA
		acosNvramConfig_setPAParam(0);
#else
		acosNvramConfig_setPAParam(1);
#endif
		/*  add end by aspen Bai, 02/12/2009 */
#endif
		/*  removed end, Tony W.Y. Wang, 04/06/2010 */
		
		/*  modified start, zacker, 08/06/2010 */
		/* Create a new value to inform loaddefault in "read_bd" */
		nvram_set("load_defaults", "1");
        eval("read_bd"); /*  wklin added, 10/22/2008 */
		/* finished "read_bd", unset load_defaults flag */
		nvram_unset("load_defaults");
		/*  modified end, zacker, 08/06/2010 */
        /*  add start, Tony W.Y. Wang, 04/06/2010 */
#ifdef SINGLE_FIRMWARE
        if (nvram_match("sku_name", "NA"))
            acosNvramConfig_setPAParam(0);
        else
            acosNvramConfig_setPAParam(1);
#else
		#ifdef FW_VERSION_NA
			acosNvramConfig_setPAParam(0);
		#else
			acosNvramConfig_setPAParam(1);
		#endif
#endif
        /*  add end, Tony W.Y. Wang, 04/06/2010 */
		nvram_commit();
		sync();         /*  added start pling 12/25/2006 */
		cprintf("done\n");
	}
}

#ifdef __CONFIG_NAT__
static void
set_wan0_vars(void)
{
	int unit;
	char tmp[100], prefix[] = "wanXXXXXXXXXX_";

	/* check if there are any connections configured */
	for (unit = 0; unit < MAX_NVPARSE; unit ++) {
		snprintf(prefix, sizeof(prefix), "wan%d_", unit);
		if (nvram_get(strcat_r(prefix, "unit", tmp)))
			break;
	}
	/* automatically configure wan0_ if no connections found */
	if (unit >= MAX_NVPARSE) {
		struct nvram_tuple *t;
		char *v;

		/* Write through to wan0_ variable set */
		snprintf(prefix, sizeof(prefix), "wan%d_", 0);
		for (t = router_defaults; t->name; t ++) {
			if (!strncmp(t->name, "wan_", 4)) {
				if (nvram_get(strcat_r(prefix, &t->name[4], tmp)))
					continue;
				v = nvram_get(t->name);
				nvram_set(tmp, v ? v : t->value);
			}
		}
		nvram_set(strcat_r(prefix, "unit", tmp), "0");
		nvram_set(strcat_r(prefix, "desc", tmp), "Default Connection");
		nvram_set(strcat_r(prefix, "primary", tmp), "1");
	}
}
#endif	/* __CONFIG_NAT__ */

/*  add start by Hank for ecosystem support 08/14/2012 */
#ifdef ECOSYSTEM_SUPPORT
int jffs2_mtd_mount(void)
{
	FILE *fp;
	char dev[PATH_MAX];
	int i, ret = -1;
	char *jpath = "/tmp/media/nand";
	struct stat tmp_stat;

	if (!(fp = fopen("/proc/mtd", "r")))
		return ret;

	while (fgets(dev, sizeof(dev), fp)) {
		if (sscanf(dev, "mtd%d:", &i) && strstr(dev, "brcmnand")) {
#ifdef LINUX26
			snprintf(dev, sizeof(dev), "/dev/mtdblock%d", i);
#else
			snprintf(dev, sizeof(dev), "/dev/mtdblock/%d", i);
#endif
			if (stat(jpath, &tmp_stat) != 0)
				mkdir(jpath, 0777);

			/*
			 * More time will be taken for the first mounting of JFFS2 on
			 * bare nand device.
			 */
			ret = mount(dev, jpath, "jffs2", 0, NULL);

			/*
			 * Erase nand flash MTD partition and mount again, in case of mount failure.
			 */
			if (ret) {
				fprintf(stderr, "Erase nflash MTD partition and mount again\n");
				if ((ret = mtd_erase("brcmnand"))) {
					fprintf(stderr, "Erase nflash MTD partition %s failed %d\n",
						dev, ret);
				} else {
					ret = mount(dev, jpath, "jffs2", 0, NULL);
				}
			}
			break;
		}
	}
	fclose(fp);

	/* mount successfully */
	if (ret != 0) {
		fprintf(stderr, "Mount nflash MTD jffs2 partition %s to %s failed\n", dev, jpath);
	}

	return ret;
}
#endif
/*  add end by Hank for ecosystem support 08/14/2012 */

static int noconsole = 0;

static void
sysinit(void)
{
	char buf[PATH_MAX];
	struct utsname name;
	struct stat tmp_stat;
	time_t tm = 0;
	char *loglevel;

	struct utsname unamebuf;
	char *lx_rel;

	/* Use uname() to get the system's hostname */
	uname(&unamebuf);
	lx_rel = unamebuf.release;

	if (memcmp(lx_rel, "2.6", 3) == 0) {
		int fd;
		if ((fd = open("/dev/console", O_RDWR)) < 0) {
			if (memcmp(lx_rel, "2.6.36", 6) == 0) {
				mount("devfs", "/dev", "devtmpfs", MS_MGC_VAL, NULL);
			} else {
				mount("devfs", "/dev", "tmpfs", MS_MGC_VAL, NULL);
				mknod("/dev/console", S_IRWXU|S_IFCHR, makedev(5, 1));
			}
		}
		else {
			close(fd);
		}
	}

	/* /proc */
	mount("proc", "/proc", "proc", MS_MGC_VAL, NULL);
#ifdef LINUX26
	mount("sysfs", "/sys", "sysfs", MS_MGC_VAL, NULL);
#endif /* LINUX26 */

	/* /tmp */
	mount("ramfs", "/tmp", "ramfs", MS_MGC_VAL, NULL);

	/* /var */
	mkdir("/tmp/var", 0777);
	mkdir("/var/lock", 0777);
	mkdir("/var/log", 0777);
	mkdir("/var/run", 0777);
	mkdir("/var/tmp", 0777);
	mkdir("/tmp/media", 0777);

#ifdef __CONFIG_UTELNETD__
	/* If kernel enable unix908 pty then we have to make following things. */
	mkdir("/dev/pts", 0777);
	if (mount("devpts", "/dev/pts", "devpts", MS_MGC_VAL, NULL) == 0) {
		/* pty master */
		mknod("/dev/ptmx", S_IRWXU|S_IFCHR, makedev(5, 2));
	} else {
		rmdir("/dev/pts");
	}
#endif	/* LINUX2636 && __CONFIG_UTELNETD__ */

#ifdef __CONFIG_SAMBA__
	/* Add Samba Stuff */
	mkdir("/tmp/samba", 0777);
	mkdir("/tmp/samba/lib", 0777);
	mkdir("/tmp/samba/private", 0777);
	mkdir("/tmp/samba/var", 0777);
	mkdir("/tmp/samba/var/locks", 0777);
#endif

#ifdef BCMQOS
	mkdir("/tmp/qos", 0777);
#endif
	/* Setup console */
	if (console_init())
		noconsole = 1;

#ifdef LINUX26
	mkdir("/dev/shm", 0777);
	eval("/sbin/hotplug2", "--coldplug");
#endif /* LINUX26 */

	if ((loglevel = nvram_get("console_loglevel")))
		klogctl(8, NULL, atoi(loglevel));
	else
		klogctl(8, NULL, 1);

	/* Modules */
	uname(&name);
	snprintf(buf, sizeof(buf), "/lib/modules/%s", name.release);
	if (stat("/proc/modules", &tmp_stat) == 0 &&
	    stat(buf, &tmp_stat) == 0) {
		char module[80], *modules, *next;

		/*  modified start, zacker, 08/06/2010 */
		/* Restore defaults if necessary */
		restore_defaults();


        /* For 4500 IR-159. by MJ. 2011.07.04  */
        /*  added start pling 02/11/2011 */
        /* WNDR4000 IR20: unset vifs NVRAM and let
         * bcm_wlan_util.c to reconstruct them if
         * necessary. move to here since they should be
         * done before read_bd */
        nvram_unset("wl0_vifs");
        nvram_unset("wl1_vifs");
        /*  added end pling 02/11/2011 */

		/* Read ethernet MAC, RF params, etc */
		eval("read_bd");
		/*  modified end, zacker, 08/06/2010 */

		/* Load ctf */
    /*  added start pling 08/19/2010 */
    /* Make sure the NVRAM "ctf_disable" exist, otherwise 
     * MultiSsidCntrl will not work.
     */
    if (nvram_get("ctf_disable") == NULL)
        nvram_set("ctf_disable", "1");
    /*  added end pling 08/19/2010 */
		if (!nvram_match("ctf_disable", "1"))
			eval("insmod", "ctf");
#if defined(__CONFIG_WAPI__) || defined(__CONFIG_WAPI_IAS__)
		wapi_mtd_restore();
#endif /* __CONFIG_WAPI__ || __CONFIG_WAPI_IAS__ */


/* #ifdef BCMVISTAROUTER */
#ifdef __CONFIG_IPV6__
		eval("insmod", "ipv6");
#endif /* __CONFIG_IPV6__ */
/* #endif */

#ifdef __CONFIG_EMF__
		/* Load the EMF & IGMP Snooper modules */
		load_emf();
#endif /*  __CONFIG_EMF__ */
#if defined(__CONFIG_HSPOT__) || defined(__CONFIG_NPS__)
		eval("insmod", "proxyarp");
#endif /*  __CONFIG_HSPOT__ || __CONFIG_NPS__ */

        /* add start by Hank for mount mtd of ecosystem support 08/14/2012*/
#ifdef ECOSYSTEM_SUPPORT
		system("mkdir /tmp/media/nand");
        jffs2_mtd_mount();
#endif
		/* add end by Hank for mount mtd of ecosystem support 08/14/2012*/
    /* Bob added start to avoid sending unexpected dad, 09/16/2009 */
#ifdef INCLUDE_IPV6
		if (nvram_match("ipv6ready","1"))
		{
			system("echo 0 > /proc/sys/net/ipv6/conf/default/dad_transmits");
		}else{
		/*  added start pling 12/06/2010 */
		/* By default ipv6_spi is inserted to system to drop all packets. */
		/* modify start by Hank for change ipv6_spi path in rootfs 08/27/2012*/
			system("/sbin/insmod /lib/modules/2.6.36.4brcmarm+/kernel/lib/ipv6_spi.ko");
		/* modify end by Hank for change ipv6_spi path in rootfs 08/27/2012*/
		/*  added end pling 12/06/2010 */
		}
#endif
    /* Bob added end to avoid sending unexpected dad, 09/16/2009 */
        
        
		/*  added start pling 09/02/2010 */
		/* Need to initialise switch related NVRAM before 
		 * insert ethernet module.
		 */
#ifdef __CONFIG_IGMP_SNOOPING__
		config_switch();
#endif
		/*  added end pling 09/02/2010 */

		//modules = nvram_get("kernel_mods") ? : "et bcm57xx wl";
		/* modify start by Hank for insert dpsta 08/27/2012*/
		modules = nvram_get("kernel_mods") ? : "proxyarp et dpsta wl"; /* foxconn wklin modified, 10/22/2008 */
		/* modify end by Hank for insert dpsta 08/27/2012*/

		foreach(module, modules, next){
            /*, [MJ] for GPIO debugging. */
#ifdef WIFI_DISABLE
            if(strcmp(module, "wl")){
			    eval("insmod", module);
            }else
                cprintf("we don't insert wl.ko.\n");
#else
            eval("insmod", module);
#endif
        }
#ifdef __CONFIG_USBAP__
		/* We have to load USB modules after loading PCI wl driver so
		 * USB driver can decide its instance number based on PCI wl
		 * instance numbers (in hotplug_usb())
		 */
		eval("insmod", "usbcore");

        /* , [MJ] start, we can't insert usb-storage easiler than
         * automount being started. */
#if 0

		eval("insmod", "usb-storage");
        /* , [MJ], for debugging. */
        cprintf("--> insmod usb-storage.\n");
#endif
        /* , [MJ] end, we can't insert usb-storage easiler than
         * automount being started. */
		{
			char	insmod_arg[128];
			int	i = 0, maxwl_eth = 0, maxunit = -1;
			char	ifname[16] = {0};
			int	unit = -1;
			char arg1[20] = {0};
			char arg2[20] = {0};
			char arg3[20] = {0};
			char arg4[20] = {0};
			char arg5[20] = {0};
			char arg6[20] = {0};
			char arg7[20] = {0};
			const int wl_wait = 3;	/* max wait time for wl_high to up */

			/* Save QTD cache params in nvram */
			sprintf(arg1, "log2_irq_thresh=%d", atoi(nvram_safe_get("ehciirqt")));
			sprintf(arg2, "qtdc_pid=%d", atoi(nvram_safe_get("qtdc_pid")));
			sprintf(arg3, "qtdc_vid=%d", atoi(nvram_safe_get("qtdc_vid")));
			sprintf(arg4, "qtdc0_ep=%d", atoi(nvram_safe_get("qtdc0_ep")));
			sprintf(arg5, "qtdc0_sz=%d", atoi(nvram_safe_get("qtdc0_sz")));
			sprintf(arg6, "qtdc1_ep=%d", atoi(nvram_safe_get("qtdc1_ep")));
			sprintf(arg7, "qtdc1_sz=%d", atoi(nvram_safe_get("qtdc1_sz")));

			eval("insmod", "ehci-hcd", arg1, arg2, arg3, arg4, arg5,
				arg6, arg7);

			/* Search for existing PCI wl devices and the max unit number used.
			 * Note that PCI driver has to be loaded before USB hotplug event.
			 * This is enforced in rc.c
			 */
			for (i = 1; i <= DEV_NUMIFS; i++) {
				sprintf(ifname, "eth%d", i);
				if (!wl_probe(ifname)) {
					if (!wl_ioctl(ifname, WLC_GET_INSTANCE, &unit,
						sizeof(unit))) {
						maxwl_eth = i;
						maxunit = (unit > maxunit) ? unit : maxunit;
					}
				}
			}

			/* Set instance base (starting unit number) for USB device */
			sprintf(insmod_arg, "instance_base=%d", maxunit + 1);
            /*, [MJ] for GPIO debugging. */
#ifndef WIFI_DISABLE
			eval("insmod", "wl_high", insmod_arg);
#endif
			/* Hold until the USB/HSIC interface is up (up to wl_wait sec) */
			sprintf(ifname, "eth%d", maxwl_eth + 1);
			i = wl_wait;
			while (wl_probe(ifname) && i--) {
				sleep(1);
			}
			if (!wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit)))
				cprintf("wl%d is up in %d sec\n", unit, wl_wait - i);
			else
				cprintf("wl%d not up in %d sec\n", unit, wl_wait);
		}
#ifdef LINUX26
		mount("usbdeffs", "/proc/bus/usb", "usbfs", MS_MGC_VAL, NULL);
#else
		mount("none", "/proc/bus/usb", "usbdevfs", MS_MGC_VAL, NULL);
#endif /* LINUX26 */
#endif /* __CONFIG_USBAP__ */

#ifdef __CONFIG_WCN__
		modules = "scsi_mod sd_mod usbcore usb-ohci usb-storage fat vfat msdos";
		foreach(module, modules, next){
            /* , [MJ] for debugging. */
            cprintf("--> insmod %s\n", ,module);
			eval("insmod", module);
#endif

#ifdef __CONFIG_SOUND__
		modules = "soundcore snd snd-timer snd-page-alloc snd-pcm snd-pcm-oss "
		        "snd-soc-core i2c-core i2c-algo-bit i2c-gpio snd-soc-bcm947xx-i2s "
		        "snd-soc-bcm947xx-pcm snd-soc-wm8750 snd-soc-wm8955 snd-soc-bcm947xx";
		foreach(module, modules, next)
			eval("insmod", module);
		mknod("/dev/dsp", S_IRWXU|S_IFCHR, makedev(14, 3));
		mkdir("/dev/snd", 0777);
		mknod("/dev/snd/controlC0", S_IRWXU|S_IFCHR, makedev(116, 0));
		mknod("/dev/snd/pcmC0D0c", S_IRWXU|S_IFCHR, makedev(116, 24));
		mknod("/dev/snd/pcmC0D0p", S_IRWXU|S_IFCHR, makedev(116, 16));
		mknod("/dev/snd/timer", S_IRWXU|S_IFCHR, makedev(116, 33));
#endif
	}
	/* add start by Hank for enable USB power 08/24/2012*/
	/* add end by Hank for enable USB power 08/24/2012*/
	system("/usr/sbin/et robowr 0x0 0x10 0x0022");
	if (memcmp(lx_rel, "2.6.36", 6) == 0) {
		int fd;
		if ((fd = open("/proc/irq/163/smp_affinity", O_RDWR)) >= 0) {
			close(fd);
			system("echo 2 > /proc/irq/163/smp_affinity");
			system("echo 2 > /proc/irq/169/smp_affinity");
		}
	}
	/* Set a sane date */
	stime(&tm);

	dprintf("done\n");
}

/* States */
enum {
	RESTART,
	STOP,
	START,
	TIMER,
	IDLE,
	WSC_RESTART,
	WLANRESTART, /*  added by EricHuang, 11/24/2006 */
	PPPSTART    /*  added by EricHuang, 01/09/2008 */
};
static int state = START;
static int signalled = -1;

/*  added start, zacker, 05/20/2010, @spec_1.9 */
static int next_state = IDLE;

static int
next_signal(void)
{
	int tmp_sig = next_state;
	next_state = IDLE;
	return tmp_sig;
}
/*  added end, zacker, 05/20/2010, @spec_1.9 */

/* Signal handling */
static void
rc_signal(int sig)
{
	if (state == IDLE) {	
		if (sig == SIGHUP) {
			dprintf("signalling RESTART\n");
			signalled = RESTART;
		}
		else if (sig == SIGUSR2) {
			dprintf("signalling START\n");
			signalled = START;
		}
		else if (sig == SIGINT) {
			dprintf("signalling STOP\n");
			signalled = STOP;
		}
		else if (sig == SIGALRM) {
			dprintf("signalling TIMER\n");
			signalled = TIMER;
		}
		else if (sig == SIGUSR1) {
			dprintf("signalling WSC RESTART\n");
			signalled = WSC_RESTART;
		}
		/*  modified start by EricHuang, 01/09/2008 */
		else if (sig == SIGQUIT) {
		    dprintf("signalling WLANRESTART\n");
		    signalled = WLANRESTART;
		}
		else if (sig == SIGILL) {
		    signalled = PPPSTART;
		}
		/*  modified end by EricHuang, 01/09/2008 */
	}
	/*  added start, zacker, 05/20/2010, @spec_1.9 */
	else if (next_state == IDLE)
	{
		if (sig == SIGHUP) {
			dprintf("signalling RESTART\n");
			next_state = RESTART;
		}
		else if (sig == SIGUSR2) {
			dprintf("signalling START\n");
			next_state = START;
		}
		else if (sig == SIGINT) {
			dprintf("signalling STOP\n");
			next_state = STOP;
		}
		else if (sig == SIGALRM) {
			dprintf("signalling TIMER\n");
			next_state = TIMER;
		}
		else if (sig == SIGUSR1) {
			dprintf("signalling WSC RESTART\n");
			next_state = WSC_RESTART;
		}
		else if (sig == SIGQUIT) {
			printf("signalling WLANRESTART\n");
			next_state = WLANRESTART;
		}
		else if (sig == SIGILL) {
			next_state = PPPSTART;
		}
	}
	/*  added end, zacker, 05/20/2010, @spec_1.9 */
}

/* Get the timezone from NVRAM and set the timezone in the kernel
 * and export the TZ variable
 */
static void
set_timezone(void)
{
	time_t now;
	struct tm gm, local;
	struct timezone tz;
	struct timeval *tvp = NULL;

	/* Export TZ variable for the time libraries to
	 * use.
	 */
	setenv("TZ", nvram_get("time_zone"), 1);

	/* Update kernel timezone */
	time(&now);
	gmtime_r(&now, &gm);
	localtime_r(&now, &local);
	tz.tz_minuteswest = (mktime(&gm) - mktime(&local)) / 60;
	settimeofday(tvp, &tz);

#if defined(__CONFIG_WAPI__) || defined(__CONFIG_WAPI_IAS__)
#ifndef	RC_BUILDTIME
#define	RC_BUILDTIME	1252636574
#endif
	{
		struct timeval tv = {RC_BUILDTIME, 0};

		time(&now);
		if (now < RC_BUILDTIME)
			settimeofday(&tv, &tz);
	}
#endif /* __CONFIG_WAPI__ || __CONFIG_WAPI_IAS__ */
}

/* Timer procedure.Gets time from the NTP servers once every timer interval
 * Interval specified by the NVRAM variable timer_interval
 */
int
do_timer(void)
{
	int interval = atoi(nvram_safe_get("timer_interval"));

	dprintf("%d\n", interval);

	if (interval == 0)
		return 0;

	/* Report stats */
	if (nvram_invmatch("stats_server", "")) {
		char *stats_argv[] = { "stats", nvram_get("stats_server"), NULL };
		_eval(stats_argv, NULL, 5, NULL);
	}

	/* Sync time */
	start_ntpc();

	alarm(interval);

	return 0;
}

/* Main loop */
static void
main_loop(void)
{
#ifdef CAPI_AP
	static bool start_aput = TRUE;
#endif
	sigset_t sigset;
	pid_t shell_pid = 0;
#ifdef __CONFIG_VLAN__
	uint boardflags;
#endif
	
    /*  wklin added start, 10/22/2008 */
	sysinit();

	/* Add loopback */
	config_loopback();
	/* Restore defaults if necessary */
	//restore_defaults(); /*  removed, zacker, 08/06/2010, move to sysinit() */

	/* Convert deprecated variables */
	convert_deprecated();

	/* Upgrade NVRAM variables to MBSS mode */
	upgrade_defaults();

    /* Read ethernet MAC, etc */
    //eval("read_bd"); /*  removed, zacker, 08/06/2010, move to sysinit() */
    /*  wklin added end, 10/22/2008 */

    /* Reset some wps-related parameters */
    nvram_set("wps_start",   "none");
    /*  added start, zacker, 05/20/2010, @spec_1.9 */
    nvram_set("wps_status", "0"); /* start_wps() */
    nvram_set("wps_proc_status", "0");
    /*  added end, zacker, 05/20/2010, @spec_1.9 */
    
    /*  Perry added start, 2011/05/13, for IPv6 router advertisment prefix information */
    /* reset IPv6 obsolete prefix information after reboot */
    nvram_set("radvd_lan_obsolete_ipaddr", "");
    nvram_set("radvd_lan_obsolete_ipaddr_length", "");
    nvram_set("radvd_lan_new_ipaddr", "");
    nvram_set("radvd_lan_new_ipaddr_length", "");
    /*  Perry added end, 2011/05/13, for IPv6 router advertisment prefix information */
    
    /*  added start, zacker, 06/17/2010, @new_tmp_lock */
    /* do this in case "wps_aplockdown_forceon" is set to "1" for tmp_lock
     * purpose but then there are "nvram_commit" and "reboot" action
     */
    if (nvram_match("wsc_pin_disable", "1"))
        nvram_set("wps_aplockdown_forceon", "1");
    else
        nvram_set("wps_aplockdown_forceon", "0");
    /*  added end, zacker, 06/17/2010, @new_tmp_lock */

    /*  added start, Wins, 04/20/2011, @RU_IPTV */
#ifdef CONFIG_RUSSIA_IPTV
    if (!is_russia_specific_support())
    {
        nvram_set(NVRAM_IPTV_ENABLED, "0");
        nvram_set(NVRAM_IPTV_INTF, "0x00");
    }
#endif /* CONFIG_RUSSIA_IPTV */
    /*  added end, Wins, 04/20/2011, @RU_IPTV */

    /*  add start, Max Ding, 02/26/2010 */
#ifdef RESTART_ALL_PROCESSES
    nvram_unset("restart_all_processes");
#endif
    /*  add end, Max Ding, 02/26/2010 */


	/* Basic initialization */
	//sysinit();

	/* Setup signal handlers */
	signal_init();
	signal(SIGHUP, rc_signal);
	signal(SIGUSR2, rc_signal);
	signal(SIGINT, rc_signal);
	signal(SIGALRM, rc_signal);
	signal(SIGUSR1, rc_signal);	
	signal(SIGQUIT, rc_signal); /*  added by EricHuang, 11/24/2006 */
	signal(SIGILL, rc_signal); //ppp restart
	sigemptyset(&sigset);

	/* Give user a chance to run a shell before bringing up the rest of the system */
	if (!noconsole)
		run_shell(1, 0);

	/* Get boardflags to see if VLAN is supported */
#ifdef __CONFIG_VLAN__
	boardflags = strtoul(nvram_safe_get("boardflags"), NULL, 0);
#endif	/* __CONFIG_VLAN__ */


#if 0 /*  modified, wklin 10/22/2008, move the the start of this function */
	/* Add loopback */
	config_loopback();

	/* Convert deprecated variables */
	convert_deprecated();



	/* Upgrade NVRAM variables to MBSS mode */
	upgrade_defaults();

	/* Restore defaults if necessary */
	restore_defaults();

    /*  added start pling 06/20/2007 */
    /* Read board data again, since the "restore_defaults" action
     * above will overwrite some of our settings */
    eval("read_bd");
    /*  added end pling 06/20/2006 */
#endif /* 0 */
    
#ifdef __CONFIG_NAT__
	/* Auto Bridge if neccessary */
	if (!strcmp(nvram_safe_get("auto_bridge"), "1"))
	{
		auto_bridge();
	}
	/* Setup wan0 variables if necessary */
	set_wan0_vars();
#endif	/* __CONFIG_NAT__ */

    /*  added start pling 07/13/2009 */
    /* create the USB semaphores */
#ifdef SAMBA_ENABLE
    usb_sem_init(); //[MJ] for 5G crash
#endif
    /*  added end pling 07/13/2009 */

#if defined(__CONFIG_FAILSAFE_UPGRADE_SUPPORT__)
	nvram_set(PARTIALBOOTS, "0");
	nvram_commit();
#endif

	/* Loop forever */
	for (;;) {
		switch (state) {
		case RESTART:
			dprintf("RESTART\n");
			/* Fall through */
			/*  added start pling 06/14/2007 */
            /* When vista finished configuring this router (wl0_wps_config_state: 0->1),
             * then we come here to restart WLAN 
             */
            stop_wps();
			stop_nas();
            stop_eapd();
			stop_bcmupnp();
			stop_wlan();
			/* add start by Hank 06/14/2012*/
			/*Enable 2.4G auto channel detect, kill acsd for stop change channel*/
			if((nvram_match("wla_channel", "0") || nvram_match("wlg_channel", "0")) && nvram_match("enable_sta_mode","0"))
				stop_acsd();
			/* add end by Hank 06/14/2012*/

    	    convert_wlan_params();  /* For WCN , added by EricHuang, 12/21/2006 */
            sleep(2);               /* Wait some time for wsc, etc to terminate */

            /* if "unconfig" to "config" mode, force it to built-in registrar and proxy mode */
            /* added start by EricHuang, 11/04/2008 */
            if ( nvram_match("wps_status", "0") ) //restart wlan for wsc
            {
                nvram_set("lan_wps_reg", "enabled");
                nvram_set("wl_wps_reg", "enabled");
                nvram_set("wl0_wps_reg", "enabled");
#if (defined INCLUDE_DUAL_BAND)
                nvram_set("wl0_wps_reg", "enabled");
#endif
                /*  modify start, Max Ding, 08/28/2010 for NEW_BCM_WPS */
                /* New NVRAM to BSP 5.22.83.0, 'wlx_wps_config_state' not used anymore. */
                //printf("restart -- wl0_wps_config_state=%s\n", nvram_get("wl0_wps_config_state"));
                //nvram_set("wl_wps_config_state", nvram_get("wl0_wps_config_state"));
                if ( nvram_match("lan_wps_oob", "enabled") )
                {
                    nvram_set("wl_wps_config_state", "0");
                    nvram_set("wl0_wps_config_state", "0");
#if (defined INCLUDE_DUAL_BAND)
                    nvram_set("wl1_wps_config_state", "0");
#endif
                }
                else
                {
                    nvram_set("wl_wps_config_state", "1");
                    nvram_set("wl0_wps_config_state", "1");
#if (defined INCLUDE_DUAL_BAND)
                    nvram_set("wl1_wps_config_state", "1");
#endif
                }
                /*  modify end, Max Ding, 08/28/2010 */
            }
            /* added end by EricHuang, 11/04/2008 */
            
            /* hide unnecessary warnings (Invaid XXX, out of range xxx etc...)*/
            {
                #include <fcntl.h>
                int fd1, fd2;
                fd1 = dup(2);
                fd2 = open("/dev/null", O_WRONLY);
                close(2);
                dup2(fd2, 2);
                close(fd2);
                start_wlan(); //<-- to hide messages generated here
                close(2);
                dup2(fd1, 2);
                close(fd1);
            }
            
            save_wlan_time();          
            start_bcmupnp();
            start_eapd();           /*  modify by aspen Bai, 10/08/2008 */
            start_nas();            /*  modify by aspen Bai, 08/01/2008 */
            start_wps();            /*  modify by aspen Bai, 08/01/2008 */
            sleep(2);               /* Wait for WSC to start */
            /*  add start by aspen Bai, 09/10/2008 */
            /* Must call it when start wireless */
            start_wl();
            /*  add end by aspen Bai, 09/10/2008 */
			/* add start by Hank 06/14/2012*/
			/*Enable 2.4G auto channel detect, call acsd to start change channel*/
			if((nvram_match("wla_channel", "0") || nvram_match("wlg_channel", "0")) && nvram_match("enable_sta_mode","0"))
				start_acsd();
			/* add end by Hank 06/14/2012*/
            nvram_commit();         /* Save WCN obtained parameters */

			/*  modified start, zacker, 05/20/2010, @spec_1.9 */
			//state = IDLE;
			state = next_signal();
			/*  modified end, zacker, 05/20/2010, @spec_1.9 */

#if 0       /*  removed start, zacker, 09/17/2009, @wps_led */
#ifdef BCM4716
			if (nvram_match("wla_secu_type", "None"))
			{
				system("/sbin/gpio 7 0");
			}
			else
			{
				system("/sbin/gpio 7 1");
			}
#else
			if (nvram_match("wla_secu_type", "None"))
			{
				system("/sbin/gpio 1 0");
			}
			else
			{
				system("/sbin/gpio 1 1");
			}
#endif
#endif      /*  removed end, zacker, 09/17/2009, @wps_led */
			
			break;
			/*  added end pling 06/14/2007 */

		case STOP:
			dprintf("STOP\n");
			pmon_init();
            stop_wps();
            stop_nas();
            stop_eapd(); 
            stop_bcmupnp();
			
			stop_lan();
#ifdef __CONFIG_VLAN__
			if (boardflags & BFL_ENETVLAN)
				stop_vlan();
#endif	/* __CONFIG_VLAN__ */
			if (state == STOP) {
				/*  modified start, zacker, 05/20/2010, @spec_1.9 */
				//state = IDLE;
				state = next_signal();
				/*  modified end, zacker, 05/20/2010, @spec_1.9 */
				break;
			}
			/* Fall through */
		case START:
			dprintf("START\n");
			pmon_init();
			/*  added start, zacker, 01/13/2012, @iptv_igmp */
#ifdef CONFIG_RUSSIA_IPTV
			if (!nvram_match("wla_repeater", "1")
#if (defined INCLUDE_DUAL_BAND)
				&& !nvram_match("wlg_repeater", "1")
#endif
				)
			{
				/* don't do this in cgi since "rc stop" need to do cleanup */
				config_iptv_params();
				/* always do this to active new vlan settings */
				active_vlan();
			}
#endif /* CONFIG_RUSSIA_IPTV */

#if (defined INCLUDE_QOS) || (defined __CONFIG_IGMP_SNOOPING__)
			config_switch_reg();
#endif
			/*  added end, zacker, 01/13/2012, @iptv_igmp */
		/* added start, water, 12/21/09*/
#ifdef RESTART_ALL_PROCESSES
		if ( nvram_match("restart_all_processes", "1") )
		{
			restore_defaults();
			eval("read_bd");
			convert_deprecated();
			/*  add start, Max Ding, 03/03/2010 */

#if (defined BCM5325E) || (defined BCM53125)
			system("/usr/sbin/et robowr 0x34 0x00 0x00e0");
#endif
			/*  add end, Max Ding, 03/03/2010 */
#if !defined(U12H245)			
			if(acosNvramConfig_match("emf_enable", "1") )
			{
    			system("insmod emf");
    			system("insmod igs");
    			system("insmod wl");
			}
#endif			
		}
#endif

		/*foxconn added end, water, 12/21/09*/
			{ /* Set log level on restart */
				char *loglevel;
				int loglev = 8;

				if ((loglevel = nvram_get("console_loglevel"))) {
					loglev = atoi(loglevel);
				}
				klogctl(8, NULL, loglev);
				if (loglev < 7) {
					printf("WARNING: console log level set to %d\n", loglev);
				}
			}

			set_timezone();
#ifdef __CONFIG_VLAN__
			if (boardflags & BFL_ENETVLAN)
				start_vlan();
#endif	/* __CONFIG_VLAN__ */
            /* wklin modified start, 10/23/2008 */
            /* hide unnecessary warnings (Invaid XXX, out of range xxx etc...)*/
            {
                #include <fcntl.h>
                int fd1, fd2;
                fd1 = dup(2);
                fd2 = open("/dev/null", O_WRONLY);
                close(2);
                dup2(fd2, 2);
                close(fd2);
                start_lan(); //<-- to hide messages generated here
                start_wlan(); //<-- need it to bring up 5G interface
                close(2);
                dup2(fd1, 2);
                close(fd1);
            }
            if (nvram_match("wla_repeater", "1")
#if (defined INCLUDE_DUAL_BAND)
            || nvram_match("wlg_repeater", "1")
#endif
            )
            {
                /* if repeater mode, del vlan1 from br0 and disable vlan */
#ifdef BCM4716
                system("/usr/sbin/brctl delif br0 vlan0");
                system("/usr/sbin/et robowr 0x34 0x00 0x00");
#else
                /* modified start, water, 01/07/10, @lan pc ping DUT failed when repeater mode & igmp enabled*/
                //system("/usr/sbin/brctl delif br0 vlan1");
                //system("/usr/sbin/et robowr 0x34 0x00 0x00");
#ifdef IGMP_PROXY
                if (!nvram_match("igmp_proxying_enable", "1"))
#endif
                {
                system("/usr/sbin/brctl delif br0 vlan1");
                system("/usr/sbin/et robowr 0x34 0x00 0x00");
                }
                /* modified end, water, 01/07/10*/
#endif
            }
            /* wklin modified end, 10/23/2008 */           
            save_wlan_time();
			start_bcmupnp();
            start_eapd();
            start_nas();
            start_wps();
            sleep(2);
            start_wl();
			/* add start by Hank 06/14/2012*/
			/*Enable 2.4G auto channel detect, call acsd to start change channel*/
			if((nvram_match("wla_channel", "0") || nvram_match("wlg_channel", "0")) && nvram_match("enable_sta_mode","0"))
				start_acsd();
			/* add end by Hank 06/14/2012*/
            /* Now start ACOS services */
            eval("acos_init");
            eval("acos_service", "start");

            /* Start wsc if it is in 'unconfiged' state, and if PIN is not disabled */
            if (nvram_match("wl0_wps_config_state", "0") && !nvram_match("wsc_pin_disable", "1"))
            {
                /* if "unconfig" to "config" mode, force it to built-in registrar and proxy mode */
                nvram_set("wl_wps_reg", "enabled");
                nvram_set("wl0_wps_reg", "enabled");
                nvram_set("wps_proc_status", "0");
                nvram_set("wps_method", "1");
                //nvram_set("wps_config_command", "1");
            }

            /*  added start pling 03/30/2009 */
            /* Fix antenna diversiy per Netgear Bing's request */
#if 0//(!defined WNR3500v2VCNA)        // pling added 04/10/2009, vnca don't want fixed antenna
            eval("wl", "down");
            eval("wl", "nphy_antsel", "0x02", "0x02", "0x02", "0x02");
            eval("wl", "up");
#endif
            /*  added end pling 03/30/2009 */
            //eval("wl", "interference", "2");    // pling added 03/27/2009, per Netgear Fanny request

#if ( (defined SAMBA_ENABLE) || (defined HSDPA) )
                if (!acosNvramConfig_match("wla_wlanstate", "Enable"))
                {/*water, 05/15/2009, @disable wireless, router will reboot continually*/
                 /*on WNR3500L, WNR3500U, MBR3500, it was just a work around..*/
                    eval("wl", "down");
                }
#endif

			/* Fall through */
		case TIMER:
            /*  removed start pling 07/12/2006 */
#if 0
			dprintf("TIMER\n");
			do_timer();
#endif
            /*  removed end pling 07/12/2006 */
			/* Fall through */
		case IDLE:
			dprintf("IDLE\n");
			/*  modified start, zacker, 05/20/2010, @spec_1.9 */
			//state = IDLE;
			state = next_signal();
			if (state != IDLE)
				break;
			/*  modified end, zacker, 05/20/2010, @spec_1.9 */

#ifdef CAPI_AP
			if (start_aput == TRUE) {
				system("/usr/sbin/wfa_aput_all&");
				start_aput = FALSE;
			}
#endif /* CAPI_AP */

			/*  added start, zacker, 09/17/2009, @wps_led */
			if (nvram_match("wps_start",   "none"))
			    /*  add modified, Tony W.Y. Wang, 12/03/2009 */
				//send_wps_led_cmd(WPS_LED_BLINK_OFF, 0);
				if (acosNvramConfig_match("dome_led_status", "ON"))
                    send_wps_led_cmd(WPS_LED_BLINK_OFF, 3);
                else if (acosNvramConfig_match("dome_led_status", "OFF"))
                    send_wps_led_cmd(WPS_LED_BLINK_OFF, 2);
			/*  added end, zacker, 09/17/2009, @wps_led */

			/* Wait for user input or state change */
			while (signalled == -1) {
				if (!noconsole && (!shell_pid || kill(shell_pid, 0) != 0))
					shell_pid = run_shell(0, 1);
				else {

					sigsuspend(&sigset);
				}
#ifdef LINUX26
				//system("echo 1 > /proc/sys/vm/drop_caches");
				system("echo 8192 > /proc/sys/vm/min_free_kbytes");
#elif defined(__CONFIG_SHRINK_MEMORY__)
				eval("cat", "/proc/shrinkmem");
#endif	/* LINUX26 */
			}
			state = signalled;
			signalled = -1;
			break;

		case WSC_RESTART:
			dprintf("WSC_RESTART\n");
			/*  modified start, zacker, 05/20/2010, @spec_1.9 */
			//state = IDLE;
			state = next_signal();
			/*  modified end, zacker, 05/20/2010, @spec_1.9 */
			stop_wps();    /*  modify by aspen Bai, 08/01/2008 */
			start_wps();    /*  modify by aspen Bai, 08/01/2008 */
			break;

            /*  added start pling 06/14/2007 */
            /* We come here only if user press "apply" in Wireless GUI */
		case WLANRESTART:
		
		    stop_wps(); 
		    stop_nas();
            stop_eapd();
            stop_bcmupnp();
            
			stop_wlan();
            
			/* add start by Hank 06/14/2012*/
			/*Enable 2.4G auto channel detect, kill acsd stop change channel*/
			if((nvram_match("wla_channel", "0") || nvram_match("wlg_channel", "0")) && nvram_match("enable_sta_mode","0"))
	            stop_acsd();
			/* add end by Hank 06/14/2012*/
			eval("read_bd");    /* sync  and brcm nvram params */
                       
            /* wklin modified start, 01/29/2007 */
            /* hide unnecessary warnings (Invaid XXX, out of range xxx etc...)*/
            {
                #include <fcntl.h>
                int fd1, fd2;
                fd1 = dup(2);
                fd2 = open("/dev/null", O_WRONLY);
                close(2);
                dup2(fd2, 2);
                close(fd2);
                start_wlan(); //<-- to hide messages generated here
                close(2);
                dup2(fd1, 2);
                close(fd1);
            }
            /* wklin modified end, 01/29/2007 */
            #if 0
            /*  add start, Tony W.Y. Wang, 03/25/2010 @Single Firmware Implementation */
            if (nvram_match("sku_name", "NA"))
            {
                printf("set wl country and power of NA\n");
                eval("wl", "country", "Q1/15");
                /*  modify start, Max Ding, 12/27/2010 "US/39->US/8" for open DFS band 2&3 channels */
                //eval("wl", "-i", "eth2", "country", "US/39");
                eval("wl", "-i", "eth2", "country", "Q1/15");
                /*  modify end, Max Ding, 12/27/2010 */
                /*  remove start, Max Ding, 12/27/2010 fix time zone bug for NA sku */
                //nvram_set("time_zone", "-8");
                /*  remove end, Max Ding, 12/27/2010 */
                nvram_set("wla_region", "11");
                nvram_set("wla_temp_region", "11");
                nvram_set("wl_country", "Q1");
                nvram_set("wl_country_code", "Q1");
                nvram_set("ver_type", "NA");
            }
            /*
            else if (nvram_match("sku_name", "WW"))
            {
                printf("set wl country and power of WW\n");
                eval("wl", "country", "EU/5");
                eval("wl", "-i", "eth2", "country", "EU/5");
                nvram_set("time_zone", "0");
                nvram_set("wla_region", "5");
                nvram_set("wla_temp_region", "5");
                nvram_set("wl_country", "EU5");
                nvram_set("wl_country_code", "EU5");
                nvram_set("ver_type", "WW");
            }
            */
            /*  add end, Tony W.Y. Wang, 03/25/2010 @Single Firmware Implementation */
            #endif
            
            save_wlan_time();
            start_bcmupnp();
            start_eapd();
            start_nas();
            start_wps();
            sleep(2);           /* Wait for WSC to start */
            start_wl();
			/* add start by Hank 06/14/2012*/
			/*Enable 2.4G auto channel detect, call acsd to start change channel*/
			if((nvram_match("wla_channel", "0") || nvram_match("wlg_channel", "0")) && nvram_match("enable_sta_mode","0"))
				start_acsd();
			/* add end by Hank 06/14/2012*/

            /* Start wsc if it is in 'unconfiged' state */
            if (nvram_match("wl0_wps_config_state", "0") && !nvram_match("wsc_pin_disable", "1"))
            {
                /* if "unconfig" to "config" mode, force it to built-in registrar and proxy mode */
                nvram_set("wl_wps_reg", "enabled");
                nvram_set("wl0_wps_reg", "enabled");
                nvram_set("wps_proc_status", "0");
                nvram_set("wps_method", "1");
                //nvram_set("wps_config_command", "1");
            }
			/*  modified start, zacker, 05/20/2010, @spec_1.9 */
			//state = IDLE;
			state = next_signal();
			/*  modified end, zacker, 05/20/2010, @spec_1.9 */
		    break;
            /*  added end pling 06/14/2007 */
        /*  added start by EricHuang, 01/09/2008 */
		case PPPSTART:
		{
            //char *pptp_argv[] = { "pppd", NULL };
            char *pptp_argv[] = { "pppd", "file", "/tmp/ppp/options", NULL };

		    _eval(pptp_argv, NULL, 0, NULL);
		    
		    /*  modified start, zacker, 05/20/2010, @spec_1.9 */
		    //state = IDLE;
		    state = next_signal();
		    /*  modified end, zacker, 05/20/2010, @spec_1.9 */
		    break;
		}
		/*  added end by EricHuang, 01/09/2008 */
		    
		default:
			dprintf("UNKNOWN\n");
			return;
		}
	}
}

int
main(int argc, char **argv)
{
#ifdef LINUX26
	char *init_alias = "preinit";
#else
	char *init_alias = "init";
#endif
	char *base = strrchr(argv[0], '/');

	base = base ? base + 1 : argv[0];

	/* init */
#ifdef LINUX26
	if (strstr(base, "preinit")) {
		mount("devfs", "/dev", "tmpfs", MS_MGC_VAL, NULL);
		/* Michael added */
//        mknod("/dev/nvram", S_IRWXU|S_IFCHR, makedev(252, 0));
/*        mknod("/dev/mtdblock16", S_IRWXU|S_IFBLK, makedev(31, 16));
        mknod("/dev/mtdblock17", S_IRWXU|S_IFBLK, makedev(31, 17));
        mknod("/dev/mtd16", S_IRWXU|S_IFCHR, makedev(90, 32));
        mknod("/dev/mtd16ro", S_IRWXU|S_IFCHR, makedev(90, 33));
        mknod("/dev/mtd17", S_IRWXU|S_IFCHR, makedev(90, 34));
        mknod("/dev/mtd17ro", S_IRWXU|S_IFCHR, makedev(90, 35));*/
		/* Michael ended */
		mknod("/dev/console", S_IRWXU|S_IFCHR, makedev(5, 1));
		mknod("/dev/aglog", S_IRWXU|S_IFCHR, makedev(AGLOG_MAJOR_NUM, 0));
		mknod("/dev/wps_led", S_IRWXU|S_IFCHR, makedev(WPS_LED_MAJOR_NUM, 0));
#ifdef __CONFIG_UTELNETD__
		mkdir("/dev/pts", 0777);	
		mknod("/dev/pts/ptmx", S_IRWXU|S_IFCHR, makedev(5, 2));
		mknod("/dev/pts/0", S_IRWXU|S_IFCHR, makedev(136, 0));
		mknod("/dev/pts/1", S_IRWXU|S_IFCHR, makedev(136, 1));
#endif	/* __CONFIG_UTELNETD__ */
		/*  added start pling 12/26/2011, for WNDR4000AC */
#if (defined GPIO_EXT_CTRL)
		mknod("/dev/ext_led", S_IRWXU|S_IFCHR, makedev(EXT_LED_MAJOR_NUM, 0));
#endif
		/*  added end pling 12/26/2011 */
#else /* LINUX26 */
	if (strstr(base, "init")) {
#endif /* LINUX26 */
		main_loop();
		return 0;
	}

	/* Set TZ for all rc programs */
	setenv("TZ", nvram_safe_get("time_zone"), 1);

	/* rc [stop|start|restart ] */
	if (strstr(base, "rc")) {
		if (argv[1]) {
			if (strncmp(argv[1], "start", 5) == 0)
				return kill(1, SIGUSR2);
			else if (strncmp(argv[1], "stop", 4) == 0)
				return kill(1, SIGINT);
			else if (strncmp(argv[1], "restart", 7) == 0)
				return kill(1, SIGHUP);
		    /*  added start by EricHuang, 11/24/2006 */
		    else if (strcmp(argv[1], "wlanrestart") == 0)
		        return kill(1, SIGQUIT);
		    /*  added end by EricHuang, 11/24/2006 */
		} else {
			fprintf(stderr, "usage: rc [start|stop|restart|wlanrestart]\n");
			return EINVAL;
		}
	}

#ifdef __CONFIG_NAT__
	/* ppp */
	else if (strstr(base, "ip-up"))
		return ipup_main(argc, argv);
	else if (strstr(base, "ip-down"))
		return ipdown_main(argc, argv);

	/* udhcpc [ deconfig bound renew ] */
	else if (strstr(base, "udhcpc"))
		return udhcpc_wan(argc, argv);
#endif	/* __CONFIG_NAT__ */

#if 0 /*  wklin removed, 05/14/2009 */
	/* ldhclnt [ deconfig bound renew ] */
	else if (strstr(base, "ldhclnt"))
		return udhcpc_lan(argc, argv);

	/* stats [ url ] */
	else if (strstr(base, "stats"))
		return http_stats(argv[1] ? : nvram_safe_get("stats_server"));
#endif

	/* erase [device] */
	else if (strstr(base, "erase")) {
		/*  modified, zacker, 07/09/2010 */
		/*
		if (argv[1] && ((!strcmp(argv[1], "boot")) ||
			(!strcmp(argv[1], "linux")) ||
			(!strcmp(argv[1], "rootfs")) ||
			(!strcmp(argv[1], "nvram")))) {
		*/
		if (argv[1]) {
			return mtd_erase(argv[1]);
		} else {
			fprintf(stderr, "usage: erase [device]\n");
			return EINVAL;
		}
	}


	/* write [path] [device] */
	else if (strstr(base, "write")) {
		if (argc >= 3)
			return mtd_write(argv[1], argv[2]);
		else {
			fprintf(stderr, "usage: write [path] [device]\n");
			return EINVAL;
		}
	}

	/* hotplug [event] */
	else if (strstr(base, "hotplug")) {
		if (argc >= 2) {
			if (!strcmp(argv[1], "net"))
				return hotplug_net();
		/* modified start, water, @usb porting, 11/11/2008*/
/*#ifdef __CONFIG_WCN__
			else if (!strcmp(argv[1], "usb"))
				return hotplug_usb();
#endif*/
        /*for mount usb disks, 4m board does not need these codes.*/
#if (defined SAMBA_ENABLE || defined HSDPA) /*  add, FredPeng, 03/16/2009 @HSDPA */
			/* else if (!strcmp(argv[1], "usb"))
				return usb_hotplug(); */
				/*return hotplug_usb();*/
			else if (!strcmp(argv[1], "block"))
                return hotplug_block(); /* wklin modified, 02/09/2011 */
#endif
        /* modified end, water, @usb porting, 11/11/2008*/
		} else {
			fprintf(stderr, "usage: hotplug [event]\n");
			return EINVAL;
		}
	}

	return EINVAL;
}
