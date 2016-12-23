#include "page.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"
#include "threads/interrupt.h"

static unsigned spt_hash_func (const struct hash_elem *, void * UNUSED);
static bool spt_less_func (const struct hash_elem *, const struct hash_elem *, void * UNUSED);
static void spt_destroy_func (struct hash_elem *, void * UNUSED);


void spt_init (struct hash *spt)
{
	ASSERT (spt != NULL);
	hash_init (spt, spt_hash_func, spt_less_func, NULL);
}

void spt_destroy (struct hash *spt)
{
	ASSERT (spt != NULL);
	hash_destroy (spt, spt_destroy_func);
}

static unsigned
spt_hash_func (const struct hash_elem *e, void *aux UNUSED)
{
	ASSERT (e != NULL);
	return hash_int (hash_entry (e, struct page_entry, elem)->vaddr);
}

static bool
spt_less_func (const struct hash_elem *a,const struct hash_elem *b, void *aux UNUSED)
{
	ASSERT (a != NULL);
	ASSERT (b != NULL);
	return hash_entry (a, struct page_entry, elem)->vaddr < hash_entry (b, struct page_entry, elem)->vaddr;
}

static void
spt_destroy_func (struct hash_elem *e, void *aux UNUSED)
{
	ASSERT (e != NULL);
	struct page_entry *pge = hash_entry (e, struct page_entry, elem);
	swap_clear (pge->swap_slot);

	free_page_vaddr (pge->vaddr);
	free (pge);
}

struct page_entry *
get_pge (void *vaddr)
{
	struct hash *spt;
	struct page_entry pge;
	struct hash_elem *elem;

	spt = &thread_current ()->sup_page_tab;

	pge.vaddr = pg_round_down (vaddr);
	ASSERT (pg_ofs (pge.vaddr) == 0);
	elem = hash_find (spt, &pge.elem);
	if(elem){
		return hash_entry(elem, struct page_entry, elem);
	}
	else return NULL;
}

bool
insert_pge (struct hash *spt, struct page_entry *pge)
{
	ASSERT (spt != NULL);
	ASSERT (pge != NULL);
	ASSERT (pg_ofs (pge->vaddr) == 0);
	return hash_insert (spt, &pge->elem) == NULL;
}

bool
load_file (void *kaddr, struct page_entry *pge)
{
	ASSERT (kaddr != NULL);
	ASSERT (pge != NULL);
	ASSERT (pge->type == VM_BIN);

	if (file_read_at (pge->file, kaddr, pge->read_bytes, pge->offset) != (int)pge->read_bytes)
	{
		return false;
	}
	memset (kaddr + pge->read_bytes, 0, pge->zero_bytes);
	return true;
}

static void
replacement_mem (void)
{
	lock_acquire (&list_LRU_lock);
	
	struct page *vict_page = get_victim ();

	ASSERT (vict_page != NULL);
	ASSERT (vict_page->thread != NULL);
	ASSERT (vict_page->thread->magic == 0xcd6abf4b);
	ASSERT (vict_page->pge != NULL);

	bool dirty = pagedir_is_dirty (vict_page->thread->pagedir, vict_page->pge->vaddr);
	
	switch (vict_page->pge->type)
	{
		case VM_BIN:
			if (dirty)
			{
				vict_page->pge->swap_slot = swap_out (vict_page->kaddr);
				vict_page->pge->type = VM_ANON;
			}
			break;
		case VM_ANON:
			vict_page->pge->swap_slot = swap_out (vict_page->kaddr);
			break;
		default:
			NOT_REACHED ();
	}
	vict_page->pge->is_loaded = false;
	__free_page (vict_page);
	lock_release (&list_LRU_lock);
}

struct page *
alloc_page (enum palloc_flags flags)
{
	struct page *page;
	page = (struct page *)malloc (sizeof (struct page));
	if (page == NULL)
		return NULL;
	memset (page, 0, sizeof (struct page));
	page->thread = thread_current ();

	ASSERT (page->thread);
  ASSERT (page->thread->magic == 0xcd6abf4b);

	page->kaddr = palloc_get_page (flags);
	while (page->kaddr == NULL)
	{
		replacement_mem ();
		page->kaddr = palloc_get_page (flags);
	}

	return page;
}

extern struct list list_LRU;
extern struct list_elem *LRU_clock;

void
free_page_kaddr (void *kaddr)
{
	lock_acquire (&list_LRU_lock);

	struct page *page = find_page_from_list_LRU (kaddr);
	
	if(page)
		__free_page(page);

	lock_release(&list_LRU_lock);

}

void
free_page_vaddr (void *vaddr)
{
	free_page_kaddr (pagedir_get_page (thread_current ()->pagedir, vaddr));
}

void
__free_page (struct page *page)
{
	ASSERT (lock_held_by_current_thread (&list_LRU_lock));

	ASSERT (page != NULL);
	ASSERT (page->thread != NULL);
	ASSERT (page->thread->magic == 0xcd6abf4b);
	ASSERT (page->pge != NULL);

	pagedir_clear_page (page->thread->pagedir, page->pge->vaddr);
	del_page_from_list_LRU (page);
	palloc_free_page (page->kaddr);
	free (page);

}
