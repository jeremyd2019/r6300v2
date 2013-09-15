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
 ***  History:
 ***
 ***  Modify Reason             Author          Date             Search Flag(Option)
 ***--------------------------------------------------------------------------------------
 ***  Created                   zZz            09/14/2007
 **   Port to WNR3500U          Water          10/31/2008
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include "bcmconfig.h"

//#define USB_HOTPLUG_DBG//@debug

#if 0
static set_usb_led()
{
    FILE *fp;
    unsigned char line[256];
    unsigned char port[2] = {0, 0};
    char cmd[32];
    
    fp = fopen("/proc/bus/usb/devices", "r");
    
    if(fp == NULL) 
    {
        fprintf(stderr, "open usb info failure\n");
    }
    
//------------------
//reset flag
//------------------
    port[0] = 0;
    port[1] = 0;
    
//----------------------------
//parse /proc/bus/usb/devices
//----------------------------
    while(1)
    {
        fgets(line, 256, fp);

    //----------------------------
    //parse Toplogy info
    //----------------------------
        if (strncmp(line, "T: ", 3) == 0)
        {
            char *subtoken;
            char *saveptr1, *saveptr2;
            char *token = strtok_r(line, " \t\n", &saveptr1);
            int   tmpNum;
                
            if (token)
            {
                while (1)
                {
                    token = strtok_r(NULL, " \t\n", &saveptr1);
                    if (token == NULL)
                    {
                        break;
                    }
                    //fprintf(stderr, "%s \n", token);
                    if (strncmp(token, "Prnt=", 5) == 0 )
                    {
                    //-----------------------------------------
                    //Parent = 0x1 -> leaf of root hub -> a node
                    //Then read Port: port 0 -> USB1
                    //                port 1 -> USB2
                    //-----------------------------------------
                        //fprintf(stderr, "%s ", token);
                        subtoken = strtok_r(token, "=", &saveptr2);
                        //fprintf(stderr, "%s ", subtoken);
                        subtoken = strtok_r(NULL, "=", &saveptr2);
                        //fprintf(stderr, "%s ", subtoken);
                           
                        tmpNum = atoi(subtoken);
                        if (tmpNum == 1)
                        {
                        //------------------
                        //get port
                        //------------------
                            token = strtok_r(NULL, " \t\n", &saveptr1);
                            //fprintf(stderr, "%s ", token);
                            subtoken = strtok_r(token, "=", &saveptr2);
                            //fprintf(stderr, "%s ", subtoken);
                            subtoken = strtok_r(NULL, "=", &saveptr2);
                            //fprintf(stderr, "%s ", subtoken);
                           
                        //------------------
                        //set flag
                        //------------------
                            tmpNum = atoi(subtoken);
                            if (tmpNum == 0)
                            {
                                port[0] = 1;
                            }
                            else if(tmpNum == 1)
                            {
                                port[1] = 1;
                            }
                            else
                            {
                                fprintf(stderr, "unexcepted port #\n");
                            }
                            break;
                        } //parse port if parent is 1 (root)
                    } //parset parent
                } //while 1
            } //if token
        } //strcmp T:
        if (feof(fp))
        {
            break;
        }
    } //while 1 -> read lines in files
    fclose(fp);
    //------------------
    //set LED
    //------------------
    snprintf(cmd, 32, "/sbin/gpio 9 %d", (int)port[0]);
    system(cmd);
    
    snprintf(cmd, 32, "/sbin/gpio 10 %d", (int)port[1]);
    system(cmd);    
}
#endif/*No usb led in WNR3500U board, need further check.*/

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

int usb_mount()
{
    char source[128];
    char target[128];
    char buf[128];
    int i;
    int rval;
//------------------------------------------------------
// action: add
// 1. mount dev with vfat
// 2. if mount ok, save data to nvram
// 3. restart smb
//------------------------------------------------------
//fixme: no good. zzz@
    for (i=1; i<16; i++ )
    {
#if 0
        //------------------------
        //set up source
        //------------------------
        snprintf(source, 128, "/dev/sda%d", i);
        
        //------------------------
        //set up target
        //------------------------
        snprintf(target, 128, "/tmp/mnt/usb%d", i);

        //------------------------
        //start mount with sync
        //------------------------       
        rval = mount(source, target, "vfat", MS_SYNCHRONOUS, 0);

#ifdef USB_HOTPLUG_DBG//debug
        snprintf(buf, 128, "echo \"mount %s %s with vfat, rval:%d \">>/tmp/usberr.log", source, target, rval);
        system(buf);
#endif        
        if (rval)
        {
            rval = mount(source, target, "ntfs", MS_RDONLY, 0);
#ifdef USB_HOTPLUG_DBG//debug
            snprintf(buf, 128, "echo \"mount %s %s with ntfs, rval:%d \">>/tmp/usberr.log", source, target, rval);
            system(buf);
#endif
        }
        /* add start, water, 12/08/2008*/
#endif/*if 0, removed water, 12/08/2008, @write throughput too low.*/
        /*If we do mount by key in mount command in the console. the write throughput is ok.*/
        /*I think there're some bugs in the above mount function. But the mount command build by busybox is ok.*/
        /*So we use mount command here. It isn't a good solution. Just a "work around".*/
        snprintf(buf, 128, "/bin/mount -t vfat /dev/sda%d /tmp/mnt/usb%d", i, i);
        system(buf);
        
        /*ntfs read only supported*/
        snprintf(buf, 128, "/bin/mount -t ntfs /dev/sda%d /tmp/mnt/usb%d", i, i);
        system(buf);
        /* add end, water, 12/08/2008*/
    } //end of for
    
    return 0;
}

int usb_umount()
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

    for (j=1; j<16; j++)
    {
        snprintf(path, 128, "/bin/umount /tmp/mnt/usb%d", j);

        rval = system(path);
#ifdef USB_HOTPLUG_DBG//debug
        snprintf(buf, 128, "echo \"umount %s, rval:%d \">>/tmp/usberr.log", path, rval);
        system(buf);
#endif
    }
    
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
    if (class == 8 && subclass == 6  && !strcmp(action, "add"))
    {
        /*
        sometimes usb_umount() not execute when unplug usb.
        so before mount, we do umount firstly.
        not sure, I think it need further test.
        */
        //usb_mount();
        usb_umount();/*water, 11/06/2008*/
    }
    //check Mass Storage for remove action
    else if (class == 8 && subclass == 6  && !strcmp(action, "remove"))
    {
        usb_umount();
    }
    
    system("/usr/bin/setsmbftpconf");
#ifdef USB_HOTPLUG_DBG//debug
    system("echo \"/usr/bin/setsmbftpconf\">>/tmp/usberr.log");
#endif
    return 0;
}
