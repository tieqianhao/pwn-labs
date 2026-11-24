CC      = gcc
CFLAGS  = -fno-stack-protector -no-pie -O0 -z norelro
# 堆 lab 不依赖栈保护设置，统一教学口径即可。

all: vuln

vuln: vuln.c
	$(CC) $(CFLAGS) -o vuln vuln.c

clean:
	rm -f vuln
