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

class CXXTest(DFFITest):
    def __init__(self, *args, **kwargs):
        super(CXXTest, self).__init__(*args, **kwargs)
        self.options = {'CXX': pydffi.CXXMode.Std11}

    def test_cxx(self):
        FFI = self.FFI
        CU = FFI.compile('''
template <class T>
static T foo(T a, T b) { return a+b; }
extern "C" int foo_int(int a, int b) { return foo(a,b); }
''')
        self.assertEqual(CU.funcs.foo_int(4,5).value, 9)

if __name__ == '__main__':
    unittest.main()
