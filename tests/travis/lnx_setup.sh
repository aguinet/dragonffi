#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

get_llvm lnx$ARCH 2b839e9e16d9c3c8b3f5573dc8b108c65c12597e69377684e67f3ef2a4f17aa6
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
