/*
 * Copyright 2003, Broadcom Corporation
 * All Rights Reserved.
 *
 * Low resolution timer interface Ecos specific implementation.
 *
 * $Id: ecos_timer.c 241182 2011-02-17 21:50:03Z $
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <cyg/hal/drv_api.h>
#include <sys/param.h>
#include <osl.h>
#include "bcmtimer.h"


/* forward declaration */
typedef struct ecos_timer_entry_s	*ecos_timer_entry_p;
typedef struct ecos_timer_list_s	*ecos_timer_list_p;

/* timer entry */
typedef struct ecos_timer_entry_s
{
	ecos_timer_entry_p	prev, next;
	ecos_timer_list_p	list;
	uint				timer;
	uint				tick;
	int					flags;
	void				(*func)(bcm_timer_id id, int data);
	int					data;
} ecos_timer_entry_t;

#define TIMER_FLAG_NONE		0x0000
#define TIMER_FLAG_DEFERRED	0x0001
#define TIMER_FLAG_FINISHED	0x0002
#define TIMER_FLAG_IN_USE	0x0004
#define TIMER_FLAG_ARMED	0x0008

/* timer list */
typedef struct ecos_timer_list_s
{
	ecos_timer_entry_t *entry;
	ecos_timer_entry_t *used;
	ecos_timer_entry_t *freed;
	int					entries;
	cyg_mutex_t			lock;
	int					flags;
} ecos_timer_list_t;

#define TIMER_LIST_FLAG_NONE	0x0000
#define TIMER_LIST_FLAG_INIT	0x0001
#define TIMER_LIST_FLAG_EXIT	0x0002

#ifdef TIMER_DEBUG
#include <stdio.h>
#define TIMERDBG(fmt, arg...)	printf("%s: "fmt"\n", __FUNCTION__, ##arg)
#else
#define TIMERDBG(fmt, arg...)
#endif /* if TIMER_DEBUG */

/* timer list lock */
#define TIMER_LIST_LOCK(list)	cyg_mutex_lock(&(list->lock))
#define TIMER_LIST_UNLOCK(list)	cyg_mutex_unlock(&(list->lock))
#define TIMER_FREE_LOCK_MECHANISM()

/* Interrupt disable */
#define INTLOCK() 0
#define INTUNLOCK(a)

extern int gettimeofday(struct timeval *tv, struct timezone *tz);

static int ecos_timer_count = 0;

static void
timer_func(int p1)
{
	ecos_timer_entry_t *entry = (ecos_timer_entry_t *)p1;
	ecos_timer_list_t *list = entry->list;

	entry->timer = 0;

	if (list == 0)
		return;

	if (list->flags & TIMER_LIST_FLAG_EXIT)
		return;

	entry->flags |= TIMER_FLAG_DEFERRED;
	entry->flags &= ~TIMER_FLAG_FINISHED;

	/* callback */
	entry->func((bcm_timer_id)entry, entry->data);

	entry->flags |= TIMER_FLAG_FINISHED;
	entry->flags &= ~TIMER_FLAG_DEFERRED;

	/* restart timeout */
	entry->timer = timeout((timeout_fun *)timer_func, entry, entry->tick);
}

static void
ecos_del_timer(ecos_timer_entry_t *entry)
{
	if (entry == NULL)
		return;

	if (entry->timer)
		untimeout((timeout_fun *)timer_func, entry);
	ecos_timer_count--;
	return;
}

static int
ecos_add_timer(ecos_timer_entry_t *entry, uint ms, int periodic)
{
	cyg_tick_count_t ostick;

	if (!entry)
		return 0;
	ostick = ms / 10;
	entry->tick = ostick;
	entry->timer = timeout((timeout_fun *)timer_func, entry, entry->tick);
	ecos_timer_count++;
	return 1;
}

/* alloc entry from top of list */
static int
get_entry(ecos_timer_entry_t **list, ecos_timer_entry_t **entry)
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
static int
put_entry(ecos_timer_entry_t **list, ecos_timer_entry_t *entry)
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
static int
remove_entry(ecos_timer_entry_t **list, ecos_timer_entry_t *entry)
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

/*
* Initialize internal resources used in the timer module. It must be called
* before any other timer function calls. The param 'timer_entries' is used
* to pre-allocate fixed number of timer entries.
*/
int
bcm_timer_module_init(int timer_entries, bcm_timer_module_id *module_id)
{
	int size = timer_entries*sizeof(ecos_timer_entry_t);
	ecos_timer_list_t *list;
	int i;

	TIMERDBG("entries %d", timer_entries);

	/* alloc fixed number of entries upfront */
	list = malloc(sizeof(ecos_timer_list_t)+size);
	if (list == NULL)
		goto exit0;

	cyg_mutex_init(&(list->lock));
	list->flags = TIMER_LIST_FLAG_NONE;
	list->entry = (ecos_timer_entry_t *)(list + 1);
	list->entries = timer_entries;
	TIMERDBG("entry %08x", list->entry);

	/* init the timer entries to form two list - freed and used */
	list->freed = NULL;
	list->used = NULL;
	memset(list->entry, 0, timer_entries*sizeof(ecos_timer_entry_t));

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
	ecos_timer_list_t *list = (ecos_timer_list_t *)module_id;
	ecos_timer_entry_t *entry;
	int key;

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
	TIMER_LIST_LOCK(list);

	key = INTLOCK();
	for (entry = list->used; entry != NULL; entry = entry->next)
	{
		ecos_del_timer(entry);
	}
	INTUNLOCK(key);

	TIMER_LIST_UNLOCK(list);

	/*
	* have to wait till all expired entries to have been handled
	*/
	for (entry = list->used; entry != NULL; entry = entry->next)
	{
		if ((entry->flags&TIMER_FLAG_DEFERRED) &&
		    !(entry->flags&TIMER_FLAG_FINISHED))
			break;
	}

	cyg_mutex_destroy(&(list->lock));
	/* now it should be safe to blindly free all the resources */
	TIMER_FREE_LOCK_MECHANISM();
	free(list);
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
	ecos_timer_list_t *list = (ecos_timer_list_t *)module_id;
	ecos_timer_entry_t *entry;
	int status = -1;

	TIMERDBG("list %08x", list);

	/* lock the timer list */
	TIMER_LIST_LOCK(list);

	/* check if timer is allowed */
	if (list->flags & TIMER_LIST_FLAG_EXIT)
		goto exit0;

	/* alloc an entry first */
	status = get_entry(&list->freed, &entry);
	if (status != 0)
		goto exit0;

	/* add the entry into used list */
	put_entry(&list->used, entry);

	entry->flags = TIMER_FLAG_IN_USE;
	entry->list = list;
	*timer_id = (bcm_timer_id)(void *)entry;

	TIMER_LIST_UNLOCK(list);
	TIMERDBG("entry %08x timer %08x", entry, entry->timer);
	return 0;

	/* error handling */
exit0:
	TIMER_LIST_UNLOCK(list);
	return status;
}

int
bcm_timer_delete(bcm_timer_id timer_id)
{
	ecos_timer_entry_t *entry = (ecos_timer_entry_t *)timer_id;
	ecos_timer_list_t *list = entry->list;
	int status;
	int key;

	TIMERDBG("entry %08x timer %08x", entry, entry->timer);

	/* make sure no interrupts can happen */
	key = INTLOCK();

	/* lock the timer list */
	TIMER_LIST_LOCK(list);

	/* remove the entry from the used list first */
	status = remove_entry(&list->used, entry);
	if (status != 0)
		goto exit0;

	/* delete the backend timer */
	ecos_del_timer(entry);

	/* free the entry back to freed list */
	put_entry(&list->freed, entry);

	entry->flags = TIMER_FLAG_NONE;
	entry->list = NULL;
	TIMER_LIST_UNLOCK(list);

	INTUNLOCK(key);

	TIMERDBG("done");
	return 0;

	/* error handling */
exit0:
	TIMER_LIST_UNLOCK(list);
	INTUNLOCK(key);
	return status;
}

int
bcm_timer_gettime(bcm_timer_id timer_id, struct itimerspec *timer_spec)
{
	return -1;

}

int
bcm_timer_settime(bcm_timer_id timer_id, const struct itimerspec *timer_spec)
{
	unsigned int ms;
	ecos_timer_entry_t *entry = (ecos_timer_entry_t *)timer_id;
	TIMERDBG("entry %08x timer %08x", entry, entry->timer);

	ms = (timer_spec->it_value.tv_sec*1000) +
		((timer_spec->it_value.tv_nsec/1000)/1000);

	return ecos_add_timer(entry, ms, 1) ? 0 : 1;
}

int
bcm_timer_connect(bcm_timer_id timer_id, bcm_timer_cb func, int data)
{
	ecos_timer_entry_t *entry = (ecos_timer_entry_t *)timer_id;

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
	ecos_timer_entry_t *entry = (ecos_timer_entry_t *)timer_id;

	TIMERDBG("entry %08x timer %08x", entry, entry->timer);

	ecos_del_timer(entry);
	entry->flags &= ~TIMER_FLAG_ARMED;
	return 0;
}

int
bcm_timer_change_expirytime(bcm_timer_id timer_id, const struct itimerspec *timer_spec)
{
	bcm_timer_cancel(timer_id);
	bcm_timer_settime(timer_id, timer_spec);
	return 1;
}

#define clockid_t	int
int
clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	struct timeval tv;
	int n;

	n = gettimeofday(&tv, NULL);
	TIMEVAL_TO_TIMESPEC(&tv, tp);

	return n;
}
