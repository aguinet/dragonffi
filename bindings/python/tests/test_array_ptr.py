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

from common import DFFITest

class ArrayPtrTest(DFFITest):
    def test_array_ptr(self):
        CU = self.FFI.compile('''
#include <stdio.h>
typedef int int2[2];
void foo(char* buf, size_t n, int2* ar)
{
    snprintf(buf, n, "%d %d", ar[0][0], ar[0][1]);
}
        ''')

        v = CU.types.int2()
        v.set(0, 1)
        v.set(1, 2)
        buf = self.FFI.arrayType(self.FFI.CharTy, 128)()
        CU.funcs.foo(pydffi.ptr(buf), 128, pydffi.ptr(v))
        self.assertEqual(self.cstr_from_array(buf), b"1 2")

        v[0] = 10
        v[1] = 20
        CU.funcs.foo(pydffi.ptr(buf), 128, pydffi.ptr(v))
        self.assertEqual(self.cstr_from_array(buf), b"10 20")

if __name__ == '__main__':
    unittest.main()
