# lab-04 · tcache poisoning：从信息泄露到 `__free_hook`（glibc 2.31）

四个 lab 里最接近真实 CVE 分析的一个：**信息泄露 → 堆布局操纵 → 任意地址分配 →
函数指针劫持**，四步串成完整利用链。文末还记录了一个真实失败：
「照抄 2.23 时代的 fastbin 老教程」在本环境直接 SIGABRT，以及为什么。

## 漏洞面（三个）

1. `free` 不清指针 —— 悬垂指针，可被 `fill` 继续写（UAF 写）
2. `fill` 不检查 chunk 是否在使用中
3. 调试接口 `info` 直接打印 `free()` 地址 —— 信息泄露，绕过 ASLR

## 为什么不能直接 double free

glibc 2.29+ 的 tcache entry 带 `key` 字段，`free` 时发现 key 仍指向 tcache，
直接 `free(): double free detected in tcache 2` 中止。所以本 lab 走 **UAF 写**：
free 一次合法，但指针没清空，可继续改它的 `next`。

## tcache poisoning 全流程（两步都要对）

```
① 泄露：info -> free() 运行时地址
   libc_base = free_runtime - free 在 libc 内的偏移

② 堆：需要 tcache 里有 **两个** chunk，改「链头」的 next
   free(A); free(B)        tcache: B -> A   (counts = 2, 链头 B)
   fill(B, p64(hook))      tcache: B -> hook
   malloc()                返回 B    (counts = 1, 链头 = hook)
   malloc()                返回 hook (counts = 0) —— 任意地址分配
   fill(hook, p64(system)) __free_hook = system
```

### 为什么要两个 chunk、要改链头

这是本 lab 用最小 C 程序逐个验证出来的约束：

- glibc 2.31 的 `__libc_malloc` 判断 `tcache->counts[idx] > 0` 才走 tcache。
  只 free 一个 chunk（counts=1），弹出后 counts=0，
  链上残留的 target 永远不会被返回——**单 chunk 中毒在 2.31 上无效**。
- 改「链头」的 next，使链变成 `B -> target` 且 counts=2，
  这样第二枪弹出时 counts 恰好还够——target 才能被返回。

### 目标地址必须可读

`tcache_get` 弹出时会解引用 `e->next`。把 next 指向不可读地址
（如 `0xdeadbeef`）会直接 SIGSEGV —— 最小复现时实测过。
`__free_hook` 位于 libc 映射内，可读，符合条件。

## 一个真实失败：为什么不用 fastbin dup 打 `__malloc_hook`

经典教材（glibc 2.23）套路是 fastbin double free → 伪造 chunk 到
`__malloc_hook - 0x23`，靠该处恰好存在的 `0x7f` 字节骗过 size 校验。

本环境实测：

```
__malloc_hook - 0x1b 处的 8 字节 = 0x0     ← 不是 0x7f
```

`_int_malloc` 弹出 fake chunk 时校验 `fastbin_index(chunksize(victim))`，
size 不合法 → `malloc(): memory corruption (fast)` → SIGABRT。
**照抄老教程会直接崩**，这就是本 lab 改用 tcache 路线的原因。
（顺带：2.34+ 之后 `__free_hook/__malloc_hook` 整体被移除，
hook 劫持的时代结束，FSOP / exit handler 成为后续方向。）

## 版本演进

| 版本 | tcache next 字段 | 攻击难度 |
|---|---|---|
| ≤ 2.31 | 明文指针 | 拿到任意写地址即可，本 lab |
| 2.32+ | **safe-linking**：`next = ptr ^ (chunk_addr >> 12)` | 需先泄露堆地址，再按公式异或还原 |
| 2.34+ | 同上，且 hook 移除 | 需换目标（FSOP 等） |

## 运行

```bash
make && python3 exploit.py
# 期望：libc base 打印 + PWNED_LAB04 回显
```

## 防御修复

1. free 后置 NULL；fill 前检查指针状态（前两个缺陷一起根治）
2. 生产环境关闭调试接口（第 3 个缺陷；真实世界大量泄露来自忘关的 debug 端点）
3. 升级 glibc：2.32+ 挡明文 next 篡改，2.34+ 移除 hook

## 验证状态

`run_tests.sh` 容器内连续三轮实测通过（打印 libc base 并回显 `PWNED_LAB04`）。
