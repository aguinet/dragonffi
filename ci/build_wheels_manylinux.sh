#!/bin/bash
# Usage: script.sh /path/to/dragonffi /path/to/llvm python_versions

set -ex

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
. "$SCRIPT_DIR/install_cmake_manylinux.sh"

# Use own cmake instead of the system one
export PATH=$(dirname "$CMAKE_BIN"):$PATH

DFFI=$1
shift
LLVM=$1
shift

LLVM=$(readlink -f "$LLVM")
export LLVM_CONFIG="$LLVM/bin/llvm-config"
export MAKEFLAGS="-j$(nproc)"

cd "$DFFI/bindings/python"
for PY in $*; do
  /opt/python/$PY/bin/python ./setup.py bdist_wheel
done
for f in dist/*.whl; do
  auditwheel repair $f
done
