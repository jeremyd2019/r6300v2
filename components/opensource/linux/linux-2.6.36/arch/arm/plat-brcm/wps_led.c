/***************************************************************************
***
***    Copyright 2008  Hon Hai Precision Ind. Co. Ltd.
***    All Rights Reserved.
***    No portions of this material shall be reproduced in any form without the
***    written permission of Hon Hai Precision Ind. Co. Ltd.
***
***    All information contained in this document is Hon Hai Precision Ind.  
***    Co. Ltd. company private, proprietary, and trade secret property and 
***    are protected by international intellectual property laws and treaties.
***
****************************************************************************/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#include <typedefs.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmdevs.h>

#include "wps_led.h"

/*#define _DEBUG*/


#ifdef CONFIG_DEVFS_FS
static int wps_led_major;
devfs_handle_t wps_leddev_handle;
#endif /* CONFIG_DEVFS_FS */

extern int wps_led_pattern;
extern int wps_led_state;
extern int is_wl_secu_mode; 

static int
wps_led_open(struct inode *inode, struct file * file)
{
    MOD_INC_USE_COUNT;
    return 0;
}

static int
wps_led_release(struct inode *inode, struct file * file)
{
    MOD_DEC_USE_COUNT;
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)   
static int
#else
static long
#endif
wps_led_ioctl(
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)   
struct inode *inode, 
#endif
struct file *file, 
unsigned int cmd, 
unsigned long arg)
{
	
    /* Add USB LED support.
     * Note: need to add at the beginning the 'arg' check below.
     *       Otherwise, we might change the wireless LED
     *       by accident.
    */
#if (defined INCLUDE_USB_LED)
    if (cmd == USB_LED_STATE_ON || cmd == USB_LED_STATE_OFF)
    {
#if defined(R6300v2) || defined(R6250)
        extern int usb1_led_state;

        if (cmd == USB_LED_STATE_ON)
            usb1_led_state = 1;
        else
            usb1_led_state = 0;

#elif (defined WNDR4000AC) 
		/* Do nothing here */

#else /* R6300v2 */
        extern int usb_led_state;
        if (cmd == USB_LED_STATE_ON)
            usb_led_state = 1;
        else
            usb_led_state = 0;
#endif /* R6300v2 */
        return 0;
    }

#if defined(R6300v2)
    if (cmd == USB2_LED_STATE_ON || cmd == USB2_LED_STATE_OFF)
    {
        extern int usb2_led_state;

        if (cmd == USB2_LED_STATE_ON)
            usb2_led_state = 1;
        else
            usb2_led_state = 0;
        return 0;
    }
#endif /* R6300v2 */
#endif

    if (arg)
        is_wl_secu_mode = 1;
    else
        is_wl_secu_mode = 0;

    switch (cmd)
    {
        case WPS_LED_BLINK_NORMAL:
            wps_led_state = 1;
#ifdef _DEBUG
            printk("%s: blink normal\n", __FUNCTION__);
#endif
            break;

        case WPS_LED_BLINK_QUICK:
            wps_led_state = 2;
#ifdef _DEBUG
            printk("%s: blink WPS\n", __FUNCTION__);
#endif
            break;

        case WPS_LED_BLINK_OFF:
            /* wps_led_state will change to 0 automatically after
             * blinking a few seconds if it's 2 currently
             */
            if (wps_led_state != 2)
                wps_led_state = 0;
#ifdef _DEBUG
            printk("%s: blink OFF\n", __FUNCTION__);
#endif
            break;

        case WPS_LED_CHANGE_GREEN:
            break;

        case WPS_LED_CHANGE_AMBER:
            break;

        case WPS_LED_BLINK_QUICK2:
            wps_led_state = 3;
            break;
            
        case WPS_LED_BLINK_AP_LOCKDOWN:
            wps_led_state = 4;
            break;

        default:
            break;
    }

    return 0;
}

static struct file_operations wps_led_fops = {
#ifndef CONFIG_DEVFS_FS
    .owner      = THIS_MODULE,
    .open       = wps_led_open,
    .release    = wps_led_release,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)    
    .ioctl      = wps_led_ioctl,
#else    
    .unlocked_ioctl = wps_led_ioctl,
#endif    
#else /* CONFIG_DEVFS_FS */
    owner:      THIS_MODULE,
    open:       wps_led_open,
    release:    wps_led_release,
    ioctl:      wps_led_ioctl,
#endif /* CONFIG_DEVFS_FS */
    };

static int __init
wps_led_init(void)
{
#ifndef CONFIG_DEVFS_FS
    int ret_val;
    /*
    * Register the character device (atleast try)
    */
    ret_val = register_chrdev(WPS_LED_MAJOR_NUM, "wps_led", &wps_led_fops);
    /*
    * Negative values signify an error
    */
    if (ret_val < 0) 
    {
        printf("%s failed with %d\n","Sorry, registering the character device wps_led", ret_val);
        return ret_val;
    } 
#else /* CONFIG_DEVFS_FS */
    if ((wps_led_major = devfs_register_chrdev(WPS_LED_MAJOR_NUM, "wps_led", &wps_led_fops)) < 0)
        return wps_led_major;

    wps_leddev_handle = devfs_register(NULL, "wps_led", DEVFS_FL_DEFAULT,
                                    wps_led_major, 0, S_IFCHR | S_IRUGO | S_IWUGO,
                                    &wps_led_fops, NULL);
#endif /* CONFIG_DEVFS_FS */
    return 0;
}

static void __exit
wps_led_exit(void)
{
#ifndef CONFIG_DEVFS_FS

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
    int ret_val;
    ret_val = unregister_chrdev(WPS_LED_MAJOR_NUM, "wps_led");
#else
	unregister_chrdev(WPS_LED_MAJOR_NUM, "wps_led");
#endif    
#else /* CONFIG_DEVFS_FS */
    if (wps_leddev_handle != NULL)
        devfs_unregister(wps_leddev_handle);
    wps_leddev_handle = NULL;
    devfs_unregister_chrdev(wps_led_major, "wps_led");
#endif /* CONFIG_DEVFS_FS */
}

module_init(wps_led_init);
module_exit(wps_led_exit);
