#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

export LLVM_CONFIG=$LNX_LLVM_CONFIG
cd $DIR/../../bindings/python &&  python ./setup.py build
