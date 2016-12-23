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
	struct page_entry *vme = hash_entry (e, struct page_entry, elem);
	free_page_vaddr (vme->vaddr);
	swap_clear (vme->swap_slot);
	free (vme);

}

struct page_entry *
find_vme (void *vaddr)
{
	struct hash *spt;
	struct page_entry vme;
	struct hash_elem *elem;

	spt = &thread_current ()->sup_page_tab;
	vme.vaddr = pg_round_down (vaddr);
	ASSERT (pg_ofs (vme.vaddr) == 0);
	elem = hash_find (spt, &vme.elem);
	return elem ? hash_entry (elem, struct page_entry, elem) : NULL;
}

	bool
insert_vme (struct hash *spt, struct page_entry *vme)
{
	ASSERT (spt != NULL);
	ASSERT (vme != NULL);
	ASSERT (pg_ofs (vme->vaddr) == 0);
	return hash_insert (spt, &vme->elem) == NULL;
}

	bool
delete_vme (struct hash *spt, struct page_entry *vme)
{
	ASSERT (spt != NULL);
	ASSERT (vme != NULL);
	if(!hash_delete(spt,&vme->elem))
		return false;
	free_page_vaddr(vme->vaddr);
	swap_clear(vme->swap_slot);	
	free(vme);
	return true;
}

bool load_file (void *kaddr, struct page_entry *vme)
{
	ASSERT (kaddr != NULL);
	ASSERT (vme != NULL);
	ASSERT (vme->type == VM_BIN);

	if (file_read_at (vme->file, kaddr, vme->read_bytes, vme->offset) != (int)vme->read_bytes)
	{
		return false;
	}
	memset (kaddr + vme->read_bytes, 0, vme->zero_bytes);
	return true;
}

static void collect (void)
{
	lock_acquire (&lru_list_lock);
	
	struct page *victim = get_victim ();

	ASSERT (victim != NULL);
	ASSERT (victim->thread != NULL);
	ASSERT (victim->thread->magic == 0xcd6abf4b);
	ASSERT (victim->vme != NULL);

	bool dirty = pagedir_is_dirty (victim->thread->pagedir, victim->vme->vaddr);
	
	switch (victim->vme->type)
	{
		case VM_BIN:
			if (dirty)
			{
				victim->vme->swap_slot = swap_out (victim->kaddr);
				victim->vme->type = VM_ANON;
			}
			break;
		case VM_ANON:
			victim->vme->swap_slot = swap_out (victim->kaddr);
			break;
		default:
			NOT_REACHED ();
	}
	victim->vme->is_loaded = false;
	__free_page (victim);
	lock_release (&lru_list_lock);
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
		collect ();
		page->kaddr = palloc_get_page (flags);
	}

	return page;
}

extern struct list lru_list;
extern struct list_elem *lru_clock;

	void
free_page_kaddr (void *kaddr)
{
	lock_acquire (&lru_list_lock);

	struct page *page = find_page_from_lru_list (kaddr);
	
	if(page)
		__free_page(page);

	lock_release(&lru_list_lock);

}

void
free_page_vaddr (void *vaddr)
{
	free_page_kaddr (pagedir_get_page (thread_current ()->pagedir, vaddr));
}

	void
__free_page (struct page *page)
{
	ASSERT (lock_held_by_current_thread (&lru_list_lock));

	ASSERT (page != NULL);
	ASSERT (page->thread != NULL);
	ASSERT (page->thread->magic == 0xcd6abf4b);
	ASSERT (page->vme != NULL);

	pagedir_clear_page (page->thread->pagedir, page->vme->vaddr);
	del_page_from_lru_list (page);
	palloc_free_page (page->kaddr);
	free (page);

}
