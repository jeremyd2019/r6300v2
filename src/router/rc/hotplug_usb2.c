/***************************************************************************
 ***
 ***    Copyright 2005  Hon Hai Precision Ind. Co. Ltd.
 ***    All Rights Reserved. 
 ***    No portions of this material shall be reproduced in any form without the
 ***    written permission of Hon Hai Precision Ind. Co. Ltd.
 ***
 ***    All information contained in this document is Hon Hai Precision Ind.  
 ***    Co. Ltd. company private, proprietary, and trade secret property and 
 ***    are protected by international intellectual property laws and treaties.
 ***
 ****************************************************************************
 ***
 ***  Filename: hotplug_usb.c
 ***
 ***  Description:
 ***    USB automount function
 ***
 ***  HISTORY:
 ***       - Created Date: 04/24/2009, Water, @USB spec1.7 implement on WNR3500L
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include "bcmconfig.h"
#include "shutils.h"
#include "bcmnvram.h"

//#define USB_HOTPLUG_DBG//@debug
#define MNT_DETACH 0x00000002 /*  add , Jenny Zhao, 04/22/2009 @usb dir */

/***********************************************************************************************************************
* Environment of hotplug for USB:
* DEVICE=bus_num, dev_num
*
* PRODUCT=Vendor, Product, bcdDevice
*
* TYPE= DeviceClass, DeviceSubClass, DeviceProtocol
*
* if DeviceClass == 0 
*  -> INTERFACE=interface [0].altsetting [alt].bInterfaceClass, interface [0].altsetting [alt].bInterfaceSubClass,
*               interface [0].altsetting [alt].bInterfaceProtocol
*
***********************************************************************************************************************/

/*  added start pling 07/13/2009 */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define LOCK           -1
#define UNLOCK          1
#define USB_SEM_KEY     0x5553424B     // "USBK"

int usb_sem_init(void)
{
    struct sembuf lockop = { 0, 0, SEM_UNDO } /* sem operation */ ;
    int semid;
        
    /* create/init sem */ 
    if ((semid = semget (USB_SEM_KEY, 1, IPC_CREAT | IPC_EXCL | 0666)) >= 0) 
    {
        /* initialize the sem vaule to 1 */
        if (semctl (semid, 0, SETVAL, 1) < 0)
        {
            perror("usb_sem_init");
            return -1;
        }
        return 0;
    }
    return -1;
}

/* static */ int usb_sem_lock(int op)
{
    struct sembuf lockop = { 0, 0, SEM_UNDO } /* sem operation */ ;
    int semid;
        
    /* sem already created. get the semid */
    if ((semid = semget (USB_SEM_KEY, 1, 0666)) < 0)
    {
        perror("usb_sem_lock");
        return -1;
    }

    lockop.sem_op = op;
    if (semop (semid, &lockop, 1) < 0)
        return -1;

    return 0;
}

#define USB_LOCK()      usb_sem_lock(LOCK)
#define USB_UNLOCK()    usb_sem_lock(UNLOCK)
/*  added end pling 07/13/2009 */

/*  add start pling 02/05/2010*/
/* USB LED on / off */
#ifdef INCLUDE_USB_LED
#include "wps_led.h"
#include <sys/stat.h>
#include <fcntl.h>

/*  added start, Wins, 04/11/2011 */
#if defined(R6300v2)
#define MAX_BUF_LEN     512
#define USB_MNT_TABLE   "/tmp/usb_mnt_table"
#define USB_MNT_TABLE2  "/tmp/usb_mnt_table2"
#define USB_MNT_PATTERN "PORT=%s,DEVICE=%s,PART=%s"

int get_usb_port(char *pDevPath, char *pUsbPort)
{
    int rtn_val = 0;
    char buf[256];
    char *strPos1 = NULL, *strPos2 = NULL;


    if ((strPos1 = strstr(pDevPath, "/usb")) != NULL) 
    {
        sscanf(strPos1, "/usb%s", buf);    
        if ((strPos2 = strchr(buf, '/')) != NULL) 
        {
            memcpy(pUsbPort, buf, (strPos2 - buf));
            rtn_val = 1;
        }
    }

    return rtn_val;
} /* get_usb_port() */

int parse_target_path(char *pTarget, int *pDevice, int *pPart)
{
    int rtn_val = 0;
    char usb_device[4], usb_part[4];
    char buf[128];
    char *strPos1 = NULL, *strPos2 = NULL;

    /* Get USB device & part */
    memset(usb_device, 0x0, sizeof(usb_device));
    memset(usb_part, 0x0, sizeof(usb_part));
    if ((strPos1 = strstr(pTarget, "/tmp/mnt/usb")) != NULL) {
        sscanf(strPos1, "/tmp/mnt/usb%s", buf);
        if ((strPos2 = strchr(buf, '/')) != NULL) {
            memcpy(usb_device, buf, (strPos2-buf));
            *pDevice = atoi(usb_device);
            strPos2 = NULL;
            if ((strPos2 = strstr(buf, "/part")) != NULL) {
                sscanf(strPos2, "/part%s", usb_part);
                *pPart = atoi(usb_part);
                rtn_val = 1;
            }
        }
    }

    return rtn_val;
} /* parse_target_path() */

int add_into_mnt_file(char *pUsbPort, char *pDevice, char *pPart)
{
    FILE *fp = NULL;
    int rtn_val = 0;
    char buf[128];

    /*  added start, Wins, 06/30/2011 */
    if ((fp = fopen(USB_MNT_TABLE, "r")) != NULL)
    {
        int rec_found = 0;
        char patt[128];
        /* Check if record of USB device exists. */
        while(fgets(buf, sizeof(buf), fp) != NULL)
        {
            memset(patt, 0x0, sizeof(patt));
            sprintf(patt, USB_MNT_PATTERN, pUsbPort, pDevice, pPart);
            strcat(patt, "\n");
            if (!strcmp(buf, patt))
            {
                rec_found = 1;
                break;  /* Record exists. */
            }
        }
        fclose(fp);
        if (rec_found)
            return 1;   /* Not need to add a duplicated record. */
    }
    fp = NULL;
    /*  added end, Wins, 06/30/2011 */

    if ((fp = fopen(USB_MNT_TABLE, "a+")) != NULL)
    {
        fseek(fp, 0, SEEK_END);
        sprintf(buf, USB_MNT_PATTERN, pUsbPort, pDevice, pPart);
        strcat(buf, "\n");
        fputs(buf, fp);
        fclose(fp);
        rtn_val = 1;
    }

    return rtn_val;
} /* add_into_mnt_file() */

int remove_from_mnt_file(char *pUsbPort, char *pDevice, char *pPart)
{
    FILE *fp = NULL, *fp2 = NULL;
    int rtn_val = 0;
    char usb_device[4], usb_part[4];
    char buf[128];
    char *strPos1 = NULL, *strPos2 = NULL;

    if ((fp = fopen(USB_MNT_TABLE, "r+")) != NULL)
    {
        char buf[128];
        sprintf(buf, "cp -f %s %s", USB_MNT_TABLE, USB_MNT_TABLE2);
        system(buf);
        fclose(fp);
    }

    fp = NULL; fp2 = NULL;
    if ((fp2 = fopen(USB_MNT_TABLE2, "r+")) != NULL)
    {
        char buf[MAX_BUF_LEN];
        char patt[MAX_BUF_LEN];
        sprintf(patt, USB_MNT_PATTERN, pUsbPort, pDevice, pPart);
        if ((fp = fopen(USB_MNT_TABLE, "w+")) != NULL)
        {
            while (fgets(buf, MAX_BUF_LEN, fp2) != NULL)
            {
                if ((strPos1 = strstr(buf, patt)) != NULL) {
                    continue;
                }
                else {
                    fputs(buf, fp);
                }
            }
            fclose(fp);
            rtn_val = 1;
        }
        fclose(fp2);

        sprintf(buf, "rm -f %s", USB_MNT_TABLE2);
        system(buf);
    }

    return rtn_val;
} /* remove_from_mnt_file() */

int usb1_led(void)
{
    int rtn_val = 0;
    int has_usb1_dev = 0;
    int fd;
    FILE *fp = NULL;
    char line[250] = "";

    if ((fp = fopen(USB_MNT_TABLE, "r")) != NULL)
    {
        rtn_val = 1;
        while (fgets(line, 200, fp) != NULL) {
            if ( (strstr(line, "PORT=1") != NULL) || (strstr(line, "PORT=3") != NULL))
            {
                has_usb1_dev = 1;
                rtn_val = 2;
                break;
            }
        }
        fclose(fp);
    }
    /*  wklin modified start, 01/19/2011 @ USB LED for WNDR4000 */
#if (defined GPIO_EXT_CTRL) /* WNDR4000 */
    if (has_usb1_dev)
        system("gpio usbled 1");
    else
        system("gpio usbled 0");
#else
    fd = open("/dev/wps_led", O_RDWR);
    if (fd >= 0) {
        rtn_val = 3;
        if (has_usb1_dev)
        {
            ioctl(fd, USB_LED_STATE_ON, 1);
            rtn_val = 4;
        }
        else
        {
            ioctl(fd, USB_LED_STATE_OFF, 1);
            rtn_val = 5;
        }
        close(fd);
    }
#endif /* GPIO_EXT_CTRL */
    /*  wklin modified end ,01/19/2011 */
    return rtn_val;
} /* usb1_led() */

int usb2_led(void)
{
    int rtn_val = 0;
    int has_usb2_dev = 0;
    int fd;
    FILE *fp = NULL;
    char line[250] = "";

    if ((fp = fopen(USB_MNT_TABLE, "r")) != NULL)
    {
        rtn_val = 1;
        while (fgets(line, 200, fp) != NULL) {
            if (strstr(line, "PORT=2") != NULL) {
                has_usb2_dev = 1;
                rtn_val = 2;
                break;
            }
        }
        fclose(fp);
    }
    /*  wklin modified start, 01/19/2011 @ USB LED for WNDR4000 */
#if (defined GPIO_EXT_CTRL) /* WNDR4000 */
    if (has_usb2_dev)
        system("gpio usbled 1");
    else
        system("gpio usbled 0");
#else
    fd = open("/dev/wps_led", O_RDWR);
    if (fd >= 0) {
        rtn_val = 3;
        if (has_usb2_dev)
        {
            ioctl(fd, USB2_LED_STATE_ON, 1);
            rtn_val = 4;
        }
        else
        {
            ioctl(fd, USB2_LED_STATE_OFF, 1);
            rtn_val = 5;
        }
        close(fd);
    }
#endif /* GPIO_EXT_CTRL */
    /*  wklin modified end ,01/19/2011 */
    return rtn_val;
} /* usb2_led() */

int usb_dual_led(void)
{
    int rtn_val = 0;

    usb1_led();
    usb2_led();

    return rtn_val;
} /* usb_dual_led() */
#else /* R6300v2 */

int usb_led(void)
{
    int has_usb_dev = 0;
    int fd;
    FILE *fp = NULL;
    char line[250] = "";

    fp = fopen("/proc/mounts", "r");
    if (fp != NULL) {
        while (fgets(line, 200, fp) != NULL) {
            if (strstr(line, "/tmp/mnt/usb") != NULL) {
                has_usb_dev = 1;
                break;
            }
        }
        fclose(fp);
    }
    /*  wklin modified start, 01/19/2011 @ USB LED for WNDR4000 */
#if (defined GPIO_EXT_CTRL) /* WNDR4000 */
#if 0   /*  removed pling 12/26/2011, not needed for WNDR4000AC */
    if (has_usb_dev)
        system("gpio usbled 1");
    else
        system("gpio usbled 0");
#endif
#else
    fd = open("/dev/wps_led", O_RDWR);
    if (fd >= 0) {
        if (has_usb_dev)
            ioctl(fd, USB_LED_STATE_ON, 1);
        else
            ioctl(fd, USB_LED_STATE_OFF, 1);
        close(fd);
    }
#endif /* GPIO_EXT_CTRL */
    /*  wklin modified end ,01/19/2011 */
    return 0;
}
#endif /* R6300v2 */
/*  added end, Wins, 04/11/2011 */
#endif
/*  add end pling 02/05/2010*/

int usb_mount(void)
{
    char source[128];
    char target[128];
    char buf[128];
    int i;
    int rval;

    /* add start, @mount approved devices, water, 05/11/2009*/
    int index = 0;
    FILE *fp = NULL;
    char line[150] = "";
    char *ptmp = NULL;
    char vendor[32] = "";
    char model[32] = "";
    int  scsi_host_num = -1;
    int usb_dev_approved[26] = {0};
    char approved_usb[20][80] = {0};
    int not_approved_index;

    /*  added start pling 09/16/2009 */
    int last_scsi_host_num = -1;
    /*  added end pling 09/16/2009 */

    sleep(3);   /* pling added 06/04/2009, add this delay seems to make mount more robust. */

    if (nvram_match("enable_any_usb_dev", "0") )
    {
        for (i=0; i<20; i++)
        {
            sprintf(line, "approved_usb_%d", i);
            strcpy(buf, nvram_safe_get(line));
            if (strlen (buf) != 0)
            {
                sscanf(buf, "%*[^+]+%[^+]+%*[^+]", approved_usb[i]);
            }
        }
        
        fp = fopen("/proc/scsi/scsi", "r");
        if (fp != NULL)
        {
            while (fgets(line, 150, fp) != NULL)
            {
                if (strncmp(line, "Host: scsi", strlen("Host: scsi")) == 0)
                {
                    sscanf(line, "Host: scsi%d %*s", &scsi_host_num);

                    /*  added start pling 09/16/2009 */
                    /* Handle multi-lun devices */
                    if (scsi_host_num == last_scsi_host_num)
                    {
                        /* Keep the same "approved"/"not-approved" status
                         * as the previous LUN
                         */
                        usb_dev_approved[index] = usb_dev_approved[index-1];
                        index++;
                        continue;
                    }
                    /*  added end pling 09/16/2009 */

                    if ((fgets(line, 150, fp) == NULL))
                        break;
                    
                    ptmp=strstr(line, "Vendor:");
                    if ( ptmp != NULL )
                    {
                        sscanf(ptmp, "Vendor: %s %*s", vendor);
                        if (0 == strcmp(vendor, "Model:") )
                            strcpy(vendor, "");
                    }
                    ptmp=strstr(line, "Model:");
                    if ( ptmp != NULL )
                    {
                        sscanf(ptmp, "Model: %s %*s", model);
                        if (0 == strcmp(vendor, "Rev:") )
                            strcpy(vendor, "");
                    }
                    sprintf(buf, "%s %s", vendor, model);

                    for (i=0; i<20; i++)
                    {
                        if (strlen(approved_usb[i]) == 0 )
                            continue;
                        if (0 == strcmp(approved_usb[i], buf) )
                        {/*this usb device was approved*/
                            /*  modified start pling 09/16/2009 */
                            /* Use 'index' to help check multi-LUN device */
                            //usb_dev_approved[scsi_host_num] = 1;
                            usb_dev_approved[index] = 1;
                            /*  modified end pling 09/16/2009 */
                            break;
                        }
                    }
                    /*  added start pling 09/16/2009 */
                    /* Multi-LUN devices */
                    index++;
                    last_scsi_host_num = scsi_host_num; 
                    /*  added end pling 09/16/2009 */
                }
            }
            fclose(fp);
        }
    }
    else
    {
        for (i=0; i<26; i++)
            usb_dev_approved[i] = 1;
    }
    not_approved_index = 0;
    /* add end, water, 05/11/2009*/
    
    index = 0;      // pling added 09/16/2009, reset index for later use

//------------------------------------------------------
// action: add
// 1. mount dev with vfat
// 2. if mount ok, save data to nvram
// 3. restart smb
//------------------------------------------------------
//fixme: no good. zzz@
/*  modified start, Jenny Zhao, 04/13/2009 @usb dir */
    char diskName;
    int j = 0;
    int max_partition = atoi(nvram_safe_get("usb_disk_max_part"));
    for (diskName='a'; diskName<='z'; diskName++)   // try to mount sda->sdz
    {
        /* add start, @mount approved devices, water, 05/11/2009*/
        if (usb_dev_approved[index++] == 1)/*this device was approved...*/
            scsi_host_num = 1;
        else
            scsi_host_num = 0;
        /* add end, water, 05/11/2009*/
        for (i=0; i<=max_partition; i++)            // try to mount sdx or sdx1, ..., sdx5
        {
            //------------------------
            //set up source
            //------------------------
            if (!i)
                snprintf(source, 128, "/dev/sd%c", diskName);
            else
                snprintf(source, 128, "/dev/sd%c%d", diskName,i);
            
            //------------------------
            //set up target
            //------------------------
            /* modified start, water, 05/12/2009*/
            /*some usb device not in approved device list, but the approved device page
                need to show its info, so it need mount too. */
            //snprintf(target, 128, "/tmp/mnt/usb%d/part%d", j, i);
            if (scsi_host_num == 1)
                snprintf(target, 128, "/tmp/mnt/usb%d/part%d", j, i);
            else
                snprintf(target, 128, "/tmp/mnt/not_approved%d", not_approved_index++);
            /* modified end, water, 05/12/2009*/
    
            //------------------------
            //start mount with sync
            //------------------------       
            //rval = mount(source, target, "vfat",  0, NULL);
            rval = mount(source, target, "vfat",0, "iocharset=utf8"); //stanley add,09/14/2009
            /* pling added start 05/07/2009 */
            /* Use mlabel to read VFAT volume label after successful mount */
            if (rval == 0)
            {
                /* , add-start by MJ., for downsizing WNDR3400v2,
                 * 2011.02.11. */
#if (defined WNDR3400v2) || (defined R6300v2)
                snprintf(buf, 128, "/lib/udev/vol_id %s", source);
                /*  added start pling 12/26/2011, for WNDR4000AC */
#elif (defined WNDR4000AC)
                get_vol_id(source);
                memset(buf, 0, sizeof(buf));
                /*  added end pling 12/26/2011, for WNDR4000AC */
#else
                /* , add-end by MJ., for downsizing WNDR3400v2,
                 * 2011.02.11. */
                snprintf(buf, 128, "/usr/bin/mlabel -i %s -s ::", source);
#endif
                system(buf);
            }
            /* pling added end 05/07/2009 */            
    
#ifdef USB_HOTPLUG_DBG//debug
            snprintf(buf, 128, "echo \"mount %s %s with vfat, rval:%d, errno=%d\">>/tmp/usberr.log",
                               source, target, rval, errno);
            system(buf);
#endif        

            /* pling added start 08/24/2009 */
            /* To speed up mounting: 
             *  if sda is mounted, then don't bother about sda1, sda2... 
             */
            if (rval == 0 && i == 0)
                break;
            /* pling added end 08/24/2009 */

            if (rval<0 && errno == EINVAL)
            {
                int ret;
                /* Use NTFS-3g driver to provide NTFS r/w function */
                snprintf(buf, 128, "/bin/ntfs-3g -o large_read %s %s 2> /dev/null", source, target);
                ret = system(buf);

#ifdef USB_HOTPLUG_DBG//debug
                snprintf(buf, 128, "echo \"try to mount %s @ %s with ntfs-3g\">>/tmp/usberr.log", source, target);
                system(buf);
#endif

                /*  added pling 08/24/2009 */
                /* To speed up mounting: 
                 *  if sda is mounted, then don't bother about sda1, sda2... 
                 */
                if (WIFEXITED(ret))
                {
                    int status = WEXITSTATUS(ret);
                    if (status == 0 && i == 0)
                        break;
                }
                /*  added pling 08/24/2009 */
            }
        } //end of for
        j++;
    }//end of for(diskName = 'a';diskName < 'd';diskName++)
    /*  modified end, Jenny Zhao, 04/13/2009 */
    //nvram_set("usb_dev_no_change", "0");
    /* send signal to httpd ,create link ,2009/05/07, jenny*/
    //nvram_set("usb_mount_signal", "1");
    system("killall -SIGUSR2 httpd");
    /* USB LED on / off */
#ifdef INCLUDE_USB_LED
    /*  modified start, Wins, 04/11/2011 */
#if defined(R6300v2)
    usb_dual_led();
#else /* R6300v2 */
    usb_led();
#endif /* R6300v2 */
    /*  modified end, Wins, 04/11/2011 */
#endif
    
    //sleep(5);   //  added pling 07/13/2009, wait for httpd to complete mount/umount processes
    //nvram_set("usb_mount_signal", "0");
    //acosNvramConfig_save();
    return 0;
}

int usb_umount(void)
{
    int i, j;
    int rval;
    char path[128];
    char buf[128];
    //------------------------------------------------------
    // action: remove
    // remount devices
    //------------------------------------------------------
    //fixme: no good. zzz@
    /*  modify start, Jenny Zhao, 04/13/2009 @usb dir */
    char diskName;
    
    /* add start, water, 05/12/2009*/
    /*some usb device not in approved device list, but the approved device 
     page need to show its info, so it need mount too. */
    for (j=0; j<20; j++)
    {
        snprintf(path, 128, "/tmp/mnt/not_approved%d", j);
        umount2(path, MNT_DETACH);
    }
    /* add end, water, 05/12/2009*/
    
    for (diskName='a', i=0; diskName<='z'; diskName++,i++)
    {
        snprintf(path, 128, "/tmp/mnt/usb%d/part0", i);
    
        rval = umount2(path, MNT_DETACH);
        
        for (j=0; j<16; j++)
        {
            snprintf(path, 128, "/tmp/mnt/usb%d/part%d", i, j);
    
            rval = umount2(path, MNT_DETACH);
            /* pling added start 05/07/2009 */
            /* remove volume name file under /tmp after a successful umount */
            if (rval == 0)
            {
                char filename[64];
                if (j == 0)
                    sprintf(filename, "/tmp/usb_vol_name/sd%c", diskName);
                else
                    sprintf(filename, "/tmp/usb_vol_name/sd%c%d", diskName, j);
                unlink(filename);
            }
            /* pling added end 05/07/2009 */

#ifdef USB_HOTPLUG_DBG//debug
            snprintf(buf, 128, "echo \"umount %s, rval:%d \">>/tmp/usberr.log", path, rval);
            system(buf);
#endif
        }
    }
    /*  modify end, Jenny Zhao, 04/13/2009 */  
    nvram_set("usb_dev_no_change", "0");
    //acosNvramConfig_save();
      
    usb_mount();
    return 0;
}
int usb_hotplug(void)
{
    char *device, *interface;
    char *action, *product;
    char *type, *devfs;
    int class, subclass, protocol;
    char cmd[512];
    
    /* add start, water, 05/11/2009*/
    if (nvram_match("restart_usb", "1") )
    {
        usb_umount();
        nvram_unset("restart_usb");
        //eval("/usr/bin/setsmbftpconf");
    }
    /* add end, water, 05/11/2009*/
    
    if (!(action = getenv("ACTION")) ||
        !(type = getenv("TYPE")) ||
        !(device = getenv("DEVICE")) ||
        !(devfs = getenv("DEVFS")) ||                
        !(interface = getenv("INTERFACE")) ||
        !(product = getenv("PRODUCT")))
    {
#ifdef USB_HOTPLUG_DBG//debug
        sprintf(cmd, "echo \"return EINVAL;\" >> /tmp/hotAct\n");    
        system(cmd);
#endif
        return EINVAL;
    }

#ifdef USB_HOTPLUG_DBG//debug
        sprintf(cmd, "echo \"ACTION=%s, TYPE=%s, DEVICE=%s, DEVFS=%s, INTERFACE=%s, PRODUCT=%s\" >> /tmp/hotAct\n", 
                action, type, device, devfs, interface, product);
        system(cmd);
#endif 

    sscanf(type, "%d/%d/%d", &class, &subclass, &protocol);
    if (class == 0) 
    {
        sscanf(interface, "%d/%d/%d", &class, &subclass, &protocol);
    }
    //set LED
    //set_usb_led();/*No usb led in WNR3500U board, need further check.*/

    //check Mass Storage for add action
    /*  modified start pling 11/11/2009 */
    /* We should mount all subclasses of Mass Storage class (8), 
     *  not just subclass 6 (Transparent SCSI).
     */
    //if (class == 8 && subclass == 6  && !strcmp(action, "add"))
    if (class == 8 && !strcmp(action, "add"))
    /*  modified end pling 11/11/2009 */
    {
        /*
        sometimes usb_umount() not execute when unplug usb.
        so before mount, we do umount firstly.
        not sure, I think it need further test.
        */
        //usb_mount();
        USB_LOCK();     //  added pling 07/13/2009
        usb_umount();/*water, 11/06/2008*/
        USB_UNLOCK();   //  added pling 07/13/2009
    }
    //check Mass Storage for remove action
    /*  modified start pling 11/11/2009 */
    /* We should un-mount all subclasses of Mass Storage class (8), 
     *  not just subclass 6 (Transparent SCSI).
     */
    //else if (class == 8 && subclass == 6  && !strcmp(action, "remove"))
    else if (class == 8 && !strcmp(action, "remove"))
    /*  modified end pling 11/11/2009 */
    {
        USB_LOCK();     //  added pling 07/13/2009
        usb_umount();
        USB_UNLOCK();   //  added pling 07/13/2009
    }
    
    //eval("/usr/bin/setsmbftpconf");
#ifdef USB_HOTPLUG_DBG//debug
    system("echo \"/usr/bin/setsmbftpconf\">>/tmp/usberr.log");
#endif
    return 0;
}
