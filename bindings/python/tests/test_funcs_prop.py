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

import unittest
import pydffi

from common import DFFITest

class FuncsPropTest(DFFITest):
    def test_funcs_prop(self):
        D = self.FFI
        CU = D.compile('''
#include <stdio.h>
#include <stdbool.h>

int foo() { return 42; }

struct A {
    int a;
};
__attribute__((ms_abi)) bool verify(struct A a, int ref) {
    return a.a == ref;
}

bool verify_int(int a, int ref) { return a == ref; }
        ''')
        self.assertEqual(CU.funcs.foo().value, 42)

        a = CU.types.A(a=15)
        self.assertTrue(getattr(CU.funcs, "verify")(a, 15))
        self.assertTrue(CU.funcs.verify_int(D.Int32(42), 42))

if __name__ == '__main__':
    unittest.main()
