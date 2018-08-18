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
import struct
import sys

FFI=pydffi.FFI()
CU = FFI.compile('''
int printf(const char* f, ...);

struct A
{
  unsigned char a;
  short b;
};

void print(struct A a) {
  printf("a=%d, b=%d\\n", a.a, a.b);
}
void printptr(struct A* a) {
  printf("a=%d, b=%d\\n", a->a, a->b);
}
void set(struct A* a) {
  a->a = 59;
  a->b = 1111;
}
struct A init() {
  struct A ret;
  ret.a = 44;
  ret.b = 5555;
  return ret;
}
''')
A = CU.types.A
fields_name = sorted((f.name for f in A))
assert(fields_name[0] == 'a')
assert(fields_name[1] == 'b')

Av = CU.types.A(a=1,b=2)

# Direct data access throught a memoryview
mv = pydffi.view_as_bytes(Av)

# Set a throught mv

v = 5 if sys.version_info >= (3, 0) else struct.pack("B", 5)
mv[0] = v
assert(Av.a == 5)
assert(Av.b == 2)
# CHECK: a=5, b=2
getattr(CU.funcs, "print")(Av)

pAv = pydffi.ptr(Av)
# CHECK: a=5, b=2
CU.funcs.printptr(pAv)
CU.funcs.set(pAv)
# CHECK: a=59, b=1111
getattr(CU.funcs, "print")(Av)
assert(Av.a == 59)
assert(Av.b == 1111)

Av = CU.funcs.init()
assert(Av.a == 44)
assert(Av.b == 5555)
