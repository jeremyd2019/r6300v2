/*
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
 * Low resolution timer interface. Timer handlers may be called
 * in a deferred manner in a different task context after the
 * timer expires or in the task context from which the timer
 * was created, depending on the implementation.
 *
 * $Id: rte_timers.c 241182 2011-02-17 21:50:03Z $
 */

#include <typedefs.h>
#include <bcmtimer.h>
#include <osl.h>
#include <hndrte.h>

#include <stdlib.h>


/* Forward declarations */
typedef struct bcm_timer_s *bcm_timer_p;
typedef struct bcm_timer_list_s *bcm_timer_list_p;

/* timer entry */
typedef struct bcm_timer_s
{
	bcm_timer_p prev, next;
	bcm_timer_list_p list;
	hndrte_timer_t *timer;
	int flags;
	void (*func)(bcm_timer_id id, int data);
	int data;
} bcm_timer_t;

#define TIMER_FLAG_NONE	0x0000
#define TIMER_FLAG_DEFERRED	0x0001
#define TIMER_FLAG_FINISHED	0x0002
#define TIMER_FLAG_IN_USE	0x0004
#define TIMER_FLAG_ARMED	0x0008


typedef struct bcm_timer_list_s
{
	bcm_timer_t *entry;
	bcm_timer_t *used;
	bcm_timer_t *freed;
	int entries;
	int flags;
} bcm_timer_list_t;

#define TIMER_LIST_FLAG_NONE	0x0000
#define TIMER_LIST_FLAG_INIT	0x0001
#define TIMER_LIST_FLAG_EXIT	0x0002

#define TIMER_DEBUG 0
#if TIMER_DEBUG
#include <stdio.h>
#define TIMERDBG(fmt, arg...)	printf("%s: "fmt"\n", __FUNCTION__, ##arg)
#else
#define TIMERDBG(fmt, arg...)
#endif /* if TIMER_DEBUG */

/* timer list lock - currently no need for this in RTE, no mechanism either */
#define TIMER_LIST_LOCK(list) 0
#define TIMER_LIST_UNLOCK(list)
#define TIMER_FREE_LOCK_MECHANISM()
/* Interrupt disable */
#define INT_LOCK() 0
#define INT_UNLOCK(a)

/* alloc entry from top of list */
static int get_entry(bcm_timer_t **list, bcm_timer_t **entry)
{
	/* take an entry from top of the list */
	TIMERDBG("list = %08x", *list);
	*entry = *list;
	if (*entry == NULL)
		return -1;
	*list = (*entry)->next;
	if (*list != NULL)
		(*list)->prev = NULL;
	TIMERDBG("new list = %08x", *list);
	(*entry)->next = NULL;
	(*entry)->prev = NULL;
	TIMERDBG("entry = %08x", *entry);
	return 0;
}

/* add entry into top of list */
static int put_entry(bcm_timer_t **list, bcm_timer_t *entry)
{
	/* add the entry into top of the list */
	TIMERDBG("list = %08x", *list);
	TIMERDBG("entry = %08x", entry);
	entry->next = *list;
	entry->prev = NULL;
	if (*list != NULL)
		(*list)->prev = entry;
	*list = entry;
	TIMERDBG("new list = %08x", *list);
	return 0;
}


/* remove entry from list */
static int remove_entry(bcm_timer_t **list, bcm_timer_t *entry)
{
	/* remove the entry from the list */
	TIMERDBG("list = %08x", *list);
	TIMERDBG("entry = %08x", entry);
	if (entry->prev != NULL)
		entry->prev->next = entry->next;
	if (entry->next != NULL)
		entry->next->prev = entry->prev;
	if (*list == entry)
		*list = entry->next;
	entry->next = NULL;
	entry->prev = NULL;
	TIMERDBG("new list = %08x", *list);
	return 0;
}

/* internal deferred timer callback function */
static void exc_func(hndrte_task_t *timer)
{
	bcm_timer_t *entry = (bcm_timer_t*)timer->data;
	TIMERDBG("entry %08x timer %08x", entry, entry->timer);
	/* make the callback first */
	TIMERDBG("func %08x data %08x", entry->func, entry->data);
	entry->func((bcm_timer_id)entry, entry->data);
	entry->flags |= TIMER_FLAG_FINISHED;
	entry->flags &= ~TIMER_FLAG_DEFERRED;
	TIMERDBG("done");
	hndrte_free_timer(timer);
	return;
}

/* internal timer callback function */
static void timer_hdlr(hndrte_timer_t *timer)
{
	bcm_timer_t *entry = (bcm_timer_t *)timer->data;
	bcm_timer_list_t *list = entry->list;
	TIMERDBG("entry %08x timer %08x", entry, entry->timer);
	if (list->flags&TIMER_LIST_FLAG_EXIT)
		return;
	entry->flags |= TIMER_FLAG_DEFERRED;
	entry->flags &= ~TIMER_FLAG_FINISHED;
	/* defer the real callback to a task context */
	TIMERDBG("job %08x data %08x", exc_func, entry);
	hndrte_schedule_work(NULL, entry, exc_func, 0);
	TIMERDBG("done");
	return;
}

/*
* Initialize internal resources used in the timer module. It must be called
* before any other timer function calls. The param 'timer_entries' is used
* to pre-allocate fixed number of timer entries.
*/
int
bcm_timer_module_init(int timer_entries, bcm_timer_module_id *module_id)
{
	int size = timer_entries * sizeof(bcm_timer_t);
	bcm_timer_list_t *list;
	int i;
	TIMERDBG("entries %d", timer_entries);
	/* alloc fixed number of entries upfront */
	list = hndrte_malloc(sizeof(bcm_timer_list_t)+size);
	if (list == NULL)
		goto exit0;
	list->flags = TIMER_LIST_FLAG_NONE;
	list->entry = (bcm_timer_t *)(list + 1);
	list->entries = timer_entries;
	TIMERDBG("entry %08x", list->entry);
	/* init the timer entries to form two list - freed and used */
	list->freed = NULL;
	list->used = NULL;
	memset(list->entry, 0, timer_entries*sizeof(bcm_timer_t));
	for (i = 0; i < timer_entries; i ++)
	{
		put_entry(&list->freed, &list->entry[i]);
	}
	list->flags = TIMER_LIST_FLAG_INIT;
	*module_id = (bcm_timer_module_id)list;
	TIMERDBG("list %08x freed %08x used %08x", list, list->freed, list->used);
	return 0;
exit0:
	return -1;
}

/*
* Cleanup internal resources used by this timer module. It deletes all
* pending timer entries from the backend timer system as well.
*/
int
bcm_timer_module_cleanup(bcm_timer_module_id module_id)
{
	bcm_timer_list_t *list = (bcm_timer_list_t *)module_id;
	bcm_timer_t *entry;
	int status;
	int key;

	ASSERT(0); /* This hasn't been tested */

	TIMERDBG("list %08x", list);
	/*
	* do nothing if the list has not been initialized
	*/
	if (!(list->flags&TIMER_LIST_FLAG_INIT))
		return -1;
	/*
	* mark the big bang flag here so that no more callbacks
	* shall be scheduled or called from this point on...
	*/
	list->flags |= TIMER_LIST_FLAG_EXIT;
	/*
	* remove all backend timers here so that no timer expires after here.
	*/
	status = TIMER_LIST_LOCK(list);
	key = INT_LOCK();
	if (status != 0)
		return status;
	for (entry = list->used; entry != NULL; entry = entry->next)
	{
		hndrte_free_timer(entry->timer);
	}

	TIMER_LIST_UNLOCK(list);
	INT_UNLOCK(key);
	/*
	* have to wait till all expired entries to have been handled
	*/
	for (entry = list->used; entry != NULL; entry = entry->next)
	{
		if ((entry->flags&TIMER_FLAG_DEFERRED) &&
		    !(entry->flags&TIMER_FLAG_FINISHED))
			break;
	}

	/* now it should be safe to blindly free all the resources */
	TIMER_FREE_LOCK_MECHANISM();
	hndrte_free(list);
	TIMERDBG("done");
	return 0;
}

int
bcm_timer_module_enable(bcm_timer_module_id module_id, int enable)
{
	return 0;
}

int
bcm_timer_create(bcm_timer_module_id module_id, bcm_timer_id *timer_id)
{
	bcm_timer_list_t *list = (bcm_timer_list_t *)module_id;
	bcm_timer_t *entry;
	int status;
	TIMERDBG("list %08x", list);
	/* lock the timer list */
	status = TIMER_LIST_LOCK(list);
	if (status != 0)
		goto exit0;
	/* check if timer is allowed */
	if (list->flags & TIMER_LIST_FLAG_EXIT)
		goto exit1;
	/* alloc an entry first */
	status = get_entry(&list->freed, &entry);
	if (status != 0)
		goto exit1;
	/* create backend timer */
	entry->timer = hndrte_init_timer(NULL, entry, timer_hdlr, NULL);
	if (!entry->timer) {
		TIMERDBG("bcm_timer_create couldn't allocate timer");
		goto exit2;
	}
	/* add the entry into used list */
	status = put_entry(&list->used, entry);
	if (status != 0)
		goto exit3;
	entry->flags = TIMER_FLAG_IN_USE;
	entry->list = list;
	*timer_id = (bcm_timer_id)(void *)entry;
	TIMER_LIST_UNLOCK(list);
	TIMERDBG("entry %08x timer %08x", entry, entry->timer);
	return 0;
	/* error handling */
exit3:
	hndrte_del_timer(entry->timer);
exit2:
	put_entry(&list->freed, entry);
exit1:
	TIMER_LIST_UNLOCK(list);
exit0:
	return status;
}

int
bcm_timer_delete(bcm_timer_id timer_id)
{
	bcm_timer_t *entry = (bcm_timer_t *)timer_id;
	bcm_timer_list_t *list = entry->list;
	int status;
	int key;
	TIMERDBG("entry %08x timer %08x", entry, entry->timer);
	/* make sure no interrupts can happen */
	key = INT_LOCK();
	/* lock the timer list */
	status = TIMER_LIST_LOCK(list);
	if (status != 0)
		goto exit0;
	/* remove the entry from the used list first */
	status = remove_entry(&list->used, entry);
	if (status != 0)
		goto exit1;
	/* delete the backend timer */
	status = hndrte_del_timer(entry->timer) ? 0 : 1;
	if (status != 0)
		goto exit2;
	hndrte_free_timer(entry->timer);

	/* free the entry back to freed list */
	status = put_entry(&list->freed, entry);
	if (status != 0)
		goto exit3;
	entry->flags = TIMER_FLAG_NONE;
	entry->list = NULL;
	TIMER_LIST_UNLOCK(list);
	INT_UNLOCK(key);
	TIMERDBG("done");
	return 0;
	/* error handling */
exit3:
	TIMERDBG("put_entry failed");
exit2:
	TIMERDBG("timer_delete failed");
exit1:
	TIMER_LIST_UNLOCK(list);
exit0:
	INT_UNLOCK(key);
	return status;
}

int
bcm_timer_gettime(bcm_timer_id timer_id, struct itimerspec *timerspec)
{
	return -1;
}

int
bcm_timer_settime(bcm_timer_id timer_id, const struct itimerspec *timerspec)
{
	unsigned int ms;
	bcm_timer_t *entry = (bcm_timer_t *)timer_id;
	TIMERDBG("entry %08x timer %08x", entry, entry->timer);

	/*	ASSERT(timerspec->it_interval.tv_sec != timerspec->it_value.tv_sec); */
	ms = (timerspec->it_value.tv_sec*1000) +
		((timerspec->it_value.tv_nsec/1000)/1000);

	return hndrte_add_timer(entry->timer, ms, 1) ? 0 : 1;
}

int
bcm_timer_connect(bcm_timer_id timer_id, bcm_timer_cb func, int data)
{
	bcm_timer_t *entry = (bcm_timer_t *)timer_id;

	entry->func = func;
	entry->data = data;
	TIMERDBG("entry %08x timer %08x func %08x data %08x",
		entry, entry->timer, entry->func, entry->data);
	entry->flags |= TIMER_FLAG_ARMED;
	return 0;
}

int
bcm_timer_cancel(bcm_timer_id timer_id)
{
	bcm_timer_t *entry = (bcm_timer_t *)timer_id;

	TIMERDBG("entry %08x timer %08x", entry, entry->timer);
	if (!hndrte_del_timer(entry->timer)) {
		TIMERDBG("bcm_timer_cancel couldn't delete entry");
		goto exit0;
	}
	entry->flags &= ~TIMER_FLAG_ARMED;
	return 0;
	/* error handling */
exit0:
	return 1;
}

int
bcm_timer_change_expirytime(bcm_timer_id timer_id,
                            const struct itimerspec *timer_spec)
{
	bcm_timer_cancel(timer_id);
	bcm_timer_settime(timer_id, timer_spec);
	return 1;
}
