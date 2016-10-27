#define STDIN 0
#define STDOUT 1
#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
/*na-10.25*/
/*proj2_1*/
#include <string.h>
#include "kernel/console.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "threads/malloc.h"

static void syscall_handler (struct intr_frame *);
/*na-10.23 make syscall_handler, syscall_halt(), syscall_read(),syscall_write()*/

/*PROJECT2_1*/
void syscall_halt(void);
int syscall_exit (int status);
tid_t syscall_exec (const char *cmd_line); 
int syscall_wait (int pid);
int syscall_read (int fd, void *buffer, unsigned size);
int syscall_write (int fd, const void *buffer, unsigned size);

bool is_valid_userptr(const void* ptr);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
/*na-10.23*/
static void
syscall_handler (struct intr_frame *f UNUSED) //intr_frame : src/threads/interrupt.h
{
	int syscallnum;
	int* argu;
 
	if(!is_valid_userptr((const void*)(f->esp))){
		syscall_exit(-1);
		return;
	}

  syscallnum = *(int*)(f->esp);
  argu=(int*)(f->esp);


	//TODO : Check the Stack Pointer is in the User Area
	//first 4 bytes of f->esp is syscall number.
	//We know how many argus by argc, so we can get argu's addr.
	//And then, We must check the argu's addr is valid.
  
  if (syscallnum == SYS_HALT){
    syscall_halt();
  }
  else if (syscallnum == SYS_EXIT){
  	  if(!is_valid_userptr((const void*)&argu[1])){
  	  	  syscall_exit(-1);
  	  	  return;
	  }
  	  	f->eax=syscall_exit(argu[1]);
  }
  else if (syscallnum == SYS_EXEC){
  	  if(!is_valid_userptr((const void*)&argu[1])){
  	  	  syscall_exit(-1);
  	  	  return;
	  }
		f->eax=syscall_exec((const char*)argu[1]);
  }
  else if (syscallnum == SYS_WAIT){
  	  if(!is_valid_userptr((const void*)&argu[1])){
  	  	  syscall_exit(-1);
  	  	  return;
	  }
		f->eax=syscall_wait((int)argu[1]);
  }
  else if (syscallnum== SYS_READ){
		if(!(is_valid_userptr((const void*)&argu[5])
		   && is_valid_userptr((const void*)&argu[6]) 
		   && is_valid_userptr((const void*)&argu[7]))){
			syscall_exit(-1);
			return;
		}
	f->eax=syscall_read((int)argu[5],(void*)argu[6],(unsigned)argu[7]);
  }
  else if (syscallnum== SYS_WRITE){
		if(!(is_valid_userptr((const void*)&argu[5])
		   && is_valid_userptr((const void*)&argu[6])
		   && is_valid_userptr((const void*)&argu[7]))){
			syscall_exit(-1);
			return;
		}
	f->eax=syscall_write((int)argu[5],(const void*)argu[6],(unsigned)argu[7]);
  }

//  thread_exit ();
}

void
syscall_halt (void) 
{
  shutdown_power_off();
  NOT_REACHED();
}

int
syscall_exit (int status) 
{
    // TODO : 현재 thread가 가지고 있는 file을 모두 close하여pagefault방지
    // save the status of the current thread and call 'thread_execute()'
/*
	struct thread *cur = thread_current();
	if (thread_alive(cur->parent))
	{
		cur->cp->status = status;
	}
	printf ("%s: exit(%d)\n", cur->name, status);
	thread_exit();

*/
	struct thread *cur_thread;

	cur_thread=thread_current();
	
	if(is_thread(cur_thread->parent_thread) && is_thread_alive(cur_thread->parent_thread->tid)){
		cur_thread->pchild_data->child_status=status;
	}
	printf("%s: exit(%d)\n",cur_thread->name,status);
	thread_exit();
/*
	e=list_begin(&(cur_thread->parent_thread->child_tlist));
	while(1){
		child_thread=list_entry(e,struct thread,child_elem);
		if(cur_thread->tid == child_thread->tid){
			printf("%s: exit(%d)\n",cur_thread->name,status);
			cur_thread->parent_thread->child_status=THREAD_DYING;
			cur_thread->parent_thread->status=status;
			thread_exit();
			break;
		}
		e=list_next(e);
	}
	*/
}

tid_t
syscall_exec (const char *cmd_line) 
{
    // TODO : call process_execute and make process and save tid(check if error or not)
	// if the exit_status of  newly created thread not -1, return pid of this thread
/*	tid_t pid = process_execute(cmd_line);
	struct child_process* cp = get_child_process(pid);
	ASSERT(cp);
	while (cp->load == NOT_LOADED)
	{
		barrier();
	}
	if (cp->load == LOAD_FAIL)
	{
		return ERROR;
	}
	return pid;
  */
    if(cmd_line==NULL) return TID_ERROR;
   	else if(strcmp("no-such-file",cmd_line)==0) return -1;

    tid_t tid=0;
    bool find_child_flag=false;

	struct list_elem* e;
	struct thread* cur_thread;
	struct child_data *pchild_data=NULL;

	cur_thread=thread_current();
	tid=process_execute(cmd_line);

	for(e=list_begin(&cur_thread->child_tlist);e!=list_end(&cur_thread->child_tlist);
		e=list_next(e)){
		pchild_data=list_entry(e,struct child_data,child_elem);
		
		if(is_thread(pchild_data->t_child)
			&& tid==pchild_data->t_child->tid){
			find_child_flag=true;
			break;
		}
		
	}
	if(find_child_flag){
		while(pchild_data->is_loaded==NOT_LOADED){
			barrier();
		}
		if(pchild_data->is_loaded==LOAD_FAIL){
			return -1;
		}
		return tid;
	}
	else return -1;

//    tid=process_execute(cmd_line);
//	return tid;
}
int
syscall_wait (int pid) 
{
    //현재 thread에 저장된 child의 pid와 일치하는 thread를 찾아
    //process.c의 process_wait함수를 실행하면 끝 
    return process_wait(pid);
}

int
syscall_read (int fd, void *buffer, unsigned size)
{//we just make this part which enable stdin stdout to execute
    int i;

    if(fd == STDIN){
    	for(i=0;i<size;i++)
        	((uint8_t*)buffer)[i]=input_getc();//src/devices/input.c return key value(notzero)
        return size;
    }
}

int
syscall_write (int fd, const void *buffer, unsigned size)
{
    if(fd == STDOUT){
        putbuf((const char *)buffer,size);
        return size;
	}
}
/********************************
 * this function must be prototyped.
 *
 int pibonacci(int n)
 {
    if((n==0)
        return 0;
    else if(n==1)
        return 1;
    else
        return pibonacci(n-1)+pibonacci(n-2));

 }
 int sum_of_four_integers(int a, int b, int c, int d)
 {
    return a+b+c+d;
 }
 *
 *
 *
 *
 *********************************/

 bool is_valid_userptr(const void* ptr){
 	 if(ptr==NULL){
 	 	 return false;
	 }
	 else
		 return is_user_vaddr(ptr);
 }
 /*
struct child_process* add_child_process (int pid)
{
	struct child_process* cp = malloc(sizeof(struct child_process));
	cp->pid = pid;
	cp->load = NOT_LOADED;
	cp->wait = false;
	cp->exit = false;
	lock_init(&cp->wait_lock);
	list_push_back(&thread_current()->child_list,
			&cp->elem);
	return cp;
}

struct child_process* get_child_process (int pid)
{
	struct thread *t = thread_current();
	struct list_elem *e;

	for (e = list_begin (&t->child_list); e != list_end (&t->child_list);
			e = list_next (e))
	{
		struct child_process *cp = list_entry (e, struct child_process, elem);
		if (pid == cp->pid)
		{
			return cp;
		}
	}
	return NULL;
}

void remove_child_process (struct child_process *cp)
{
	list_remove(&cp->elem);
	free(cp);
}

void remove_child_processes (void)
{
	struct thread *t = thread_current();
	struct list_elem *next, *e = list_begin(&t->child_list);

	while (e != list_end (&t->child_list))
	{
		next = list_next(e);
		struct child_process *cp = list_entry (e, struct child_process,
				elem);
		list_remove(&cp->elem);
		free(cp);
		e = next;
	}
}*/
