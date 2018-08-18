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

# RUN: "%python" "%s"

import pydffi
import sys
import struct

F = pydffi.FFI()
CU = F.compile('''
#include <stdio.h>
struct A {
  unsigned int a;
  unsigned long long b;
};

void print(struct A a) { printf("%u %lu\\n", a.a, a.b); }
''')

S = pydffi.view_as(pydffi.const(CU.types.A), b"A"*16)
assert(int(S.a) == 0x41414141)
assert(int(S.b) == 0x4141414141414141)
B = bytearray(pydffi.view_as_bytes(S))
assert(B == b"A"*16)

B = pydffi.view_as_bytes(S)
One = 1 if sys.version_info >= (3, 0) else struct.pack("B", 1)
B[0] = One
B[1] = One
B[2] = One
B[3] = One
assert(int(S.a) == 0x01010101)
