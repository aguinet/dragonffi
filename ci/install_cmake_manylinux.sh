#!/bin/bash
#

set -ex

CMAKE_VERSION="3.18.3"
CMAKE_FILE="cmake-${CMAKE_VERSION}-Linux-$(uname -m).tar.gz"
cd /tmp
if [ $(uname -m) == "x86_64" ]; then
  URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}"
  echo "eb23bac95873cc0bc3946eed5d1aea6876ac03f7c0dcc6ad3a05462354b08228  cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz" >cmake.sha
else
  URL="https://files.geekou.info"
  echo "ae54e46bcd5a5bc81be330c4ebeabbf3c0c09fd775d28e394a58a18aba52492a  cmake-${CMAKE_VERSION}-Linux-i686.tar.gz" >cmake.sha
fi
curl -sSL $URL/${CMAKE_FILE} -O
sha256sum -c cmake.sha
tar -C /opt -xf "${CMAKE_FILE}"
ln -s /opt/cmake* /opt/cmake
rm "${CMAKE_FILE}" cmake.sha
