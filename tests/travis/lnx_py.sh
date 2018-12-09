#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

if [ ! -z ${PYTHON_EXECUTABLE+x} ]; then
  PYTHON_EXECUTABLE="-DPYTHON_EXECUTABLE=$(which $PYTHON_EXECUTABLE)"
fi

mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=ON -DPYTHON_BINDINGS=ON -DPYTHON_VERSION=$PYTHON_VERSION -DBUILD_TESTS=ON -DLLVM_CONFIG=$LNX_LLVM_CONFIG $PYTHON_EXECUTABLE -DCMAKE_BUILD_TYPE=debug -G Ninja .. -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
ninja
lit -v tests
lit -v bindings/python/tests
