#!/bin/bash
#

set -ex

CMAKE_VERSION="3.18.3"
CMAKE_FILE="cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz"
cd /tmp
curl -sSL https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/${CMAKE_FILE} -O
echo "eb23bac95873cc0bc3946eed5d1aea6876ac03f7c0dcc6ad3a05462354b08228  ${CMAKE_FILE}" >cmake.sha
sha256sum -c cmake.sha
tar -C /opt -xf "${CMAKE_FILE}"
ln -s /opt/cmake* /opt/cmake
rm "${CMAKE_FILE}" cmake.sha
