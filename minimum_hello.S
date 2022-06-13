// gcc minimum_hello.S -c && ld minimum_hello.o && ./a.out

#include <sys/syscall.h>

.globl _start
_start:
    movq $SYS_write,    %rax
    movq $2,            %rdi
    movq $st,           %rsi
    movq $(ed-st),      %rdx
    syscall

    movq $SYS_exit,     %rax
    movq $0,            %rdi
    syscall

st:
    .ascii "\033[01;31mHello World\033[0m\n"
ed: