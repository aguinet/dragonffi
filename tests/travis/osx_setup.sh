#!/bin/bash
#

DIR=$(dirname $(realpath $0))
. "$DIR/common.sh"

get_llvm osx c34f4d9c5634cf93e43ee97ac322702722b5100dfc1c6d7b0a0afb73c6bb5537  
configure_pip

brew install ccache ninja
if [ "${PYTHON_VERSION:0:1}" == "3" ]; then
  brew upgrade python3
fi
