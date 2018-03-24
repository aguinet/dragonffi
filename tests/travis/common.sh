LLVM=llvm-5.0.1.dffi

set -ex

function get_llvm {
  OS=$1
  SHA256=$2
  if [ ! -f "$LLVM/bin/llvm-config" ]; then
    LLVM_FILE=$LLVM.$OS.tar.bz2
    # Get a precompiled and patched version of LLVM
    echo "$2  $LLVM_FILE" >${LLVM_FILE}.sha256
    wget http://files.geekou.info/$LLVM_FILE && shasum -a256 -c ${LLVM_FILE}.sha256
    tar xf ${LLVM_FILE}
    rm ${LLVM_FILE}
  fi
}

function configure_pip {
  if [ ! -z $ARCH ]; then
    SUDO="sudo"
  fi
  $SUDO pip install --upgrade pip
  if [ -z $VIRTUAL_ENV ]; then
    USER="--user"
  fi
  $SUDO pip install $USER lit
}
