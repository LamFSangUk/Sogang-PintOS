#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "vm/page.h"

void list_LRU_init (void);
void add_page_to_list_LRU (struct page *);
struct page *find_page_from_list_LRU (void *);
void del_page_from_list_LRU (struct page *);

struct page *get_victim (void);

#endif
