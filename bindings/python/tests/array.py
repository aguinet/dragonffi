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
import sys
import struct

FFI=pydffi.FFI()
CU=FFI.compile('''
#include <stdio.h>
#include <string.h>

struct A
{
  char buf[128];
};

struct A init() {
  struct A a;
  strcpy(a.buf, "hello");
  return a;
}

void dump(struct A const* v) {
  puts(v->buf);
}
''')
A = CU.funcs.init()
Buf = A.buf
assert(Buf.get(0) == "h")
Buf.set(0, 'H')
assert(Buf.get(0) == "H")
# CHECK: Hello
CU.funcs.dump(FFI.ptr(A))

m = memoryview(Buf)
v = ord("Z") if sys.version_info >= (3, 0) else struct.pack("B", ord("Z"))
m[0] = v
# CHECK: Zello
CU.funcs.dump(FFI.ptr(A))

UIntTy = FFI.basicType(pydffi.BasicKind.UInt)
N = 10
ArrTy = FFI.arrayType(UIntTy, N)
Arr=pydffi.CArrayObj(ArrTy)
for i in range(N):
    Arr.set(i, i)
for i in range(N):
    assert(Arr.get(i) == i)

m = memoryview(Arr)
for i in range(N):
    v = m[i]
    if sys.version_info[0] < 3:
        v = struct.unpack("I", v)[0]
    assert(v == i)
