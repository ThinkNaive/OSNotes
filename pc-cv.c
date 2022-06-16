// http://jyywiki.cn/pages/OS/2022/demos/pc-cv.c
// gcc pc-cv.c -lpthread && ./a.out 1 | head -c 100

#include "thread.h"
#include "thread-sync.h"

int n, count = 0;
mutex_t lk = MUTEX_INIT();
cond_t cv = COND_INIT();

void Tproduce() {
  while (1) {
    mutex_lock(&lk);
    while (count == n) {
      cond_wait(&cv, &lk);
    }
    printf("("); count++;
    cond_broadcast(&cv);
    mutex_unlock(&lk);
  }
}

void Tconsume() {
  while (1) {
    mutex_lock(&lk);
    while (count == 0) {
      pthread_cond_wait(&cv, &lk);
    }
    printf(")"); count--;
    cond_broadcast(&cv); // 若使用cond_signal可能Tconsume唤醒Tconsume，导致死锁
    mutex_unlock(&lk);
  }
}

int main(int argc, char *argv[]) {
  assert(argc == 2);
  n = atoi(argv[1]);
  setbuf(stdout, NULL);
  for (int i = 0; i < 8; i++) {
    create(Tproduce);
    create(Tconsume);
  }
}
