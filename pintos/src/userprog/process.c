#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"

/*na-2016.10.22*/
/*psu-2016.10.23*/

#include "threads/malloc.h"
#include "userprog/syscall.h"

#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);
extern struct lock _lock_for_file;
/****************************************/
/* 
 na-2016.10.22
 extract file name(program name) for executing the process
 //using strtok_r() strtok_r(char *s, const char * delimiters, char **save_ptr)
 return number of arguments
 na-2016.10.23
 strlcpy file_name change progName 
 psu-2016.10.22
 change strcpy to strlcpy Cause strcpy is not defined in string.h
 
*/
int
parse_filename(const char *file_name, char *argu_list[MAX_ARGS])
{
	char *s,*token, *save_ptr;
  int cnt = 0,len_file_name,len_token;

	len_file_name=strlen(file_name)+1;
	s=(char*)malloc(sizeof(char)*len_file_name);

	strlcpy(s,file_name,len_file_name);

  for(token = strtok_r(s, " ",  &save_ptr) ; token != NULL;
  		token = strtok_r(NULL, " ",  &save_ptr)) 
  	{
  		len_token=strlen(token)+1;
      
      if(cnt ==0)
      	{
        	strlcpy((char*)file_name, token, len_token);
        }
        
      argu_list[cnt]=(char*)malloc(sizeof(char)*len_token);
      strlcpy(argu_list[cnt],token, len_token);
        
      cnt++;
    }

	free(s);

  return cnt;
  //make the argulist.
  //return the number of argus.
}
/*
 psu-2016.10.22
 construct stack with the arguments
 na-2016.10.22
 construct word-alignment system
*/

void
construct_ESP(void **esp, char *argu_list[MAX_ARGS],int argu_num)
{
	size_t size_argu; 
	int i, totallen_align = 0;              //totallen_align is total sum of argument lengths when we calculate all length of the arguments FOR word alignment!!
  int addr[128];

  memset(addr,0,argu_num*sizeof(int));
    /*1. argv[3]~argv[0] memset*/
  for(i=argu_num-1;i>=0;i--){
       
  	//TODO : copy string to esp(maybe use memcpy)
    size_argu=strlen(argu_list[i])+1;     //strlen() doesn't count NULL character. so each size_argu  plus more 1.
    totallen_align += size_argu;
    *esp-=size_argu;
    addr[i] = (int)*esp;
    memcpy(*esp,argu_list[i],size_argu);
  }
 
  /*2. padding */
  if(totallen_align%4!=0){ 
  	for(i=0; i< 4-(totallen_align%4);i++){
    	*esp-= 1;
     	memset(*esp, 0, sizeof(uint8_t));       
    }
  }
  
  /*3. argv[4] memcpy  */
  *esp-=4;
  memset(*esp, 0, sizeof(char *));

  /*4. argv[3]~argv[0] memcpy */
  for(i=argu_num-1;i>=0;i--){ 
  	*esp-= 4;
    memcpy(*esp,&addr[i],sizeof(char*));
  }
  
  //TODO : argv memset
	*esp-=4;
	*(char**)*esp=*esp+4;
	*esp-=4;
	*(int*)*esp=argu_num;
	*esp-=4;
	*(int*)*esp=0;


  //To debug
  //hex_dump((uintptr_t)*esp,(const char*)*esp,PHYS_BASE-(uintptr_t)*esp,true);
}
/******************************************/
/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);
	
  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);

  if (tid == TID_ERROR)
    palloc_free_page (fn_copy); 

  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (file_name, &if_.eip, &if_.esp);

	thread_current()->is_loaded=success;

	sema_up(&thread_current()->sema_load);

  /* If load failed, quit. */
  palloc_free_page (file_name);
  if (!success)
    thread_exit ();

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid UNUSED) 
{
	struct thread *child_thread;
	int exit_status;

	if((child_thread=thread_get_child(child_tid))!=NULL){
		sema_down(&child_thread->sema_wait);
		list_remove(&child_thread->child_elem);
		exit_status=child_thread->exit_status;
		sema_up(&child_thread->sema_elim);

		return exit_status;
	}
	return -1;
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

	//Close currnet thread's all files.
	cur->fd_num=cur->fd_num-1; //Cause fd_num point to fd_tab for next file descripter.
	for(;cur->fd_num>=2;cur->fd_num--)
		file_close(cur->fd_tab[cur->fd_num]);

	file_close(cur->exec_file);

	spt_destroy(&cur->sup_page_tab);	

	/* Destroy the current process's page directory and switch back
		 to the kernel-only page directory. */

  pd = cur->pagedir;
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
    for(token=strtok_r(file_name," "
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */

/*na-10.22 add parse_filename() */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofset;
  bool success = false;
  int i;
  char *argu_list[MAX_ARGS];
  int argu_num=0;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  /*na-2016.10.22*/
  argu_num = parse_filename(file_name, argu_list);
  strlcpy(t->name,file_name,strlen(file_name)+1);
  /* Open executable file. */
	
	/*kny-2016.11.13*/
	lock_acquire(&_lock_for_file);
  file = filesys_open (t->name);
  lock_release(&_lock_for_file);
  
  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", t->name);
      goto done; 
    }
	/*kny-2016.11.13*/
  t->exec_file = file;
  file_deny_write(file); 

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", t->name);
      goto done; 
    }

  /* Read program headers. */
  file_ofset = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;
		
      if (file_ofset < 0 || file_ofset > file_length (file))
        goto done;
      file_seek (file, file_ofset);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofset += sizeof phdr;

      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) //Clear 2016.10.23
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp))
    goto done;
  
  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

/*10.22 : add construct_ESP*/
	construct_ESP(esp, argu_list, argu_num);

  success = true;
 done:

	//psu 2016.10.23 : free the argu_list
	for(i=0;i<argu_num;i++){
		free(argu_list[i]);
	}
  /* We arrive here whether the load is successful or not. */
  //file_close (file);
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)//psu 2016.10.23 modify vaddr->offset
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment(struct file *file, off_t ofs, uint8_t *upage,
	uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
	ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT(pg_ofs(upage) == 0);
	ASSERT(ofs % PGSIZE == 0);

	file_seek(file, ofs);
	while (read_bytes > 0 || zero_bytes > 0)
	{
		/* Calculate how to fill this page.
		We will read PAGE_READ_BYTES bytes from FILE
		and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct page_entry *pge = (struct page_entry *)malloc(sizeof(struct page_entry));
		if(pge==NULL) return false;

		memset(pge,0,sizeof(struct page_entry));
		pge->type=VM_BIN;
		pge->file=file;
		pge->offset=ofs;
		pge->read_bytes=page_read_bytes;
		pge->zero_bytes=page_zero_bytes;
		pge->writable=writable;
		pge->vaddr=upage;

		insert_pge(&thread_current()->sup_page_tab,pge);

//		/* Get a page of memory. */
//		uint8_t *knpage = palloc_get_page(PAL_USER);
//		if (knpage == NULL)
//			return false;
//
//		/* Load this page. */
//		if (file_read(file, knpage, page_read_bytes) != (int)page_read_bytes)
//		{
//			palloc_free_page(knpage);
//			return false;
//		}
//		memset(knpage + page_read_bytes, 0, page_zero_bytes);

//		/* Add the page to the process's address space. */
//		if (!install_page(upage, knpage, writable))
//		{
//			palloc_free_page(knpage);
//			return false;
//		}

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;

		ofs+=page_read_bytes;
	}
	return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp) 
{
//  uint8_t *kpage;
	struct page *kpage;
	void *upage = ((uint8_t*)PHYS_BASE)-PGSIZE;

	struct page_entry *pge=(struct page_entry *)malloc(sizeof(struct page_entry));
	if(pge==NULL)
		return false;

  kpage = alloc_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
	{
			kpage->pge=pge;
			add_page_to_list_LRU(kpage);

			if(!install_page(upage,kpage->kaddr,true)){
				free_page_kaddr(kpage);
				free(pge);
				return false;
			}
			*esp=PHYS_BASE-12;

			memset(kpage->pge,0,sizeof(struct page_entry));
			kpage->pge->type=VM_ANON;
			kpage->pge->vaddr=upage;
			kpage->pge->writable=true;
			kpage->pge->is_loaded=true;
		

			insert_pge(&thread_current()->sup_page_tab,kpage->pge);
		}
	return true;
}
				
/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *th = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (th->pagedir, upage) == NULL
          && pagedir_set_page (th->pagedir, upage, kpage, writable));
}

bool
handle_mm_fault(struct page_entry *pge){
//	uint8_t *kpage;
	struct page *kpage;
	kpage = alloc_page(PAL_USER);

	ASSERT(kpage!=NULL);
	ASSERT(pg_ofs(kpage->kaddr)==0);
	ASSERT(pge!=NULL);

	kpage->pge=pge;

	switch(pge->type){
		case VM_BIN:
			if(!load_file(kpage->kaddr,pge) ||
					!install_page(pge->vaddr,kpage->kaddr,pge->writable)){
				free_page_kaddr(kpage);
				return false;
			}
			break;
		case VM_ANON:
			swap_in (pge->swap_slot, kpage->kaddr);
			ASSERT(pg_ofs(kpage->kaddr)==0);
			if (!install_page (pge->vaddr, kpage->kaddr, pge->writable)){
				free_page_kaddr(kpage);
				return false;
			}
			break;
		default:
			NOT_REACHED();
	}

	pge->is_loaded=true;
	add_page_to_list_LRU(kpage);
	return true;
}

void
stack_grow (void *addr)
{
	struct page *kpage;
	void *upage = pg_round_down (addr);

	struct page_entry *pge = (struct page_entry *)malloc(sizeof(struct page_entry));
	if (pge == NULL)
		return;

	kpage = alloc_page (PAL_USER | PAL_ZERO);
	if (kpage != NULL)
	{
		kpage->pge = pge;
		add_page_to_list_LRU (kpage);

		if (!install_page (upage, kpage->kaddr, true))
		{
			free_page_kaddr (kpage);
			free (pge);
			return;
		}

		memset (kpage->pge, 0, sizeof (struct page_entry));
		kpage->pge->type = VM_ANON;
		kpage->pge->vaddr = upage;
		kpage->pge->writable = true;
		kpage->pge->is_loaded = true;

		insert_pge (&thread_current ()->sup_page_tab, kpage->pge);
	}

}
