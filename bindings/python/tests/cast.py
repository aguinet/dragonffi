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

J=pydffi.FFI()

# Integer casts
for Ty in (J.UInt8, J.UInt16, J.UInt32, J.UInt64):
    v = random.getrandbits(Ty(0).size()*8)
    V = Ty(v)
    for CTy in (J.UInt8Ty, J.UInt16Ty, J.UInt32Ty, J.UInt64Ty):
        VC = V.cast(CTy)
        assert(VC.value == (v & (2**(CTy.size*8)-1)))

# Array/pointer casts
CU = J.compile('''
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
print_struct = CU.getFunction("print_struct")

SA = CU.getType("A")
A = pydffi.CStructObj(SA)
mem = memoryview(A)
b = b"hello!\x00"
mem[:len(b)] = b

# CHECK: hello!
print_struct.call(J.ptr(A))

print_ = CU.getFunction("print")
buf = A.buf
buf = buf.cast(J.Int8PtrTy)
# CHECK: hello!
print_.call(buf)

# Cast back
buf = buf.cast(J.pointerType(SA))
# CHECK: hello!
print_struct.call(buf)
