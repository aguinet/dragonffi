#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

get_llvm lnx$ARCH 291ae264965240493f0d5f2e154ab7dbfcf2c7de23291ba415822594ca6cfecb
configure_pip

if [ ! -z $ARCH ]; then
  case $ARCH in
    aarch64)
      DEB_ARCH=arm64
      ;;
    *)
      DEB_ARCH=$ARCH
      ;;
  esac
  sudo dpkg-architecture --add $DEB_ARCH
  sudo apt-get update
  sudo apt-get install libstdc++-dev:$DEB_ARCH libxml2-dev:$DEB_ARCH binutils-$ARCH-linux-gnu
fi
