#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 堆菜单程序：alloc / fill / free 三件套，外加一个"调试信息"接口。
   三个缺陷：
   1. free 后不清空指针 -> 悬垂指针，可被 fill 继续写入（UAF 写）
   2. 不检查 fill 的 chunk 是否在使用中 -> 可以改写已释放 chunk 的 fd
   3. 调试信息接口泄露 libc 函数地址 -> 绕过 ASLR（真实服务里这类
      "debug 接口没关"是最常见的信息泄露来源之一）

   本 lab 主线：信息泄露拿到 libc 基址 -> UAF 写篡改 tcache 链 ->
   下一次 malloc 把"chunk"发到 __free_hook -> 改成 system。 */

#define SLOT 8
#define NOTE_SIZE 0x68        /* 0x70 chunk -> tcache[0x70] */

char *notes[SLOT];
long  sizes[SLOT];

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    int choice, idx;
    while (1) {
        puts("1. alloc  2. fill  3. free  4. info  5. exit");
        printf("> ");
        if (scanf("%d", &choice) != 1) return 0;
        if (choice == 1) {
            printf("idx: ");
            if (scanf("%d", &idx) != 1 || idx < 0 || idx >= SLOT) return 0;
            if (notes[idx]) { puts("slot in use"); continue; }
            notes[idx] = malloc(NOTE_SIZE);
            sizes[idx] = NOTE_SIZE;
            printf("note %d at %p\n", idx, (void *)notes[idx]);
        } else if (choice == 2) {
            printf("idx: ");
            if (scanf("%d", &idx) != 1 || idx < 0 || idx >= SLOT) return 0;
            /* 缺陷：不检查 notes[idx] 是否已 free —— UAF 写 */
            printf("data: ");
            read(0, notes[idx], sizes[idx]);
        } else if (choice == 3) {
            printf("idx: ");
            if (scanf("%d", &idx) != 1 || idx < 0 || idx >= SLOT) return 0;
            /* 缺陷：free 后不清 NULL */
            free(notes[idx]);
        } else if (choice == 4) {
            /* 缺陷：调试接口泄露 libc 地址，等于把 ASLR 关了一半 */
            printf("free() is at [%p]\n", (void *)free);
        } else {
            return 0;
        }
    }
}
