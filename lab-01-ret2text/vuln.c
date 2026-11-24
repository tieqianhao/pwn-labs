#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* 后门函数：程序里现成的 get shell 入口。
   lab-01 的目标就是让 main 的返回地址落到这里。 */
void backdoor(void) {
    system("/bin/sh");
}

int main(void) {
    char buf[64];
    setvbuf(stdout, NULL, _IONBF, 0);
    puts("== ret2text: tell me your name ==");
    /* 缺陷：buf 只有 64 字节，却允许读 256 字节 —— 经典栈溢出。 */
    read(0, buf, 256);
    printf("hello, %s\n", buf);
    return 0;
}
