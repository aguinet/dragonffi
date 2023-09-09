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

class CastTest(DFFITest):
    def test_cast(self):
        FFI = self.FFI
        # Integer casts
        for Ty in (FFI.UInt8, FFI.UInt16, FFI.UInt32, FFI.UInt64):
            v = random.getrandbits(pydffi.sizeof(Ty(0))*8)
            V = Ty(v)
            for CTy in (FFI.UInt8Ty, FFI.UInt16Ty, FFI.UInt32Ty, FFI.UInt64Ty):
                VC = pydffi.cast(V,CTy)
                self.assertEqual(VC.value, v & (2**(CTy.size*8)-1))

        # Array/pointer casts
        CU = FFI.compile('''
#include <string.h>
#include <stdbool.h>

typedef struct {
    char buf[256];
} A;

bool verify(const char* msg, const char* ref) {
    return strcmp(msg, ref) == 0;
}

bool verify_struct(A const* a, const char* ref) {
    return strcmp(a->buf, ref) == 0;
}
        ''')
        verify_struct = CU.funcs.verify_struct

        SA = CU.types.A
        A = pydffi.CStructObj(SA)
        mem = pydffi.view_as_bytes(A)
        b = b"hello!\x00"
        mem[:len(b)] = b

        self.assertTrue(verify_struct(pydffi.ptr(A), b"hello!"))

        verify = getattr(CU.funcs, "verify")
        buf = A.buf
        buf = pydffi.cast(pydffi.ptr(buf),FFI.Int8PtrTy)
        self.assertTrue(verify(buf, b"hello!"))

        # Cast back
        buf = pydffi.cast(buf, FFI.pointerType(SA))
        self.assertTrue(verify_struct(buf, b"hello!"))

if __name__ == '__main__':
    unittest.main()
