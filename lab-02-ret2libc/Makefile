CC      = gcc
CFLAGS  = -fno-stack-protector -no-pie -O0 -z norelro
# NX 保持开启（栈不可执行）：所以不能往栈上塞 shellcode，
# 只能复用 libc 里已有的代码 —— 这正是 ret2libc 的前提。

all: vuln

vuln: vuln.c
	$(CC) $(CFLAGS) -o vuln vuln.c

clean:
	rm -f vuln
