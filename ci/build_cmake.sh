#!/bin/bash
#

set -ex

CMAKE_VERSION="3.18.3"
CMAKE_FILE="cmake-${CMAKE_VERSION}.tar.gz"
cd /tmp
curl -sSL https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/${CMAKE_FILE} -O
echo "2c89f4e30af4914fd6fb5d00f863629812ada848eee4e2d29ec7e456d7fa32e5  ${CMAKE_FILE}" >cmake.sha
sha256sum -c cmake.sha
tar -xf "${CMAKE_FILE}"
cd cmake*
INST_DIR="/pwd/cmake-${CMAKE_VERSION}"
./bootstrap --prefix=$INST_DIR --parallel=$(nproc) -- -DCMAKE_USE_OPENSSL=OFF 
make install -j$(nproc)
