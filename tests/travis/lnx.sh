#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

if [ -z $ARCH ]; then
  ARCH="x86_64"
fi

mkdir build
cd build
cmake -DPYTHON_BINDINGS=OFF -DLLVM_CONFIG=../$LLVM/bin/llvm-config -DCMAKE_BUILD_TYPE=debug -G Ninja -DCMAKE_C_FLAGS="-target $ARCH-linux-gnu" -DCMAKE_CXX_FLAGS="-target $ARCH-linux-gnu" -DCMAKE_C_COMPILER="clang-6.0" -DCMAKE_CXX_COMPILER="clang++-6.0" ..
ninja
ninja check
