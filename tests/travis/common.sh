CLANG_DEB=clang-7
LNX_LLVM_CONFIG=/usr/bin/llvm-config-7

set -ex

function configure_pip {
  if [ -z $VIRTUAL_ENV ]; then
    USER="--user"
  fi
  pip install $USER lit
}
