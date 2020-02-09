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

class NewStructsTest(DFFITest):
    def test_new_structs(self):
        D = self.FFI
        CU = D.compile('''
        #include <stdio.h>
        struct A {
            int a;
            short b;
        };
        void print(char* ret, size_t n, struct A a) {
            snprintf(ret, n, "%d %d", a.a, a.b);
        }
        ''')

        buf = self.FFI.arrayType(self.FFI.CharTy, 128)()
        a = CU.types.A(a=4,b=10)
        getattr(CU.funcs, "print")(pydffi.ptr(buf), 128, a)
        self.assertEqual(self.cstr_from_array(buf), b"4 10")

        a = CU.types.A(a=0,b=0)
        getattr(CU.funcs, "print")(pydffi.ptr(buf), 128, a)
        self.assertEqual(self.cstr_from_array(buf), b"0 0")

if __name__ == '__main__':
    unittest.main()
