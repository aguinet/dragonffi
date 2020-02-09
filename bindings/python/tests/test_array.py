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

import pydffi
import sys
import struct
import unittest

from common import DFFITest

class ArrayTest(DFFITest):
    def test_array(self):
        CU=self.FFI.compile('''
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

struct A
{
  char buf[128];
};

struct A init() {
  struct A a;
  strcpy(a.buf, "hello");
  return a;
}

bool verify(struct A const* v, const char* ref) {
  return strcmp(v->buf, ref) == 0;
}
        ''')
        A = CU.funcs.init()
        Buf = A.buf
        self.assertEqual(Buf.get(0), "h")
        Buf.set(0, 'H')
        self.assertEqual(Buf.get(0), "H")
        self.assertTrue(CU.funcs.verify(pydffi.ptr(A), b"Hello"))

        m = pydffi.view_as_bytes(Buf)
        v = ord("Z") if sys.version_info >= (3, 0) else struct.pack("B", ord("Z"))
        m[0] = v
        self.assertTrue(CU.funcs.verify(pydffi.ptr(A), b"Zello"))

        UIntTy = self.FFI.basicType(pydffi.BasicKind.UInt)
        N = 10
        ArrTy = self.FFI.arrayType(UIntTy, N)
        Arr=pydffi.CArrayObj(ArrTy)
        for i in range(N):
            Arr.set(i, i)
        for i in range(N):
            self.assertEqual(Arr.get(i), i)

        # TOFIX!
        #m = pydffi.view_as_bytes(Arr)
        #for i in range(N):
        #    v = m[i]
        #    if sys.version_info[0] < 3:
        #        v = struct.unpack("I", v)[0]
        #    assert(v == i)

if __name__ == '__main__':
    unittest.main()
