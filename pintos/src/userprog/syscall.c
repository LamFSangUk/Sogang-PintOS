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
tid_t syscall_exec (const char *cmd_line); /*
int syscall_wait (int pid); */
void syscall_read (int fd, void *buffer, unsigned size);
void syscall_write (int fd, const void *buffer, unsigned size);


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
	char* argu_list[4];
  
  syscallnum = *(int*)(f->esp);


	//TODO : Check the Stack Pointer is in the User Area
	//first 4 bytes of f->esp is syscall number.
	//We know how many argus by argc, so we can get argu's addr.
	//And then, We must check the argu's addr is valid.
  
  if (syscallnum == SYS_HALT){
    syscall_halt();
  }
  else if (syscallnum == SYS_EXIT){
  	  argu_list[0]=(char*)(f->esp+4);
  	  f->eax=syscall_exit(*(int*)argu_list[0]);
  }
  else if (syscallnum == SYS_EXEC){
	//syscall_exec();
  }
  else if (syscallnum == SYS_WAIT){
	//syscall_wait();
  }
  else if (syscallnum== SYS_READ){
		argu_list[0]=(char*)(f->esp+4);
		argu_list[1]=(char*)(f->esp+8);
		argu_list[2]=(char*)(f->esp+12);
	syscall_read(*(int*)argu_list[0],(const void*)argu_list[1],*(unsigned *)argu_list[2]);
  }
  else if (syscallnum== SYS_WRITE){
  	  argu_list[0]=(char*)(f->esp+4);
  	  argu_list[1]=(char*)(f->esp+8);
  	  argu_list[2]=(char*)(f->esp+12);
	syscall_write(*(int*)argu_list[0],(const void*)argu_list[1],*(unsigned *)argu_list[2]);
  }

  thread_exit ();
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
	f->eax=status;
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
    process_wait(pid);
}

void
syscall_read (int fd, void *buffer, unsigned size)
{//we just make this part which enable stdin stdout to execute
    if(fd == STDIN){
        input_getc();//src/devices/input.c return key value(notzero)
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
