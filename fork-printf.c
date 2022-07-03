// http://jyywiki.cn/pages/OS/2022/demos/fork-printf.c
// gcc fork-printf.c && ./a.out

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
  int n = 2;
  for (int i = 0; i < n; i++) {
    pid_t pid1 = fork();
    printf("Hello, lv-%d, %d\n", i, pid1);
    // fflush(stdout);
  }
  for (int i = 0; i < n; i++) {
    wait(NULL);
  }
}
