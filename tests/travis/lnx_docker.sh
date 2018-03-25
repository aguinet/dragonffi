#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

sudo docker run --privileged --rm -v "$PWD:/opt/pwd" dffi_multiarch /bin/bash -c "cd /tmp && mkdir build && cd build && \
  mount binfmt_misc -t binfmt_misc /proc/sys/fs/binfmt_misc && \
  cmake -DPYTHON_BINDINGS=OFF -DLLVM_CONFIG=/opt/pwd/$LLVM/bin/llvm-config -DCMAKE_BUILD_TYPE=debug -G Ninja -DCMAKE_C_COMPILER=clang-6.0 -DCMAKE_CXX_COMPILER=clang++-6.0 -DCMAKE_C_FLAGS=\"-target $ARCH-linux-gnu\" -DCMAKE_CXX_FLAGS=\"-target $ARCH-linux-gnu\" /opt/pwd/ && \
  ninja && \
  ninja check"
