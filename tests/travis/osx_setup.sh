#!/bin/bash
#

DIR=$(dirname $(realpath $0))
. "$DIR/common.sh"

#configure_pip
LLVM_FILE=llvm.tar.xz
echo "b3ad93c3d69dfd528df9c5bb1a434367babb8f3baea47fbb99bf49f1b03c94ca  $LLVM_FILE" >$LLVM_FILE.sha256
wget http://releases.llvm.org/7.0.0/clang+llvm-7.0.0-x86_64-apple-darwin.tar.xz -O $LLVM_FILE && shasum -a256 -c $LLVM_FILE.sha256
mkdir llvm
tar xf $LLVM_FILE --strip-components 1 -C llvm


brew install ccache ninja
#if [ "${PYTHON_VERSION:0:1}" == "3" ]; then
#  brew upgrade python3
#fi
