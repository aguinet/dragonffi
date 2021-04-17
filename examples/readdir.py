# REQUIRES: linux
# RUN: %python "%s" "%S" |%FileCheck "%s"

# CHECK: readdir.py

import pydffi
import sys

D = pydffi.FFI()
CU = D.cdef("#include <dirent.h>")

dir_ = CU.funcs.opendir(sys.argv[1])
if not dir_:
    print("error reading directory")
    sys.exit(1)
readdir = CU.funcs.readdir
while True:
    dirent = readdir(dir_)
    if not dirent:
        break
    name = dirent.obj.d_name
    name = pydffi.cast(pydffi.ptr(name), D.CharPtrTy)
    print(name.cstr.tobytes())
CU.funcs.closedir(dir_)
