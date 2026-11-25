# pwn-labs · Linux 二进制漏洞利用实验室

![regression](https://github.com/tieqianhao/pwn-labs/actions/workflows/regression.yml/badge.svg)

四个可复现的堆/栈漏洞利用实验，从「栈溢出返回后门」一路打通到
「glibc 2.31 信息泄露 + tcache poisoning 劫持 `__free_hook`」。
每个 lab = 漏洞源码 + exploit 脚本 + 逐段原理讲解 + 缓解机制对照 + 防御修复，
全部在 Docker 环境中真实跑通后才写进文档。

## 实验

| Lab | 技术 | 关键点 |
|---|---|---|
| [lab-01 ret2text](lab-01-ret2text/) | 栈溢出 → 跳转程序内后门函数 | 栈帧布局、偏移计算、canary/NX/PIE 各自挡住什么 |
| [lab-02 ret2libc](lab-02-ret2libc/) | 泄露 GOT → 算 libc 基址 → ROP 调 system | ROP gadget、栈对齐（movaps 坑）、两段式利用 |
| [lab-03 UAF](lab-03-uaf/) | 悬垂指针 → 复用 chunk → 劫持函数指针 | tcache 后进先出、free 不清零 / malloc 不擦除 |
| [lab-04 tcache poisoning](lab-04-tcache/) | 信息泄露 → 篡改 tcache next → 劫持 `__free_hook` | counts 机制、目标须可读、safe-linking 演进、fastbin 老套路为何失效 |

难度递进：01 → 02 → 03 → 04。03 与 04 与 glibc 2.31（Ubuntu 20.04）真实行为逐条对齐，
不是 2.23 时代的老教程复刻——04 的文档里记录了照抄老教程直接 SIGABRT 的实测过程。

## 一键环境

```bash
docker build -t pwn-labs .
docker run --rm -it pwn-labs
# 容器内：
bash run_tests.sh        # 四个 exploit 全自动验证，期望 4 passed
```

国内网络拉取基础镜像慢时，用 `Dockerfile.cn`（换阿里云 apt 源 + 清华 pip 源）：

```bash
docker build -f Dockerfile.cn -t pwn-labs .
```

单个实验：

```bash
cd lab-02-ret2libc && python3 exploit.py
```

## 缓解机制速查

| 机制 | 防什么 | 典型绕法 |
|---|---|---|
| Stack Canary | 覆盖返回地址 | 泄露 canary（格式化字符串/信息泄露） |
| NX | 栈上执行注入代码 | ret2libc / ROP（复用已有代码） |
| ASLR | 地址随机化 | 泄露运行时地址（GOT/堆指针） |
| PIE | 代码段随机化 | 先泄露代码段，再取 gadget |
| RELRO | GOT 劫持 | Full RELRO 下改打其他目标（如 hook、栈） |
| tcache key (2.29+) | tcache double free | 走 fastbin（先撑满 tcache）或破坏 key |
| safe-linking (2.32+) | tcache fd 篡改 | 堆地址泄露后异或还原 |
| hook 移除 (2.34+) | `__free_hook` 类劫持 | FSOP / exit handlers / 栈写入 |

## 环境标定与诚实声明

- 全部 exploit 按 **Ubuntu 20.04 / glibc 2.31 / x86-64** 标定，Docker 镜像内置同环境。
- `run_tests.sh` 是自动化验证脚本：每个 exploit 拿到 shell 并产生命令回显才算 PASS。
- 各 lab README 的「验证状态」行记录真实测试结果；未验证的会明确写「待验证」，
  不存在「应该能跑通」式的模糊表述。

## 面向的学习者

已理解 C 语言指针/函数调用约定、正在学习二进制安全的学习者。
配套知识：x86-64 调用约定（rdi/rsi/rdx 传参）、ELF 基础、glibc 堆管理器结构。

## 许可

MIT
