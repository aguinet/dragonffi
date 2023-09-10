#!/bin/bash
#

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
. "$SCRIPT_DIR/install_cmake_manylinux.sh"

cd $1

mkdir build
mkdir install
cd build

if [ $(uname -m) == "aarch64" ]; then
  TARGET="AArch64"
  CMAKE_OPTS=-DLLVM_USE_LINKER=lld
else
  TARGET="X86"
fi

$CMAKE_BIN ../llvm/ \
  -DLLVM_BUILD_EXAMPLES=OFF \
  -DCLANG_ENABLE_ARCMT=OFF \
  -DCLANG_ENABLE_STATIC_ANALYZER=OFF \
  -DLLVM_BUILD_TOOLS=ON \
  -DCLANG_BUILD_TOOLS=OFF \
  -DLLVM_INSTALL_UTILS=ON \
  -DLLVM_TARGETS_TO_BUILD=$TARGET \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_ENABLE_BINDINGS=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$PWD/../install \
  -DLLVM_ENABLE_THREADS=OFF \
  -DLLVM_ENABLE_ZLIB=OFF \
  -DLLVM_ENABLE_ZSTD=OFF \
  -DLLVM_ENABLE_TERMINFO=OFF \
  -DPython3_EXECUTABLE=/opt/python/cp38-cp38/bin/python \
  $CMAKE_OPTS

make install -j$(nproc)
