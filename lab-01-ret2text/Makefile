CC      = gcc
CFLAGS  = -fno-stack-protector -no-pie -O0 -z norelro
# -fno-stack-protector : 关闭 canary，栈溢出不会触发 __stack_chk_fail
# -no-pie              : 关闭 PIE，代码段地址固定，backdoor 地址可直接硬编码
# -z norelro           : 关闭 RELRO（本 lab 不依赖，统一教学口径）
# 未加 -z execstack：本 lab 跳转到已有代码，不需要栈可执行（NX 保持开启）

all: vuln

vuln: vuln.c
	$(CC) $(CFLAGS) -o vuln vuln.c

clean:
	rm -f vuln
