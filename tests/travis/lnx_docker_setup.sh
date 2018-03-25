#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

case $ARCH in
  aarch64)
    LLVM_HASH=c0d2a71a596d6712451ff28f60b5d6a68276b7da2b4959ca209a3461a0e86f81
    ;;
  *)
    echo "Unsupported arch $ARCH!" 1>&2
    exit 1
    ;;
esac

# Get LLVM
get_llvm lnx$ARCH $LLVM_HASH

# Build multiarch docker
sudo apt-get update
sudo apt-get -y -o Dpkg::Options::="--force-confnew" install docker-ce

"$DIR/../dockers/multiarch/build.sh"
