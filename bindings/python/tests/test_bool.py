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
import random

import pydffi
from common import DFFITest

class BoolTest(DFFITest):
    def test_bool(self):
        CU = self.FFI.compile('''
#include <stdbool.h>
bool foo(int a) { return a==1; }
bool invert(const bool v) { return !v; }
        ''')

        self.assertTrue(CU.funcs.foo(1))
        self.assertFalse(CU.funcs.foo(0))
        self.assertFalse(CU.funcs.invert(True))
        self.assertTrue(CU.funcs.invert(False))

        b = CU.funcs.foo(1)
        self.assertTrue(b)
        self.assertFalse(not b)
        self.assertFalse(b & False)
        self.assertTrue(b & True)

if __name__ == '__main__':
    unittest.main()
