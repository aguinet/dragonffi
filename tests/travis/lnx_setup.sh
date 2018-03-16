#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

get_llvm lnx$ARCH 291ae264965240493f0d5f2e154ab7dbfcf2c7de23291ba415822594ca6cfecb
configure_pip

if [ ! -z $ARCH ]; then
  sudo dpkg-architecture --add $ARCH
  sudo apt-get update
fi
