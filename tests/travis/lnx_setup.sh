#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

case $ARCH in
  aarch64)
    DEB_ARCH=arm64
    LLVM_HASH=2b839e9e16d9c3c8b3f5573dc8b108c65c12597e69377684e67f3ef2a4f17aa6
    ;;
  *)
    DEB_ARCH=$ARCH
    LLVM_HASH=291ae264965240493f0d5f2e154ab7dbfcf2c7de23291ba415822594ca6cfecb
    ;;
esac

get_llvm lnx$ARCH $LLVM_HASH
configure_pip

if [ ! -z $ARCH ]; then
  ( cat <<EOF
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ trusty main restricted
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ trusty-updates main restricted
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ trusty universe
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ trusty-updates universe
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ trusty multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports/ trusty-updates multiverse
EOF
  ) |sudo tee /etc/apt/sources.list

  sudo dpkg --add-architecture $DEB_ARCH
  sudo apt-get update
  sudo apt-get install libstdc++-4.9-dev:$DEB_ARCH libxml2-dev:$DEB_ARCH #binutils-$ARCH-linux-gnu
fi
