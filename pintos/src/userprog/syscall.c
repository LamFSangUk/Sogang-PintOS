#define STDIN 0
#define STDOUT 1
#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
/*na-10.25*/
/*proj2_1*/
#include "kernel/console.h"
#include "devices/input.h"
#include "devices/shutdown.h"
static void syscall_handler (struct intr_frame *);
/*na-10.23 make syscall_handler, syscall_halt(), syscall_read(),syscall_write()*/

/*PROJECT2_1*/
void syscall_halt(void);
int syscall_exit (int status);
tid_t syscall_exec (const char *cmd_line); 
int syscall_wait (int pid);
void syscall_read (int fd, void *buffer, unsigned size);
void syscall_write (int fd, const void *buffer, unsigned size);

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
  	  	f->eax=syscall_exit(argu[1]);
  }
  else if (syscallnum == SYS_EXEC){
		f->eax=syscall_exec((const char*)argu[1]);
  }
  else if (syscallnum == SYS_WAIT){
		f->eax=syscall_wait((int)argu[1]);
  }
  else if (syscallnum== SYS_READ){
	syscall_read((int)argu[5],(const void*)argu[6],(unsigned)argu[7]);
  }
  else if (syscallnum== SYS_WRITE){
	syscall_write((int)argu[5],(const void*)argu[6],(unsigned)argu[7]);
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
	struct thread *cur_thread;

	cur_thread=thread_current();
	printf("%s: exit(%d)\n",cur_thread->name,status);
	thread_exit();
}

tid_t
syscall_exec (const char *cmd_line) 
{
    // TODO : call process_execute and make process and save tid(check if error or not)
    // if the exit_status of  newly created thread not -1, return pid of this thread
    tid_t tid;
    tid=process_execute(cmd_line);
	return tid;
}
int
syscall_wait (int pid) 
{
    //현재 thread에 저장된 child의 pid와 일치하는 thread를 찾아
    //process.c의 process_wait함수를 실행하면 끝 
    return process_wait(pid);
}

void
syscall_read (int fd, void *buffer, unsigned size)
{//we just make this part which enable stdin stdout to execute
    int i;

    if(fd == STDIN){
    	for(i=0;i<size;i++)
        	((uint8_t*)buffer)[i]=input_getc();//src/devices/input.c return key value(notzero)
    }
}

void
syscall_write (int fd, const void *buffer, unsigned size)
{
    if(fd == STDOUT)
        putbuf((const char *)buffer,size);
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
