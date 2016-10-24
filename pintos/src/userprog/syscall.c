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
pid_t syscall_exec (const char *cmd_line); 
int syscall_wait (pid_t pid); 
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
  
  syscallnum = f->esp;

  
  if (syscallnum == SYS_HALT){
    syscall_halt();
  }
  else if (syscallnum == SYS_EXIT){

  }
  else if (syscallnum == SYS_EXEC){

  }
  else if (syscallnum == SYS_WAIT){

  }
  else if (syscallnum== SYS_READ){

  }else if (syscallnum== SYS_WRITE){

  }




 
  thread_exit ();
}

void
syscall_halt (void) 
{
  shutdown_power_off();
}

void
syscall_exit (int status) 
{

    return status;
}

pid_t
syscall_exec (const char *cmd_line) 
{
}

int
syscall_wait (pid_t pid) 
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
