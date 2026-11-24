#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* UAF：free 之后指针没有置空，后续可以继续使用这块悬垂内存。
   结构体里带函数指针 —— 一旦这块内存被重新分配并写入新内容，
   旧指针调用 ->talk() 时执行的就是攻击者写入的地址。 */

struct person {
    char name[24];
    void (*talk)(void);      /* 函数指针：UAF 劫持的靶子 */
};

void safe_hello(void) {
    puts("hello, I am a normal person");
}

void backdoor(void) {
    system("/bin/sh");
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    puts("== UAF: create a person ==");
    struct person *p = malloc(sizeof(struct person));
    p->talk = safe_hello;
    strncpy(p->name, "alice", sizeof(p->name) - 1);

    free(p);                 /* 释放了…… */
    /* 缺陷：没有 p = NULL。p 仍是悬垂指针，指向的 chunk 已回到 tcache。 */

    puts("now the freed slot is reused by a 'note' buffer");
    char *note = malloc(sizeof(struct person));   /* 同尺寸 -> 直接复用那个 chunk */
    puts("write your note (48 bytes, includes a function address):");
    /* note 与 p 指向同一块内存：这里写入的内容 = p->talk 变成的内容 */
    read(0, note, sizeof(struct person));

    puts("the dangling pointer still believes it is a person:");
    p->talk();               /* 调用的已经是 note 里写入的地址 */
    free(note);
    return 0;
}
