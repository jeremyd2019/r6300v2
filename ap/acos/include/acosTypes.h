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
#****************************************************************************
***
***    Filename: acosTypes.h
***
***    Description:
***         This is the header file containing basic type definitions of ACOS libraries.
***         The following definition refer the fxxing Ambit Coding Rule D01.1.
***         When this header file is included, programmer should avoid re-declaration issue. Because
***         if there is same definition respective to different type. There will be some trouble.
***
***    History:
***
***	   Modify Reason		                                        Author		         Date		                Search Flag(Option)
***	--------------------------------------------------------------------------------------
***    File Creation                                            Jasmine Yang    11/02/2005
*******************************************************************************
***
***
***    Caution: 1. If you would like to use vxWorks's general types and only ACOS "IN" "OUT"
***                definitions. Please include "vxworks.h" first than include "acosTypes.h" to
***                avoid compiler error.
***                
***                Example:
***                         #include <vxWorks.h>
***                         #include "acosTypes.h"
***                         int test(IN int nArg)   //ACOS definition(IN)
***                         {
***                             UINT8 ucTest;       //ACOS definition(UINT8)
***                             UINT16 usTest;       //ACOS definition(UINT16)
***                             UINT32 ulTest;       //vxWorks definition(UINT32)
***                             IMPORT int nTest;   //vxWorks definition(IMPORT)
***                             return OK;          //vxWorks definition(OK)
***                         }
***
***
***             2. If you would like to use only ACOS general types as well as vxWorks's extra definitions,
***                please don't include "vxworks.h" before including "acosTypes.h"
***
***                Example:
***                         #include "acosTypes.h" 
***                         #include <vxWorks.h>
***                         int test(IN int nArg)   //ACOS definition(IN)
***                         {
***                             UINT8 ucTest;       //ACOS definition(UINT8)
***                             UINT16 usTest;       //ACOS definition(UINT16)
***                             UINT32 ulTest;       //ACOS definition(UINT32)
***                             IMPORT int nTest;   //vxWorks definition(IMPORT)
***                             return OK;          //ACOS definition(OK)
***                         }
***
***
***             3. If you are building ACOS library, you should never include "vxworks.h"
***
***                Example:
***                         #include "acosTypes.h" 
***                         int test(IN int nArg)   //ACOS definition(IN)
***                         {
***                             UINT8 ucTest;       //ACOS definition(UINT8)
***                             UINT16 usTest;       //ACOS definition(UINT16)
***                             UINT32 ulTest;       //ACOS definition(UINT32)
***                             return OK;          //ACOS definition(OK)
***                         }
********************************************************************************************************/
#ifndef _ACOS_TYPES_H
#define _ACOS_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef bool
#define bool int
#define false 0
#define true  1
#endif

#if !defined(__INCvxWorksh)     /* not include vxWorks.h */

#if	!defined(FALSE) || (FALSE!=0)
#define FALSE   0
#endif

#if	!defined(TRUE) || (TRUE!=1)
#define TRUE    1
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

/* return status values */
#if	!defined(OK) || (OK!=0)
#define OK		0
#endif

#if	!defined(ERROR) || (ERROR!=(-1))
#define ERROR   (-1)
#endif

/* ACOS types */
#ifndef INT8
#define INT8    char
#endif

#ifndef INT16
#define INT16   short
#endif

#ifndef INT32
#define INT32   long
#endif

#ifndef UINT8
#define UINT8   unsigned char
#endif

#ifndef UNIT16
#define UINT16  unsigned short
#endif

#ifndef UNIT32
#define UINT32  unsigned int
#endif

#ifndef BOOL
#define BOOL    int
#endif

#ifndef STATUS
#define STATUS  int
#endif

#ifndef uint8
#define uint8 unsigned char
#endif

#ifndef uint16
#define uint16 unsigned int
#endif

#ifndef uint32
#define uint32 unsigned int
#endif

#ifndef UCHAR
#define	UCHAR unsigned char
#endif

#ifndef USHORT
#define USHORT unsigned short
#endif

#ifndef UINT
#define UINT unsigned int
#endif

#ifndef ULONG
#define ULONG unsigned long
#endif

#ifndef INLINE

#ifdef _MSC_VER

#define INLINE __inline

#elif __GNUC__

#define INLINE __inline__

#else

#define INLINE

#endif /* _MSC_VER */

#endif /* INLINE */

#ifndef IMPORT
#define IMPORT extern
#endif

#ifndef LOCAL
#define LOCAL static
#endif

#endif                          /* __INCvxWorksh */

#ifdef __cplusplus
}
#endif

/* here defined general constant*/
#ifndef MAX_IF_NAME_LEN
#define MAX_IF_NAME_LEN         20      /* mirror, ixe0, ixe1 ..., brenda add, 2003/07/24 */
#endif

/* CMI definitions */
//#include "acosCmiTypes.h"
//#include "epivers.h"
/* Driver Version String, ASCII, 32 chars max */
#ifndef EPI_VERSION_STR
#define	EPI_VERSION_STR		"3.90.23.0"
#define	EPI_ROUTER_VERSION_STR	"3.91.23.0"
#endif
#ifndef EZC_VERSION_STR
#define EZC_VERSION_STR		"2"
#endif

#ifndef MAX_NVPARSE
/* 256 entries per list */
#define MAX_NVPARSE 256         //in "Nvparse.h"
#endif

#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN 6
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a):(b))
#endif
#endif                          /* _ACOS_TYPES_H */
