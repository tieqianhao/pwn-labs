CC      = gcc
CFLAGS  = -fno-stack-protector -no-pie -O0 -z norelro
# 堆攻击不依赖这些开关，统一教学口径。

all: vuln

vuln: vuln.c
	$(CC) $(CFLAGS) -o vuln vuln.c

clean:
	rm -f vuln
