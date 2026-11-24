# lab-03 · UAF：释放后的内存还在听你的

## 漏洞

`free(p)` 之后没有 `p = NULL`，p 变成**悬垂指针**；
随后同尺寸的 `malloc` 会从 tcache 拿回**同一个 chunk**，
此时 `note` 和 `p` 指向同一块内存——写 note 就是改 p 的世界。

## 原理：glibc tcache 的回收与复用

```
free(p):     chunk A 头部写入 next/key，挂进 tcache bin[0x30]     （不再清零用户数据区）
malloc(48):  tcache 命中 -> 直接摘下 chunk A（后进先出）          （不做任何清零）
```

关键：**free 不擦除数据、malloc 不清零内存**。所以：

```
struct person 内存布局（0x30 chunk）：
    [ name[24] ][ talk 8B ]     <- p 视角
    [ note 写入区 ........]     <- note 视角（同一块内存）
note 写 "N"*24 + p64(backdoor)  ==  p->name 不变，p->talk = backdoor
```

`p->talk()` 的 `call rax` 打到 `backdoor` 上，shell 到手。

## 这个 lab 模拟了真实世界的什么

- C++ 里析构后继续调用对象方法（同名 chunk 被新对象占据）
- 浏览器/内核 UAF（CVE 常客）：释放后的对象被另一类型对象复用（type confusion）
- 检测工具：ASan（AddressSanitizer）对 UAF 是秒级检出，`-fsanitize=address` 跑一遍就报 heap-use-after-free

## 缓解机制对照

| 机制 | 对 UAF 的作用 |
|---|---|
| tcache key 检测（glibc 2.29+） | 只防 double free，不防 UAF（本 lab 正常 free 一次，检测不触发） |
| safe-linking（2.32+） | 加密 next 指针，影响的是 tcache poisoning，不是本 lab |
| ASan / 保守的编码习惯 | 真正的解法 |

## 运行

```bash
make && python3 exploit.py
# 期望：PWNED_LAB03 回显
```

## 验证状态

`run_tests.sh` 容器内实测通过（`PWNED_LAB03` 回显）。

## 防御修复

1. `free(p)` 之后立即 `p = NULL`（悬垂指针的根治法）
2. 释放后不再保留任何指向该内存的引用；确需复用，走所有权设计（Rust/C++ smart pointer）
3. CI 里加 `-fsanitize=address` 的测试管线，UAF 在测试期就该爆出来
