/***************************************************
 * File name: dict.c
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
/** \file dict.c 
 */
/** \defgroup dict  Simple Dictionnary
 *  
 *  \ingroup gigled Gigle Daemon  
 *   
 *  \brief Simple key/value dictionary implementation
 *
 *  Simples't possible implementation of a dictionary. Expected number of
 *  entries is very low (about 10~20) so emphasis has been given to simplicity
 *  to reduce risk
 *  
 *  The dictionnary is based on a single linked list and a head pointer. 
 *  Elements are always unshifted at the front of the list; no order is kept.
 * 
 * $Id: dict.c 298543 2011-11-24 01:30:47Z $
 */
/*@{*/ 

/***************************************************
*                 Include section
***************************************************/

/* FILE-CSTYLED */

#include "dict.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***************************************************
 *                 Local Defines Section
 ***************************************************/

/***************************************************
 *                 Local Constants Section
 ***************************************************/

/***************************************************
 *                 Local Typedefs Section
 ***************************************************/

/** \brief Dict list node
 */
typedef struct s_dict_node {
  struct s_dict_node *next; /**< \brief Pointer to next node of NULL if NIL */
  char * key;               /**< \brief Pointer to malloc'ed key copy       */
  char * value;             /**< \brief Pointer to malloc'ed value copy     */
}
  t_dict_node;

/***************************************************
 *                 Local Variables Section
 ***************************************************/

/** This macro for easier rewriting */
#define Dict_head (*(t_dict_node **)dict)

/**************************************************
 *  Function Prototype Section
 * Add prototypes for all local functions defined
 * in this file
 ***************************************************/
static t_dict_node *DictInternalFind(dict_hdl_t dict, const char * const key);
static void DictInternalFreeNode(t_dict_node ** const p);

/***************************************************
 *  Function Definition Section
 * Define all public and local functions from now on
 **************************************************/

/**************************************************
 * Function name : static t_dict_node *DictInternalFind(const char * const key)
 * Created by    : Toni Homedes i Saun
 * Date created  : 03-08-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Searches for given key and returns pointer to the whole node
 *
 *  \param[in]  key Pointer to the key to search for
 *   
 *  \return Pointer to the found node or NULL if not
 *
 **************************************************/
static t_dict_node *DictInternalFind(const dict_hdl_t dict, const char * const key)
{
  t_dict_node *p;
  
  /* run over list looking for key */
  for (p = Dict_head; p; p = p->next)  
    if (!strcmp(key, p->key))
      break;
  return p;
}

/**************************************************
 * Function name : static void DictInternalFreeNode(t_dict_node ** const p)
 * Created by    : Toni Homedes i Saun
 * Date created  : 03-08-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Frees passed node and related mallocs and relinks list
 *
 *  \param[in,out] p Pointer to the pointer holding node to delete
 *
 **************************************************/
static void DictInternalFreeNode(t_dict_node ** const p)
{
  if (*p)
  {
    t_dict_node *tmp = (*p)->next;
    free((*p)->key);
    free((*p)->value);
    free(*p);
    *p = tmp;
  }    
}

/**************************************************
 * Function name : dict_hdl_t DictNew(void)
 * Created by    : Toni Homedes i Saun
 * Date created  : 05-10-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Creates a new Dictionary
 *
 *  \return New dictionnary handle
 *
 **************************************************/
dict_hdl_t DictNew(void)
{
  t_dict_node * const *dict = malloc(sizeof (t_dict_node*));
  if (!dict)
  {
    perror("Can't get memory for dict handle");
    exit(EXIT_FAILURE);
  }
  Dict_head = NULL;
  return (dict_hdl_t)dict;
}

/**************************************************
 * Function name : static t_dict_node *DictInternalFind(const char * const key)
 * Created by    : Toni Homedes i Saun
 * Date created  : 03-08-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Searches for given key and returns pointer to the whole node
 *
 *  \param[in]  key Pointer to the key to search for
 *   
 *  \return Pointer to the found node or NULL if not
 *
 **************************************************/
void DictFree(dict_hdl_t dict)
{
  if (dict)
    DictDoEmpty(dict);

  free((void *)dict);
}

/**************************************************
 * Function name : void DictSet(const char * const key, const char * const 
 *                     value)
 * Created by    : Toni Homedes i Saun
 * Date created  : 03-08-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Sets given value in the dictionary
 *
 *  \param[in] key    Key to store
 *  \param[in] value  Value to store
 *
 **************************************************/
void DictSet(const dict_hdl_t dict, const char * const key, const char * const 
    value)
{
  t_dict_node *node = DictInternalFind(dict, key);

  if (node)
  {
    if (strcmp(node->value, value))
    {
      free(node->value);
      node->value = strdup(value);
    }
  }
  else
  {
    /* Inset new node in linked list */
    /* for this application we are assuming we'll never run out of memory */
    node = malloc(sizeof *node);
    node->key = strdup(key);
    node->value = strdup(value);
    node->next = Dict_head;
    Dict_head = node;
  }
}

/**************************************************
 * Function name : const char *DictGet(const char * const key)
 *                     value)
 * Created by    : Toni Homedes i Saun
 * Date created  : 03-08-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Gets a key's value from the dictionnary
 *
 *  \param[in]  key    Key to store
 *
 *  \returns    value if found or NULL if not
 *  
 **************************************************/
const char *DictGet(const dict_hdl_t dict, const char * const key)
{
  t_dict_node *node = DictInternalFind(dict, key);
  return node ? node->value : NULL;
}

/**************************************************
 * Function name : void DictDelete(const char * const key)
 * Created by    : Toni Homedes i Saun
 * Date created  : 03-08-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Deletes given value from the dictionary
 *
 *  \param[in] key    Key to delete
 *
 **************************************************/
void DictDelete(const dict_hdl_t dict, const char * const key)
{
  t_dict_node **p;
  
  /* run over list looking for key keeping a * to previous for relinking */
  for (p = &Dict_head; *p; p = &(*p)->next)  
    if (!strcmp(key, (*p)->key))
      break;
      
  DictInternalFreeNode(p);
}

/**************************************************
 * Function name : int DictIsEmpty(void)
 * Created by    : Toni Homedes i Saun
 * Date created  : 03-08-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Checks if the dictionary is empty
 *
 **************************************************/
int DictIsEmpty(const dict_hdl_t dict)
{
  return NULL == Dict_head;
}

/**************************************************
 * Function name : void DictDoEmpty(void)
 * Created by    : Toni Homedes i Saun
 * Date created  : 03-08-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Empties the dictionary
 *
 **************************************************/
void DictDoEmpty(const dict_hdl_t dict)
{
  while (Dict_head)
  {
    DictInternalFreeNode(&Dict_head);
  }
}

/**************************************************
 * Function name : void DictMap(void (*fn)(const char *key, const char *value))
 * Created by    : Toni Homedes i Saun
 * Date created  : 03-08-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Maps a function on all dictionary entries. Order is random.
 *
 *  \param[in]  fn    Fucntion to be mapped
 *  
 **************************************************/
void DictMap(const dict_hdl_t dict, void (*fn)(const char *key, const char *value))
{
  t_dict_node *p;
  for (p = Dict_head; p; p = p->next)
    fn(p->key, p->value);
}

/**************************************************
 * Function name : dict_iterator_t DictIteratorNew(dict_hdl_t dict)
 *                     value)
 * Created by    : Toni Homedes i Saun
 * Date created  : 06-10-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Create a new Dict Iterator
 *
 *  \param[in] dict   Dictionnary
 *
 **************************************************/
dict_iterator_t DictIteratorNew(dict_hdl_t dict)
{
  t_dict_node ** const iterator = malloc(sizeof (t_dict_node*));
  if (!iterator)
  {
    perror("Can't get memory for iterator handle");
    exit(EXIT_FAILURE);
  }
  *iterator = Dict_head;
  return (dict_iterator_t)iterator;
}

/**************************************************
 * Function name : void DictSet(const char * const key, const char * const 
 *                     value)
 * Created by    : Toni Homedes i Saun
 * Date created  : 06-10-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Destroys given iterator
 *
 *  \param[in] iterator Iterator to destroy
 *
 **************************************************/
void DictIteratorFree(dict_iterator_t iterator)
{
  free((void *)iterator);
}

/**************************************************
 * Function name : const char *DictIteratorKey(dict_iterator_t iterator)
 * Created by    : Toni Homedes i Saun
 * Date created  : 06-10-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Returns key for given iterator
 *
 *  \param[in] iterator Iterator to use
 *
 **************************************************/
const char *DictIteratorKey(dict_iterator_t iterator)
{
  t_dict_node **p = (t_dict_node **)iterator;
  
  return *p ? (*p)->key : NULL;
}

/**************************************************
 * Function name : void DictSet(const char * const key, const char * const 
 *                     value)
 * Created by    : Toni Homedes i Saun
 * Date created  : 06-10-2010
 * Notes         : Public function
 *                 Restrictions (pre-conditions)
 *                 Odd modes (post-conditions)
 **************************************************/
/** \brief Advances given iterator
 *
 *  \param[in] iterator Iterator to advance
 *
 **************************************************/
int DictIteratorAdvance(dict_iterator_t iterator)
{
  t_dict_node **p = (t_dict_node **)iterator;
  
  if (*p)
    *p = (*p)->next;
    
  return *p != NULL;
}

/*@}*/
