/*	$Id: hash.c 241182 2011-02-17 21:50:03Z $	*/

/*
 * Copyright (C) International Business Machines  Corp., 2003
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Author: Elizabeth Kon, beth@us.ibm.com */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>

#include "hash.h"

#ifdef	__GNUC__
extern void dprintf(int, const char *, ...)
	__attribute__ ((__format__(__printf__, 2, 3)));
#else
extern void dprintf __P((int, const char *, ...));
#endif

struct hash_table * hash_table_create (
	unsigned int hash_size,
	unsigned int (*hash_function)(const void *hash_key),
	void * (*find_hashkey)(const void *data),
	int (*compare_hashkey)(const void *data, const void *hashkey))
{
	int i;
	struct hash_table *hash_tbl;
	hash_tbl = malloc(sizeof(struct hash_table));
	if (!hash_tbl) {
		dprintf(LOG_ERR, "Couldn't allocate hash table");
		return NULL;
	}
	hash_tbl->hash_list = malloc(sizeof(struct hashlist_element *)*hash_size);
        for (i=0; i<hash_size; i++) {
		hash_tbl->hash_list[i] = NULL;
	}
	hash_tbl->hash_count = 0;
	hash_tbl->hash_size = hash_size;
	hash_tbl->hash_function = hash_function;
	hash_tbl->find_hashkey = find_hashkey;
	hash_tbl->compare_hashkey = compare_hashkey;
	return hash_tbl;
}

int  hash_add(struct hash_table *hash_tbl, const void *key, void *data)
{
	int index;
	struct hashlist_element *element;
	element = (struct hashlist_element *)malloc(sizeof(struct hashlist_element));
	if(!element){
		dprintf(LOG_ERR, "Could not malloc hashlist_element");
		return (-1);
	}
	if (hash_full(hash_tbl)) {
	       grow_hash(hash_tbl);
	}
	index = hash_tbl->hash_function(key) % hash_tbl->hash_size;
	if (hash_search(hash_tbl, key)) {
		dprintf(LOG_DEBUG, "hash_add: duplicated item");
		return HASH_COLLISION;
	}
	element->next = hash_tbl->hash_list[index];
	hash_tbl->hash_list[index] = element;
	element->data = data;
	hash_tbl->hash_count++;
	return 0;
}            

int hash_delete(struct hash_table *hash_tbl, const void *key)
{
	int index;
	struct hashlist_element *element, *prev_element = NULL;
	index = hash_tbl->hash_function(key)  % hash_tbl->hash_size;
	element = hash_tbl->hash_list[index];
	while (element) {
		if (MATCH == hash_tbl->compare_hashkey(element->data, key)) {
			if(prev_element){
				prev_element->next = element->next;	
			} else {
				hash_tbl->hash_list[index] = element->next;
			}
			element->next = NULL;
	  		free(element);		
	   		hash_tbl->hash_count--;
			return 0;
		}
		prev_element = element;
		element = element->next;
	}
	return HASH_ITEM_NOT_FOUND;
}            

void * hash_search(struct hash_table *hash_tbl, const void *key) 
{
	int index;
	struct hashlist_element *element;
	index = hash_tbl->hash_function(key)  % hash_tbl->hash_size;
	element = hash_tbl->hash_list[index];
	while (element) {
		if (MATCH == hash_tbl->compare_hashkey(element->data, key)) {
			return element->data; 
		}		
		element = element->next;
	}
	return NULL;
}

int hash_full(struct hash_table *hash_tbl) {
	int rc = 0;
	if((hash_tbl->hash_count)*100/(hash_tbl->hash_size) > 90) rc = 1;
	return rc;
}

int grow_hash(struct hash_table *hash_tbl) {
	int i, hash_size, index;
        struct hashlist_element *element, *oldnext;
	struct hash_table *new_table;
	void *key;
	hash_size = 2*hash_tbl->hash_size;
	new_table = hash_table_create(hash_size, hash_tbl->hash_function, 
			hash_tbl->find_hashkey, hash_tbl->compare_hashkey);
	if (!new_table) {
		dprintf(LOG_ERR, "couldn't grow hash table");
		return (-1);
	}
        for (i = 0; i < hash_tbl->hash_size; i++) {
                element = hash_tbl->hash_list[i];
                while (element) {
		  	key = hash_tbl->find_hashkey(element->data);
			index = new_table->hash_function(key)  % hash_size;
			oldnext = element->next;
			element->next = new_table->hash_list[index];
			new_table->hash_list[index] = element;
			new_table->hash_count++;
			element = oldnext;
                }
        }
	free(hash_tbl->hash_list);
	hash_tbl->hash_count = new_table->hash_count;
	hash_tbl->hash_size = hash_size;
	hash_tbl->hash_list = new_table->hash_list;
	free(new_table);
	return 0;
}
