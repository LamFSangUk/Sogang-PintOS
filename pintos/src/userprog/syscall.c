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
#include "threads/synch.h"

#define STDIN 0
#define STDOUT 1

static void syscall_handler (struct intr_frame *);
/*na-10.23 make syscall_handler, syscall_halt(), syscall_read(),syscall_write()*/

/*PROJECT2_1*/
static void syscall_halt(void);
static tid_t syscall_exec (const char *cmd_line); 
static int syscall_wait (int pid);
static int syscall_read (int fd, void *buffer, unsigned size);
static int syscall_write (int fd, const void *buffer, unsigned size);
static int syscall_fibonacci(int n);
static int syscall_sum_of_four_integers(int,int ,int,int);

static bool is_valid_userptr(const void* ptr);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
/*na-10.23
	----------MODIFIED FUNCTION----------
	syscall_handler get the syscall number from the interrupt frame.
	f->esp has syscall number, and passed arguments by syscall in
	lib/user/syscall.c
	It works to branch off codes. The syscall number is able to be
	SYS_HALT,SYS_EXIT,SYS_EXEC,SYS_WAIT,SYS_READ,SYS_WRITE,SYS_FIBO,
	and SYS_SUM4. The handler will execute the proper function.
	If the function has return value, save it to f->eax.						*/
static void
syscall_handler (struct intr_frame *f UNUSED) //intr_frame : src/threads/interrupt.h
{
	
	/*The first 4 bytes of f->esp is syscall number. We know 
		how many argus are given from lib/user/systemcall.c, 
		and they are saved in esp. So	we can get argu's addr,
		We can extract arguments from esp.
		To Do this work, We	must check the argu's addr is valid.*/

	int syscallnum;
	int* argu;
 
	if(!is_valid_userptr((const void*)(f->esp))){
		syscall_exit(-1);
		return;
	}
	
	//Get the syscallnum from esp.
  syscallnum = *(int*)(f->esp);
  argu=(int*)(f->esp);
  
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
		if(!is_valid_userptr((const void*)&argu[5])
		   || !is_valid_userptr((const void*)&argu[6]) 
		   || !is_valid_userptr((const void*)&argu[7])){
			syscall_exit(-1);
			return;
		}
		
		f->eax=syscall_read((int)argu[5],(void*)argu[6],(unsigned)argu[7]);
  }
  else if (syscallnum== SYS_WRITE){
		if(!is_valid_userptr((const void*)&argu[5])
				|| !is_valid_userptr((const void*)&argu[6])
				|| !is_valid_userptr((const void*)&argu[7])){
			syscall_exit(-1);
			return;
		}
		
		f->eax=syscall_write((int)argu[5],(const void*)argu[6],(unsigned)argu[7]);
  }
  else if(syscallnum==SYS_FIBO){
  	if(!is_valid_userptr((const void*)&argu[1])){
			syscall_exit(-1);
			return;
	  }
  	
  	f->eax=syscall_fibonacci((int)argu[1]);
  }
  else if(syscallnum==SYS_SUM4){
  	if(!is_valid_userptr((const void*)&argu[6])
  		  || !is_valid_userptr((const void*)&argu[7])
  	  	|| !is_valid_userptr((const void*)&argu[8])
  	  	|| !is_valid_userptr((const void*)&argu[9])){
  	  syscall_exit(-1);
  	  return;
		}
		
		f->eax=syscall_sum_of_four_integers((int)argu[6],(int)argu[7],(int)argu[8],(int)argu[9]);
  }
}

/*
	----------ADDED FUNCTION----------
	syscall_halt shutdowns the device.		*/
static void
syscall_halt (void) 
{
  shutdown_power_off();
  NOT_REACHED();
}

/*
	----------ADDED FUNCTION----------
	syscall_exit executes when the userprogram call 'exit(status)'
	It calls 'thread_exit()' to close the current thread. If it has a
	child, saves the status and notices the child's state .					*/
int
syscall_exit (int status) 
{
	
	struct thread *cur_thread;
	int *pchild_status=NULL;

	cur_thread=thread_current();

	if(cur_thread->pchild_data!=NULL){
		cur_thread->pchild_data->status = status;

		pchild_status=&(cur_thread->pchild_data->child_status);
		if(status<0) *pchild_status=KILLED;
		else *pchild_status=COMPLETE_EXIT;
	}

	printf("%s: exit(%d)\n",cur_thread->name,status);
	thread_exit();
	return status;
}

/*
	----------ADDED FUNCTION----------
	syscall_exec makes a new process by call process_execute(). If it
	works successfully, the current thread must wait until the child.
	To wait, we will check whether the child is loaded. If the child
	not loaded, we must wait, So it uses 'barrier()' to busy-wait. 
	After, the child loaded completely, the parent will stop 
	busy-waiting(child's loading complete means it executed successfully.*/
static tid_t
syscall_exec (const char *cmd_line) 
{
    // TODO : call process_execute and make process and save tid(check if error or not)
	// if the exit_status of  newly created thread not -1, return pid of this thread
    
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
	else return tid;
}
/*
	----------ADDED FUNCTION----------
	syscall_wait just calls 'process_wait().' In process_wait,
	we may do something to synchronize the threads.						*/

static int
syscall_wait (int pid) 
{
	return process_wait(pid);
}

/*
	----------ADDED FUNCTION----------
	syscall_read reads the data from file system. we just 
	implememted STDIN. It saves the data to buffer						*/

static int
syscall_read (int fd, void *buffer, unsigned size)
{
	unsigned i;

	if(fd == STDIN){
  	for(i=0;i<size;i++)
    	((uint8_t*)buffer)[i]=input_getc();//src/devices/input.c return key value(notzero)
      return size;
  }
  
  return -1;
}

/*
	----------ADDED FUNCTION----------
	syscall_write prints the data which is saved in buffer.
	We just implemented STDOUT.															*/

static int
syscall_write (int fd, const void *buffer, unsigned size)
{
	if(fd == STDOUT){
  	putbuf((const char *)buffer,size);
    return size;
  }
	
	return -1;
}
/*
	----------ADDED FUNCTION----------
	syscall_fibonacci calculates the input's fibonacci number.
	fibonacci number has recurrence formula. 
	f(n)=f(n-1)+f(n-2).																				*/ 

static int 
syscall_fibonacci(int _n_input)
{
	int _n_0=0,_n_1=1,_n_2=0,i;

	if(_n_input<0){
		return -1;
	}
	else if(_n_input==0){
		return 0;
	}
	else if(_n_input==1){
		return 1;
	}
	else{
		for(i=1;i<_n_input;i++){
			_n_2=_n_1+_n_0;
			_n_0=_n_1;
			_n_1=_n_2;
		}

		return _n_2;
	}
}

/*
	----------ADDED FUNCTION----------
	syscall_sum_of_four_integers just calculate the sum of
	given four numbers.																		*/
static int syscall_sum_of_four_integers(int _para_1, int _para_2, int _para_3, int _para_4)
{
   return _para_1+_para_2+_para_3+_para_4;
}

/*
	----------ADDED FUNCTION----------
	is_valid_userptr checks the pointer's addr is valid for
	user. If it points to kernel's memory area, the OS must
	defend it. Cause if user can correct kernel's memory,
	the operating system program will not correctly work.		*/

static bool is_valid_userptr(const void* ptr){
	if(ptr==NULL){
 		return false;
 	}
	else
		return is_user_vaddr(ptr);
}
