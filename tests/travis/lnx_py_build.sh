#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

export LLVM_CONFIG=$DIR/../../$LLVM/bin/llvm-config
cd $DIR/../../bindings/python &&  python ./setup.py build
