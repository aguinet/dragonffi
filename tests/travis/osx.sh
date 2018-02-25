#!/bin/bash
#

DIR=$(dirname $(realpath $0))
. "$DIR/common.sh"

mkdir build
cd build
export PATH=$PATH:$HOME/Library/Python/2.7/bin/
cmake -DPYTHON_BINDINGS=$PYTHON_BINDINGS -DLLVM_CONFIG=../llvm-5.0.1.dffi/bin/llvm-config -DCMAKE_BUILD_TYPE=debug -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -G Ninja ..
ninja
ninja check
