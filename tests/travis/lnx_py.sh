#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

mkdir build
cd build
cmake -DPYTHON_BINDINGS=ON -DPYTHON_VERSION=$PYTHON_VERSION -DBUILD_TESTS=OFF -DLLVM_CONFIG=../$LLVM/bin/llvm-config -DCMAKE_BUILD_TYPE=debug -G Ninja ..
ninja
ninja check
