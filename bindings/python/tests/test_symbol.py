# Copyright 2020 Adrien Guinet <adrien@guinet.me>
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
import sys
import struct

from common import DFFITest

class SymbolTest(DFFITest):
    def test_symbol(self):
        addr = 0xAABBCCDD
        pydffi.addSymbol("my_custom_symbol", 0xAABBCCDD)
        F = self.FFI
        CU = F.cdef('''
        extern void my_custom_symbol(int);
        ''')
        self.assertEqual(pydffi.ptr(CU.funcs.my_custom_symbol).value, addr)

if __name__ == '__main__':
    unittest.main()
