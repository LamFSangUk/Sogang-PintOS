#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdint.h>
#include <stdbool.h>

void syscall_init (void);
int syscall_exit (int status);

bool is_valid_stack(int32_t addr,int32_t esp);

#endif /* userprog/syscall.h */
