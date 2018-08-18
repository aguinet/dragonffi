# Copyright 2018 Adrien Guinet <adrien@guinet.me>
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# RUN: "%python" "%s" | "%FileCheck" "%s"
#

import pydffi
import random

FFI=pydffi.FFI()

# Integer casts
for Ty in (FFI.UInt8, FFI.UInt16, FFI.UInt32, FFI.UInt64):
    v = random.getrandbits(pydffi.sizeof(Ty(0))*8)
    V = Ty(v)
    for CTy in (FFI.UInt8Ty, FFI.UInt16Ty, FFI.UInt32Ty, FFI.UInt64Ty):
        VC = pydffi.cast(V,CTy)
        assert(VC.value == (v & (2**(CTy.size*8)-1)))

# Array/pointer casts
CU = FFI.compile('''
#include <stdio.h>
typedef struct {
    char buf[256];
} A;

void print(const char* msg) {
    puts(msg);
}

void print_struct(A const* a) {
    print(a->buf);
}
''')
print_struct = CU.funcs.print_struct

SA = CU.types.A
A = pydffi.CStructObj(SA)
mem = pydffi.view_as_bytes(A)
b = b"hello!\x00"
mem[:len(b)] = b

# CHECK: hello!
print_struct(pydffi.ptr(A))

print_ = getattr(CU.funcs, "print")
buf = A.buf
buf = pydffi.cast(pydffi.ptr(buf),FFI.Int8PtrTy)
# CHECK: hello!
print_(buf)

# Cast back
buf = pydffi.cast(buf, FFI.pointerType(SA))
# CHECK: hello!
print_struct(buf)
