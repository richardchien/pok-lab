FROM ubuntu:16.04

RUN apt-get update && \
    apt-get install -y build-essential mtools qemu qemu-system-x86 libxml-libxml-perl bc && \
    apt-get clean
