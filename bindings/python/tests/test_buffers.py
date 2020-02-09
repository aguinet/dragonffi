# -*- coding: utf8 -*-
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

import random
import unittest

import pydffi
from common import DFFITest

class BuffersTest(DFFITest):
    def test_buffers(self):
        CU = self.FFI.compile('''
#include <stdio.h>
#include <stdint.h>

void print(const char* msg) {
    puts(msg);
}
void print_u8(uint8_t const* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        printf("%02X ", buf[i]);
    }
    printf("\\n");
} 

void bytesupper(uint8_t* S) {
    const size_t Len = strlen(S);
    printf("%lu\\n", Len);
    for (size_t i = 0; i < Len; ++i) {
        S[i] = toupper(S[i]);
    }
}

void strupper(char* S) {
    bytesupper(S);
}
        ''')
        print_ = getattr(CU.funcs, "print")
        print_u8 = CU.funcs.print_u8
        bytesupper = CU.funcs.bytesupper
        strupper = CU.funcs.strupper

        # CHECK: coucou
        print_("coucou")
        # CHECK: héllo 
        print_("héllo")

        buf = u"héllo".encode("utf8")
        # CHECK: 68 C3 A9 6C 6C 6F
        print_u8(buf, len(buf))

        buf = bytearray(b"hello")
        bytesupper(buf)
        self.assertEqual(buf, b"HELLO")

        buf = bytearray(b"hello")
        buf_char = pydffi.view_as(self.FFI.arrayType(self.FFI.UInt8Ty, len(buf)), buf)
        strupper(pydffi.cast(pydffi.ptr(buf_char),self.FFI.CharPtrTy))
        self.assertEqual(buf, b"HELLO")

if __name__ == '__main__':
    unittest.main()
