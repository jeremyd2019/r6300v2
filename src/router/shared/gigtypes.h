/**
 * \addtogroup common
 * $Id: gigtypes.h 299852 2011-12-01 03:46:47Z $
 */
/*@{*/

/***************************************************
 * Header name: gigtypes.h
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * First written on 13/07/2006 by I. Barrera
 *
 ***************************************************/
/** \file gigtypes.h
 *
 * \brief This contains the definition of types
 *
 * Only general types that will be used for several
 * modules must be defined here
 *
 **************************************************/

/* FILE-CSTYLED */

#ifndef GIGTYPES_H_
#define GIGTYPES_H_

/***************************************************
 *                 Public Defines Section
 ***************************************************/

#define INIT_THROW

/** \brief Boolean type */
typedef enum {
  GIG_FALSE = 0,    /**< Boolean false */
  GIG_TRUE = 1      /**< Boolean true */
} t_bool;

/** \brief Number of possible Boolean values */
#define GIG_N_SWITCH   (2)

/** \brief Signed 8-bit integer */
typedef   signed char  t_int8;

/** \brief Unsigned 8-bit integer */
typedef unsigned char  t_uint8;

/** \brief Signed 16-bit integer */
typedef   signed short t_int16;

/** \brief Unsigned 16-bit integer */
typedef unsigned short t_uint16;

/** \brief Signed 32-bit integer */
typedef   signed int   t_int32;

/** \brief Unsigned 32-bit integer */
typedef unsigned int   t_uint32;

/** \brief Signed 64-bit integer */
typedef   signed long long  t_int64;

/** \brief Unsigned 64-bit integer */
typedef unsigned long long  t_uint64;

/** \brief type for names */
typedef char* t_name ;

/** \brief type for integer/pointer data */
typedef long t_intptr ;

/** \brief type for unsigned integer/pointer data */
typedef unsigned long t_uintptr ;

/** \brief Swap two bytes inside a half word */
#define swap16(x) (              \
  (t_uint16)(((t_uint16)(x) & 0xff) << 8 | ((t_uint16)(x) & 0xff00) >> 8)  \
)

/** \brief Swap four bytes inside a word */
#define swap32(x) (              \
    (t_uint32)(((t_uint32)(x) & 0xff) << 24 |        \
    ((t_uint32)(x) & 0xff00) << 8 | ((t_uint32)(x) & 0xff0000) >> 8 |  \
    ((t_uint32)(x) & 0xff000000) >> 24)    \
)

#define u__(n)  ((unsigned)(n))       /**< \internal */

#define BIT(n)          (1U<<u__(n))  /**< \brief Return value with bit n */

#define BIT_SET(y, mask) \
  ((y) |=  u__(mask))                 /**< \brief Set given bits to 1   */

#define BIT_CLEAR(y, mask) \
  ((y) &= ~u__(mask))                 /**< \brief Clear given bits to 0 */

#define BIT_FLIP(y, mask) \
  ((y) ^=  u__(mask))                 /**< \brief Invert given bits     */

/** \brief  Return bitmask with first n bits set to 1. E.g. 
 *          BITMASK(3) = 00000111
 */
#define BIT_MASK(len) (BIT(len)-1u)

/** \brief  Assigns value to y filtered by mask. \a Important: y is evaluated
 *          twice, once to read it's value and the other to write it.
 */
#define BM_SET(y,value,mask) ((y)=((y)&~(mask))|((value)&(mask)))


#define BF_MASK(start, len) ( BIT_MASK(len) << (start) )
#define BF_PREP(y, start, len) (((y) & BIT_MASK(len)) << (start)) 
#define BF_GET(y, start, len) (((y)>>(start)) & BIT_MASK(len)) 
#define BF_SET(y, x, start, len) (   (y) = ( (y) & ~BF_MASK(start, len) ) | BF_PREP(x, start, len)   )


#define hex__(x)  (0x##x##UL)         /**< \internal */
/** \internal */
#define bin8__(x)  (                  \
    ((x) & 0x0000000FUL ? BIT(0) : 0) \
  | ((x) & 0x000000F0UL ? BIT(1) : 0) \
  | ((x) & 0x00000F00UL ? BIT(2) : 0) \
  | ((x) & 0x0000F000UL ? BIT(3) : 0) \
  | ((x) & 0x000F0000UL ? BIT(4) : 0) \
  | ((x) & 0x00F00000UL ? BIT(5) : 0) \
  | ((x) & 0x0F000000UL ? BIT(6) : 0) \
  | ((x) & 0xF0000000UL ? BIT(7) : 0))
  
/** \brief Easy writing of 8 bit binary literals. eg.
 * 
 *  BIN8(00110101) 
 */
#define BIN8(a)         ((t_uint8)bin8__(hex__(a))) 

/** \brief Easy writing of 16 bit binary literals. eg.
 * 
 *  BIN16(00110101,00101111) 
 */
#define BIN16(a,b)      ((t_uint16)BIN8(a) << 8 | BIN8(b)) 

/** \brief Easy writing of 32 bit binary literals. eg.
 * 
 *  BIN32(00110101,00101111,10101001,00001111) 
 */
#define BIN32(a,b,c,d)  ((t_uint32)BIN16(a,b) << 16 | BIN16(c,d)) 

#else /*GIGTYPES_H_*/
#error "Header file __GIGTYPE_H__ has already been included!"
#endif /*GIGTYPES_H_*/

/*@}*/
