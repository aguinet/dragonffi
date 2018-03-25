#!/bin/bash
#

DIR=$(dirname $(readlink -f $0))
. "$DIR/common.sh"

case $ARCH in
  aarch64)
    LLVM_HASH=d2a9ecc958857655360823cf3d41480ff5c91dc00aa941ea5af1bebbaaf5d5fd
    ;;
  *)
    echo "Unsupported arch $ARCH!" 1>&2
    exit 1
    ;;
esac

# Get LLVM
get_llvm lnx$ARCH $LLVM_HASH

# Get multiarch docker

