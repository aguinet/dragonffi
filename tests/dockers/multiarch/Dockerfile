# manylinux1 docker
FROM debian:stretch

RUN apt-get update && apt-get install -y wget gnupg && \
    echo "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-6.0 main" >/etc/apt/sources.list.d/llvm.list && \
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    dpkg --add-architecture arm64 && apt-get update && \
    apt-get update && apt-get install -y xz-utils patch build-essential cmake clang-6.0 libpython-dev libpython3-dev g++ libstdc++-6-dev:arm64 ninja-build binutils-aarch64-linux-gnu qemu-user python-pip && \
    pip install lit
