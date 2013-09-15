/*	$Id: hash.h 241182 2011-02-17 21:50:03Z $	*/

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

#ifndef DHCPV6_HASH_H
#define DHCPV6_HASH_H

#define DEFAULT_HASH_SIZE 4096-1

#define MATCH 0
#define MISCOMPARE 1
#define HASH_COLLISION        2
#define HASH_ITEM_NOT_FOUND   3

struct hashlist_element {
        struct hashlist_element *next;
        void *data;
};

struct hash_table {
        unsigned int hash_count;
        unsigned int hash_size;
        struct hashlist_element **hash_list;
        unsigned int (*hash_function)(const void *hash_key);
	void * (*find_hashkey)(const void *data);
        int (*compare_hashkey)(const void *data, const void *key);
};

 
extern int init_hashes(void);
extern struct hash_table * hash_table_create(unsigned int hash_size,
	unsigned int (*hash_function)(const void *hash_key),
	void * (*find_hashkey)(const void *data),
	int (*compare_hashkey)(const void *data, const void *hashkey));
extern int  hash_add(struct hash_table *table, const void *key, void *data);
extern int hash_delete(struct hash_table *table, const void *key);
extern void * hash_search(struct hash_table *table, const void *key);
extern int hash_full(struct hash_table *table);
extern int grow_hash(struct hash_table *table);
#endif
