/* 
   Unix SMB/CIFS implementation.
   Manage connections_struct structures
   Copyright (C) Andrew Tridgell 1998
   Copyright (C) Alexander Bokovoy 2002
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

#ifdef MAX_USB_ACCESS
/* Obtain a binary semaphore¡¦s ID, allocating if necessary. */
int binary_semaphore_allocation (key_t key, int sem_flags)
{
    return semget (key, 1, sem_flags);
}
/* Deallocate a binary semaphore. All users must have finished their
use. Returns -1 on failure. */
int binary_semaphore_deallocate (int semid)
{
    union semun ignored_argument;
    return semctl (semid, 1, IPC_RMID, ignored_argument);
}

/* Initialize a binary semaphore with a value of 1. */
int binary_semaphore_initialize (int semid)
{
    union semun argument;
    unsigned short values[1];
    values[0] = 1;
    argument.array = values;
    return semctl (semid, 0, SETALL, argument);
}
/* Wait on a binary semaphore. Block until the semaphore value is positive, then
decrement it by 1. */
int binary_semaphore_wait (int semid)
{
    struct sembuf operations[1];
    /* Use the first (and only) semaphore. */
    operations[0].sem_num = 0;
    /* Decrement by 1. */
    operations[0].sem_op = -1;
    /* Permit undo¡¦ing. */
    operations[0].sem_flg = SEM_UNDO;
    return semop (semid, operations, 1);
}

/* Post to a binary semaphore: increment its value by 1.
This returns immediately. */
int binary_semaphore_post (int semid)
{
    struct sembuf operations[1];
    /* Use the first (and only) semaphore. */
    operations[0].sem_num = 0;
    /* Increment by 1. */
    operations[0].sem_op = 1;
    /* Permit undo¡¦ing. */
    operations[0].sem_flg = SEM_UNDO;
    return semop (semid, operations, 1);
}
#endif //End of MAX_USB_ACCESS


/* The connections bitmap is expanded in increments of BITMAP_BLOCK_SZ. The
 * maximum size of the bitmap is the largest positive integer, but you will hit
 * the "max connections" limit, looong before that.
 */
#define BITMAP_BLOCK_SZ 128

static connection_struct *Connections;

/* number of open connections */
static struct bitmap *bmap;
static int num_open;

/****************************************************************************
init the conn structures
****************************************************************************/
void conn_init(void)
{
	bmap = bitmap_allocate(BITMAP_BLOCK_SZ);
}

/****************************************************************************
return the number of open connections
****************************************************************************/
int conn_num_open(void)
{
	return num_open;
}


/****************************************************************************
check if a snum is in use
****************************************************************************/
BOOL conn_snum_used(int snum)
{
	connection_struct *conn;
	for (conn=Connections;conn;conn=conn->next) {
		if (conn->service == snum) {
			return(True);
		}
	}
	return(False);
}


/****************************************************************************
find a conn given a cnum
****************************************************************************/
connection_struct *conn_find(unsigned cnum)
{
	int count=0;
	connection_struct *conn;

	for (conn=Connections;conn;conn=conn->next,count++) {
		if (conn->cnum == cnum) {
			if (count > 10) {
				DLIST_PROMOTE(Connections, conn);
			}
			return conn;
		}
	}

	return NULL;
}


/****************************************************************************
  find first available connection slot, starting from a random position.
The randomisation stops problems with the server dieing and clients
thinking the server is still available.
****************************************************************************/
connection_struct *conn_new(void)
{
	TALLOC_CTX *mem_ctx;
	connection_struct *conn;
	int i;
    int find_offset = 1;
#if 0
    extern CON_STATISTIC *con_st;

    if(con_st != NULL){
        dbgtext("->total con num: %d, %s\n", con_st->num, __FUNCTION__);
        binary_semaphore_wait (con_st->sem_id);
        if(con_st->num >= MAX_CON_NUM){
            binary_semaphore_post (con_st->sem_id); 
            return NULL;
        }
        binary_semaphore_post (con_st->sem_id);
    }
#endif

find_again:
	i = bitmap_find(bmap, find_offset);
	
	if (i == -1) {
                /* Expand the connections bitmap. */
                int             oldsz = bmap->n;
                int             newsz = bmap->n + BITMAP_BLOCK_SZ;
                struct bitmap * nbmap;

                if (newsz <= 0) {
                        /* Integer wrap. */
		        DEBUG(0,("ERROR! Out of connection structures\n"));
                        return NULL;
                }

		DEBUG(4,("resizing connections bitmap from %d to %d\n",
                        oldsz, newsz));

                nbmap = bitmap_allocate(newsz);

                bitmap_copy(nbmap, bmap);
                bitmap_free(bmap);

                bmap = nbmap;
                find_offset = oldsz; /* Start next search in the new portion. */

                goto find_again;
	}

	if ((mem_ctx=talloc_init("connection_struct"))==NULL) {
		DEBUG(0,("talloc_init(connection_struct) failed!\n"));

		return NULL;
	}

	if ((conn=TALLOC_ZERO_P(mem_ctx, connection_struct))==NULL) {
		DEBUG(0,("talloc_zero() failed!\n"));

		return NULL;
	}
	conn->mem_ctx = mem_ctx;
	conn->cnum = i;

	bitmap_set(bmap, i);

	num_open++;

	string_set(&conn->user,"");
	string_set(&conn->dirpath,"");
	string_set(&conn->connectpath,"");
	string_set(&conn->origpath,"");
	
	DLIST_ADD(Connections, conn);

	return conn;
}

/****************************************************************************
close all conn structures
****************************************************************************/
void conn_close_all(void)
{
	connection_struct *conn, *next;
	for (conn=Connections;conn;conn=next) {
		next=conn->next;
		set_current_service(conn, 0, True);
		close_cnum(conn, conn->vuid);
	}
}

/****************************************************************************
 Idle inactive connections.
****************************************************************************/

BOOL conn_idle_all(time_t t, int deadtime)
{
	pipes_struct *plist = NULL;
	BOOL allidle = True;
	connection_struct *conn, *next;

	for (conn=Connections;conn;conn=next) {
		next=conn->next;
		/* close dirptrs on connections that are idle */
		if ((t-conn->lastused) > DPTR_IDLE_TIMEOUT)
			dptr_idlecnum(conn);

		if (conn->num_files_open > 0 || 
		    (t-conn->lastused)<deadtime)
			allidle = False;
	}

	/*
	 * Check all pipes for any open handles. We cannot
	 * idle with a handle open.
	 */

	for (plist = get_first_internal_pipe(); plist; plist = get_next_internal_pipe(plist))
		if (plist->pipe_handles && plist->pipe_handles->count)
			allidle = False;
	
	return allidle;
}

/****************************************************************************
 Clear a vuid out of the validity cache, and as the 'owner' of a connection.
****************************************************************************/

void conn_clear_vuid_cache(uint16 vuid)
{
	connection_struct *conn;
	unsigned int i;

	for (conn=Connections;conn;conn=conn->next) {
		if (conn->vuid == vuid) {
			conn->vuid = UID_FIELD_INVALID;
		}

		for (i=0;i<conn->vuid_cache.entries && i< VUID_CACHE_SIZE;i++) {
			if (conn->vuid_cache.array[i].vuid == vuid) {
				struct vuid_cache_entry *ent = &conn->vuid_cache.array[i];
				ent->vuid = UID_FIELD_INVALID;
				ent->read_only = False;
				ent->admin_user = False;
			}
		}
	}
}

/****************************************************************************
 Free a conn structure.
****************************************************************************/

void conn_free(connection_struct *conn)
{
 	vfs_handle_struct *handle = NULL, *thandle = NULL;
 	TALLOC_CTX *mem_ctx = NULL;

	/* Free vfs_connection_struct */
	handle = conn->vfs_handles;
	while(handle) {
		DLIST_REMOVE(conn->vfs_handles, handle);
		thandle = handle->next;
		if (handle->free_data)
			handle->free_data(&handle->data);
		handle = thandle;
	}

	DLIST_REMOVE(Connections, conn);

	if (conn->ngroups && conn->groups) {
		SAFE_FREE(conn->groups);
		conn->ngroups = 0;
	}

	if (conn->nt_user_token) {
		delete_nt_token(&(conn->nt_user_token));
	}

	free_namearray(conn->veto_list);
	free_namearray(conn->hide_list);
	free_namearray(conn->veto_oplock_list);
	
	string_free(&conn->user);
	string_free(&conn->dirpath);
	string_free(&conn->connectpath);
	string_free(&conn->origpath);

	bitmap_clear(bmap, conn->cnum);
	num_open--;

	mem_ctx = conn->mem_ctx;
	ZERO_STRUCTP(conn);
	talloc_destroy(mem_ctx);
}


/****************************************************************************
receive a smbcontrol message to forcibly unmount a share
the message contains just a share name and all instances of that
share are unmounted
the special sharename '*' forces unmount of all shares
****************************************************************************/
void msg_force_tdis(int msg_type, pid_t pid, void *buf, size_t len)
{
	connection_struct *conn, *next;
	fstring sharename;

	fstrcpy(sharename, buf);

	if (strcmp(sharename, "*") == 0) {
		DEBUG(1,("Forcing close of all shares\n"));
		conn_close_all();
		return;
	}

	for (conn=Connections;conn;conn=next) {
		next=conn->next;
		if (strequal(lp_servicename(conn->service), sharename)) {
			DEBUG(1,("Forcing close of share %s cnum=%d\n",
				 sharename, conn->cnum));
			close_cnum(conn, (uint16)-1);
		}
	}
}
