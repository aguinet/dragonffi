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

import unittest
import pydffi
import struct
import sys

from common import DFFITest

class ConstnessTest(DFFITest):
    def test_constness(self):
        FFI = self.FFI
        CU = FFI.compile('''
static int ar[] = {0,1,2,3,4};
const int* foo() {
    return ar;
}

struct A {
    int v;
};

struct A ga = {1};

const struct A* bar() { return &ga; }
        ''')

        ar = CU.funcs.foo()
        ar = ar.viewObjects(5)
        def upck_int(v):
            if isinstance(v,int):
                return v
            return struct.unpack("i",v)[0]
        self.assertTrue(all(upck_int(v) == i for i,v in enumerate(ar)))

        with self.assertRaises(Exception):
            ar[0] = 1 if sys.version_info >= (3, 0) else struct.pack("B", 1)

        ga = CU.funcs.bar()
        with self.assertRaises(Exception):
            ga.obj.v = 2

        self.assertTrue(pydffi.cast(ga.obj, CU.types.A) is None)

if __name__ == '__main__':
    unittest.main()
