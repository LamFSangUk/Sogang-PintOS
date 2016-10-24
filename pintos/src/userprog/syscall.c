#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) //intr_frame : src/threads/interrupt.h
{
  printf ("system call!\n");
  thread_exit ();
}
/*
void
halt (void) 
{
  shutdown_power_off();
}

void
exit (int status) 
{

    return status;
}

pid_t
exec (const char *cmd_line) 
{
}

int
wait (pid_t pid) 
{
}

int
read (int fd, void *buffer, unsigned size)
{//we just make this part which enable stdin stdout to execute
}
int
write (int fd, const void *buffer, unsigned size)
{
}*/
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
