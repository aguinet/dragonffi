#!/bin/bash
#

if [ $# -ne 1 ]; then
  echo "Usage: $0 pyversion" 1>&2
  echo "  where pyversion is cp27-cp27mu... (see in /opt/python in the container)" 1>&2
  exit 1
fi

set -ex

DIR=$(dirname $(realpath $0))
DOCKER_IMAGE_NAME=dffi
GID=$(id -g)

sudo docker run --rm -v /etc/group:/etc/group:ro -v /etc/passwd:/etc/passwd:ro -u=$UID:$GID --tmpfs /home/current:size=100M,uid=$UID,gid=$GID -v $(readlink -f "$DIR/../.."):/home/current/src $DOCKER_IMAGE_NAME /bin/sh -c \
  "cd /home/current/src/bindings/python && LLVM_CONFIG=/opt/llvm/llvm-5.0.1.src/build/bin/llvm-config /opt/python/$1/bin/python ./setup.py bdist_wheel"
