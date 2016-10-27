#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syscall.h"
#incldue "../lib/user/syscall.h"

int
main(int argc, char **argv)
{
    int a,b,c,d;
    int fib;
    int sum = 0;

    a = atoi(argv[1]);
    b = atoi(argv[2]);
    c = atoi(argv[3]);
    d = atoi(argv[4]);

    sum = sum_of_4( a, b, c, d);
    fib = fibonacci(a);
    printf("%d %d\n",fib, sum);

    return 0;
}
