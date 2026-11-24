FROM ubuntu:20.04

# glibc 2.31 环境：四个 lab 的 exploit 全部按此版本标定
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential libc6-dev gdb file vim-common \
        python3 python3-pip && \
    python3 -m pip install --no-cache-dir pwntools && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /labs
COPY . /labs

# 预编译全部 lab
RUN make -C lab-01-ret2text && make -C lab-02-ret2libc && \
    make -C lab-03-uaf && make -C lab-04-tcache

CMD ["/bin/bash"]
