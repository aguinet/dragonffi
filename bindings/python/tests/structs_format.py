# coding: utf-8
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

# RUN: "%python" "%s"

import pydffi
import struct
import codecs

FFI = pydffi.FFI()
CU = FFI.cdef('''
#include <stdbool.h>
typedef struct {
    bool a;
    unsigned int b;
    unsigned short c;
} A;

typedef struct {
    unsigned char buf[9];
    unsigned short v0;
    A a;
    unsigned short v1;
} B;
''')

vA = CU.types.A(a=1,b=0xAAAAAAAA,c=0x4444)
buf = pydffi.view_as_bytes(vA)
vAup = struct.unpack(CU.types.A.format, buf)
assert(vAup == (1,0xAAAAAAAA,0x4444))

buf_ref = bytearray(b"012345678")
vB = CU.types.B(v0=1,v1=2,a=vA,buf=pydffi.view_as(CU.types.B.buf.type, buf_ref))
buf = pydffi.view_as_bytes(vB)
vBup = struct.unpack(CU.types.B.format, buf)
assert(bytearray(vBup[:9]) == buf_ref)
assert(vBup[9:] == (1,1,0xAAAAAAAA,0x4444,2))
