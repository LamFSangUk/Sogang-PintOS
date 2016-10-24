#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
/*na-10.23 make syscall_handler, syscall_halt(), syscall_read(),syscall_write()*/

//for #proj2_1 make functions//
void syscall_halt(void);
void syscall_exit (int status); 
void syscall_exec (const char *cmd_line); 
int syscall_wait (int pid); 
int syscall_read (int fd, void *buffer, unsigned size);
int syscall_write (int fd, const void *buffer, unsigned size);

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
  printf ("system call!\n");
  //syscallnum = (f->esp);

  //printf("%d",syscallnum);

	//TODO : Check the Stack Pointer is in the User Area
	//first 4 bytes of f->esp is syscall number.
	//We know how many argus by argc, so we can get argu's addr.
	//And then, We must check the argu's addr is valid.
  /*
  if (syscallnum == SYS_HALT){
    syscall_halt();
  }
  else if (syscallnum == SYS_EXIT){
	//syscall_exit();
  }
  else if (syscallnum == SYS_EXEC){
	//syscall_exec();
  }
  else if (syscallnum == SYS_WAIT){
	//syscall_wait();
  }
  else if (syscallnum== SYS_READ){
	//syscall_read();
  }
  else if (syscallnum== SYS_WRITE){
	//syscall_write();
  }
*/
  thread_exit ();
}

void
syscall_halt (void) 
{
  shutdown_power_off();
  NOT_REACHED();
}

void
syscall_exit (int status) 
{

    return status; 
}

void
syscall_exec (const char *cmd_line) 
{
}

int
syscall_wait (int pid) 
{
}

int
syscall_read (int fd, void *buffer, unsigned size)
{//we just make this part which enable stdin stdout to execute
    
}
int
syscall_write (int fd, const void *buffer, unsigned size)
{
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
