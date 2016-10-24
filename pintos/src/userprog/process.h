#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H
#define MAX_ARGS 128
#include "threads/thread.h"
/*na-2016.10.22 : add parse_filename,construct_ESP*/
int parse_filename(const char *file_name, char *argu_list[MAX_ARGS]);
void construct_ESP(void **esp, char *argu_listi[MAX_ARGS],int argu_num);

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
