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
import platform

from common import DFFITest

class MultipleCUTest(DFFITest):
    def test_mutiplecu(self):
        if platform.system() == "Windows":
            return

        J = self.FFI
        CU = J.cdef('''
        struct A {
          short a;
          int b;
        };

        short get_a(struct A a);
        int get_b(struct A b);
        ''', '/lib.h')

        CUA = J.compile('''
        #include "/lib.h"
        short get_a(struct A a) { return a.a; }
        ''')

        CUB = J.compile('''
        #include "/lib.h"
        int get_b(struct A a) { return a.b; }
        ''')

        # Explicitly delete J. Everything should still works!
        del J

        SA = CU.types.A
        A = CU.types.A(a=1,b=2)
        self.assertEqual(int(CU.funcs.get_a(A)), 1)
        self.assertEqual(int(CU.funcs.get_b(A)), 2)

        # TODO: assert struct A types are all equal between various CU. This is not the
        # case for now, and so these tests fail!
        #assert(CUA.getFunction("get_a").call(A).value == 1)
        #assert(CUB.getFunction("get_b").call(A).value == 2)

if __name__ == '__main__':
    unittest.main()
