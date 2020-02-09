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

from common import DFFITest

class StructsPortableFormatTest(DFFITest):
    def test_structs_portable_format(self):
        CU = self.FFI.cdef('''
#include <stdlib.h>
#include <stdbool.h>
typedef struct {
    bool valid;
    void* a;
    unsigned short len;
    size_t v;
} A;
        ''')

        a = CU.types.A(valid=1,len=0xBBAA,v=0xDDCCBBAA)
        av = pydffi.view_as_bytes(a)
        self.assertEqual(struct.unpack(CU.types.A.format, av), struct.unpack(CU.types.A.portable_format, av))

if __name__ == '__main__':
    unittest.main()
