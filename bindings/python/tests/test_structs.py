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
import struct
import sys
import unittest

from common import DFFITest

class StructsTest(DFFITest):
    def test_structs(self):
        FFI=self.FFI
        CU = FFI.compile('''
struct A
{
  unsigned char a;
  short b;
};

int check(struct A a, unsigned char aref, short bref) {
  return (a.a == aref) && (a.b == bref);
}
int checkptr(struct A* a, unsigned char aref, short bref) {
  return (a->a == aref) && (a->b == bref);
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
        self.assertEqual(fields_name[0], 'a')
        self.assertEqual(fields_name[1], 'b')

        Av = CU.types.A(a=1,b=2)

        # Direct data access throught a memoryview
        mv = pydffi.view_as_bytes(Av)

        # Set a throught mv

        v = 5 if sys.version_info >= (3, 0) else struct.pack("B", 5)
        mv[0] = v
        self.assertEqual(Av.a, 5)
        self.assertEqual(Av.b, 2)
        self.assertTrue(getattr(CU.funcs, "check")(Av, 5, 2))

        pAv = pydffi.ptr(Av)
        self.assertTrue(CU.funcs.checkptr(pAv, 5, 2))
        CU.funcs.set(pAv)
        self.assertTrue(getattr(CU.funcs, "check")(Av, 59, 1111))
        self.assertEqual(Av.a, 59)
        self.assertEqual(Av.b, 1111)

        Av = CU.funcs.init()
        self.assertEqual(Av.a, 44)
        self.assertEqual(Av.b, 5555)

if __name__ == '__main__':
    unittest.main()
