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

   注意输入实现：菜单用 stdio（fgets/sscanf），fill 用 fread。
   绝不能在同一个 stdin 上混用 stdio 与 read()——scanf 的预读会把
   后续字节吞进自己的缓冲区，导致 read() 读到错位数据。
   这个坑在本 lab 开发中真实出现过（表现为 exploit 偶发失效），
   统一走 stdio 后消失。

   本 lab 主线：信息泄露拿到 libc 基址 -> UAF 写篡改 tcache 链 ->
   下一次 malloc 把"chunk"发到 __free_hook -> 改成 system。 */

#define SLOT 8
#define NOTE_SIZE 0x68        /* 0x70 chunk -> tcache[0x70] */

char *notes[SLOT];
long  sizes[SLOT];

static int read_int(int *out) {
    char line[32];
    if (!fgets(line, sizeof(line), stdin)) return -1;
    return sscanf(line, "%d", out);
}

int main(void) {
    /* stdin 必须无缓冲：stdio 的预读会抢走本该交给子进程（shell）的字节，
       表现为拿到 shell 后命令被父进程吞掉。这是真实的交互程序坑。 */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    int choice, idx;
    while (1) {
        puts("1. alloc  2. fill  3. free  4. info  5. exit");
        printf("> ");
        if (read_int(&choice) != 1) return 0;
        if (choice == 1) {
            printf("idx: ");
            if (read_int(&idx) != 1 || idx < 0 || idx >= SLOT) return 0;
            if (notes[idx]) { puts("slot in use"); continue; }
            notes[idx] = malloc(NOTE_SIZE);
            sizes[idx] = NOTE_SIZE;
            printf("note %d at %p\n", idx, (void *)notes[idx]);
        } else if (choice == 2) {
            printf("idx: ");
            if (read_int(&idx) != 1 || idx < 0 || idx >= SLOT) return 0;
            /* 缺陷：不检查 notes[idx] 是否已 free —— UAF 写 */
            printf("data: ");
            fread(notes[idx], 1, sizes[idx], stdin);
        } else if (choice == 3) {
            printf("idx: ");
            if (read_int(&idx) != 1 || idx < 0 || idx >= SLOT) return 0;
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
