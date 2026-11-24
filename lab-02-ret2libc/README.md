# lab-02 · ret2libc：没有后门时，自己拼一个

## 与 lab-01 的本质区别

程序里没有 `backdoor` 了，且 NX 开启（栈不可执行）——
**既不能跳已有后门，也不能往栈上放代码**。
唯一出路：跳到 libc 里**本来就有的** `system("/bin/sh")`。

## 难点：libc 加载基址未知（ASLR）

ASLR 每次运行把 libc 映射到随机地址，但你**不需要猜**——
GOT 表里存的就是本次运行中 `puts` 的真实地址。
让程序替你把它打印出来（`puts(puts@GOT)`），再用：

```
libc_base = 泄露的 puts 运行时地址 − puts 在 libc 文件内的偏移
system    = libc_base + system 偏移
"/bin/sh" = libc_base + binsh 偏移
```

这就是「泄露 → 计算 → 二段利用」的三步节奏，几乎所有 libc 题都是这个骨架。

## ROP 是什么

返回地址只有 8 字节、只能控制 rip 一次，但栈是**我们写满的**——
把一连串「地址 + 参数」铺在栈上，`ret` 会像链条一样逐个弹出执行：

```
ret → pop rdi; ret   ← 弹出下一项到 rdi（第一参数寄存器）
       puts@GOT       ← rdi = 要泄露的地址
       puts@PLT       ← ret 到 puts，打印泄露内容
       main           ← puts 返回后回到 main，开始第二段
```

`pop rdi; ret` 这类 2 字节碎片叫 **gadget**，在二进制里用 ROPgadget 或
pwntools `ROP(elf).find_gadget(['pop rdi', 'ret'])` 找。
`__libc_csu_init` 尾部的 `pop rdi; pop rbp...; ret` 是 64 位题最常用的来源。

## 两个必修的坑

1. **栈对齐**：`system` 内部 `movaps` 要求 `rsp % 16 == 0`，不对齐直接 SIGSEGV，
   表现为「payload 看起来全对但 shell 不出来」。解法：链里加一个裸 `ret`。
2. **泄露读取**：puts 输出以 `\n` 结尾且地址是 6 字节有效，
   直接用 `recvline()` 有概率被地址中间的 `0x0a` 字节截断——
   本 lab 实测就踩到了（泄露值变成 5 字节，算出的 base 全错、直接 SIGSEGV）。
   正确做法是 `recv(6)` 固定读 6 字节，或用定界符包裹后读取。

## 验证状态

`run_tests.sh` 容器内实测通过（打印 libc base 并回显 `PWNED_LAB02`）。

## 缓解机制对照

| 机制 | 状态 | 若开启会怎样 |
|---|---|---|
| NX | 开启（默认） | 正是本题前提：栈上数据不可执行 |
| Canary | 关闭 | 开启时第一段 payload 就会撞死，需先泄露 canary（常见套路：格式化字符串漏洞） |
| PIE | 关闭 | gadget 地址（0x4013F3）才能写死；开启时需先泄露代码段 |
| Full RELRO | 关闭 | 本题不打 GOT；开启 Full RELRO 时 GOT 只读，就不能改 GOT 但 ret2libc 本身不受影响 |

## 运行

```bash
make && python3 exploit.py
# 期望：libc base 打印 + PWNED_LAB02 回显
```

## 防御修复

同 lab-01 的 1–3 条全部适用。额外的：
5. 编译开 PIE（`-pie` 默认）+ Full RELRO（`-Wl,-z,relro,-z,now`），
   代码段与 GOT 全随机/只读，gadget 地址无法写死
6. 不给「反复触发同一溢出」的机会——本题能二段利用，靠的是 main 被重新进入
