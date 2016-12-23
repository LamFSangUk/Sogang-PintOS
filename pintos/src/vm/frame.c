#include "vm/frame.h"
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "threads/interrupt.h"
#include "threads/vaddr.h"

static struct list_elem *get_next_LRU_clock (void);

struct list list_LRU;
struct lock list_LRU_lock;
struct list_elem *LRU_clock;

void
list_LRU_init (void)
{
	LRU_clock = NULL;
	lock_init (&list_LRU_lock);
	list_init (&list_LRU);
}

void
add_page_to_list_LRU (struct page *page)
{
	lock_acquire (&list_LRU_lock);  

	ASSERT(page->thread);
	ASSERT(page->thread->magic==0xcd6abf4b);

	list_push_back (&list_LRU, &page->LRU);

	lock_release (&list_LRU_lock);
}

struct page *
find_page_from_list_LRU (void *kaddr)
{
	ASSERT(lock_held_by_current_thread(&list_LRU_lock));
	ASSERT (pg_ofs (kaddr) == 0);

	struct list_elem *e;

	for (e = list_begin (&list_LRU);
			e != list_end (&list_LRU);
			e = list_next (e)){

		struct page *page = list_entry (e, struct page, LRU);
		ASSERT(page);
		if (page->kaddr == kaddr)	
			return page; 
	}

	return NULL;
}

void
del_page_from_list_LRU (struct page *page)
{
	ASSERT (lock_held_by_current_thread (&list_LRU_lock));
	ASSERT (page);
	if (LRU_clock == &page->LRU)
	{
		LRU_clock = list_remove (LRU_clock);
	}
	else
	{
		list_remove (&page->LRU);
	}
}

static struct list_elem *
get_next_LRU_clock (void)
{
	ASSERT (lock_held_by_current_thread (&list_LRU_lock));
	if (LRU_clock == NULL || LRU_clock == list_end (&list_LRU)){
		if (list_empty (&list_LRU))
			return NULL;
		else{
			LRU_clock=list_begin(&list_LRU);
			return LRU_clock;
		}
	}
	
	LRU_clock = list_next (LRU_clock);
	if (LRU_clock == list_end (&list_LRU))
		return get_next_LRU_clock ();

	return LRU_clock;
}

struct page *
get_victim (void)
{
	struct page *page;
	struct list_elem *e;

	ASSERT (lock_held_by_current_thread (&list_LRU_lock));

	e = get_next_LRU_clock ();
	ASSERT (e != NULL);
	page = list_entry (e, struct page, LRU);
	ASSERT (page);
	ASSERT (page->thread);
	ASSERT (page->thread->magic == 0xcd6abf4b);
	ASSERT (page->pge);

	while (pagedir_is_accessed (page->thread->pagedir, page->pge->vaddr))
	{
		pagedir_set_accessed (page->thread->pagedir, page->pge->vaddr, false);
		e = get_next_LRU_clock ();
		ASSERT (e != NULL);
		page = list_entry (e, struct page, LRU);
		ASSERT (page);
		ASSERT (page->thread);
		ASSERT (page->thread->magic == 0xcd6abf4b);
		ASSERT (page->pge);
	}

	return page;
}
