/***************************************************
 * Header name: dict.h
 *
 * Copyright (C) 2011, Broadcom Corporation. All Rights Reserved.
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
 * First written on 03/08/2010 by Toni Homedes i Saun.
 *
 ***************************************************/
/** \file dict.h
 *
 * \ingroup dict
 * 
 * \brief Header file for dict.c
 *
 * $Id: dict.h 298543 2011-11-24 01:30:47Z $
 **************************************************/

#ifndef _DICT_H_
#define _DICT_H_

/* FILE-CSTYLED */

/***************************************************
 *                 Public Defines Section 
 ***************************************************/

/***************************************************
 *                 Public Constants Section  
 ***************************************************/

/***************************************************
 *                 Public Typedefs Section  
 ***************************************************/

/***************************************************
 *                 Public Function Prototypes Section  
 ***************************************************/
typedef const struct dict_hdl_s { int foo; } *dict_hdl_t;
typedef const struct dict_iterator_s { int foo; } *dict_iterator_t;

dict_hdl_t DictNew(void);
void DictFree(dict_hdl_t dict);
void DictSet(dict_hdl_t dict, const char * const key, const char * const value);
const char *DictGet(dict_hdl_t dict, const char * const key);
void DictDelete(dict_hdl_t dict, const char * const key);
int DictIsEmpty(dict_hdl_t dict);
void DictDoEmpty(dict_hdl_t dict);
void DictMap(dict_hdl_t dict, void (*fn)(const char *key, const char *value));

dict_iterator_t DictIteratorNew(dict_hdl_t dict);
void DictIteratorFree(dict_iterator_t iterator);
const char *DictIteratorKey(dict_iterator_t iterator);
int DictIteratorAdvance(dict_iterator_t iterator);

#else
#error "Header file __FILE__ has already been included!"
#endif
