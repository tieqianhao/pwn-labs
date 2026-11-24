# lab-01 · ret2text：栈溢出返回到程序内已有函数

## 漏洞

`main` 里 `char buf[64]` 但 `read(0, buf, 256)` 允许写 256 字节——
超出栈帧的 192 字节会依次覆盖：`saved rbp`（8B）→ `return address`（8B）→ 上层栈帧。

## 原理：函数返回时发生了什么

```
main 被 call 时：
    push rip        ; 返回地址入栈
    push rbp        ; 旧栈底入栈
    sub  rsp, 64    ; 给 buf 腾空间

main 返回时：
    leave           ; mov rsp, rbp / pop rbp     ← 读走被覆盖的 rbp
    ret             ; pop rip                     ← 读走被覆盖的返回地址并跳过去
```

所以 `ret` 执行的那一刻，控制流完全由我们写入栈的那 8 字节决定。
本 lab 没有开任何缓解（见下），直接把这 8 字节填成 `backdoor` 的地址即可。

## 偏移计算

| 栈帧内容 | 大小 | 累计偏移 |
|---|---|---|
| buf | 64 B | 0 |
| 对齐/其他局部 | 8 B（gcc -O0 下恰好） | 64 |
| saved rbp | 8 B | 72 ← 返回地址从这里开始 |

精确偏移可以用 pattern 定位（pwntools `cyclic(200)` + `cyclic_find`），
也可以直接看汇编：`objdump -d vuln` 里 `sub rsp, 0x40`。

payload = `b"A"*72 + p64(ret) + p64(backdoor)`

> **栈对齐（实测踩到的坑）**：main 返回时 `rsp % 16 == 8`，直接 ret 进 backdoor，
> `system` 内部的 `movaps` 会因为 16 字节未对齐而 SIGSEGV——表现是
> 「payload 偏移全对，但 shell 不出来」。补一个裸 `ret` gadget 把 rsp 再挪 8 字节即可。
> 这个问题在 lab-01 首次运行时真实发生过，不是理论推演。

## 验证状态

`run_tests.sh` 容器内实测通过（`PWNED_LAB01` 回显，进程随后 SIGSEGV 属预期——
返回后没有合法栈帧可回）。

## 缓解机制对照（为什么这个 lab 能打通）

| 机制 | 本 lab 状态 | 若开启会怎样 |
|---|---|---|
| Stack Canary | `-fno-stack-protector` 关闭 | 开启时覆盖返回地址必先踩坏 canary，`__stack_chk_fail` 终止进程；需先泄露 canary |
| NX（栈不可执行） | 保持开启 | 不影响——我们跳到已有代码，不是注入代码 |
| PIE | `-no-pie` 关闭 | 开启时代码段随机化，backdoor 地址每次运行都变；需先泄露代码段地址 |
| RELRO | `-z norelro` 关闭 | 与本 lab 无关（不涉及 GOT 劫持） |

## 运行

```bash
make            # 编译
python3 exploit.py
# 期望输出：PWNED_LAB01（拿到 shell 并执行了 echo）
```

## 防御修复（开发者视角）

1. `read(0, buf, sizeof(buf))` —— 长度用常量而非魔数，这是本缺陷的直接根源
2. 编译默认开 `-fstack-protector-strong`（Ubuntu 发行版默认已开）
3. 启用 `FORTIFY_SOURCE`，让 `read`/`strcpy` 等在编译期检查边界
4. `backdoor` 这种调试后门函数不应出现在发布产物里；ret2text 成立的前提就是它存在
