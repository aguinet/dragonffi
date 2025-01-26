#!/bin/bash
#

set -ex

function cmake_install {
  yum install -y cmake
}

if [ ! -f "$CMAKE_BIN" ]; then
  cmake_install
  export CMAKE_BIN=/usr/bin/cmake
fi
