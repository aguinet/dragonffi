#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

if [ ! -z ${PYTHON_EXECUTABLE+x} ]; then
  PYTHON_EXECUTABLE="-DPYTHON_EXECUTABLE=$(which $PYTHON_EXECUTABLE)"
fi

mkdir build
cd build
cmake -DPYTHON_BINDINGS=ON -DPYTHON_VERSION=$PYTHON_VERSION -DBUILD_TESTS=OFF -DLLVM_CONFIG=../$LLVM/bin/llvm-config -DCMAKE_BUILD_TYPE=debug $PYTHON_EXECUTABLE -G Ninja ..
ninja
ninja check
