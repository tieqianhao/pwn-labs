#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* 与 lab-01 的区别：没有后门函数了。
   程序只引用了 puts/printf/read/libc_start_main 等 libc 函数，
   要拿到 shell 必须自己定位 libc 里的 system 与 "/bin/sh"。 */

int main(void) {
    char buf[64];
    setvbuf(stdout, NULL, _IONBF, 0);
    puts("== ret2libc: no backdoor this time ==");
    read(0, buf, 256);          /* 同样的栈溢出缺陷 */
    printf("bye\n");
    return 0;
}
