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

# -*- coding: utf8 -*-
# RUN: "%python" "%s" | "%FileCheck" "%s"

import pydffi
import random

J=pydffi.FFI()

CU = J.compile('''
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
print_ = CU.getFunction("print")
print_u8 = CU.getFunction("print_u8")
bytesupper = CU.getFunction("bytesupper")
strupper = CU.getFunction("strupper")

# CHECK: coucou
print_.call("coucou")
# CHECK: héllo 
print_.call("héllo")

buf = u"héllo".encode("utf8")
# CHECK: 68 C3 A9 6C 6C 6F
print_u8.call(buf, len(buf))

buf = bytearray(b"hello")
# CHECK: 5
bytesupper.call(buf)
assert(buf == b"HELLO")

buf = bytearray(b"hello")
buf_char = J.view(buf)
# CHECK: 5
strupper.call(buf_char.cast(J.CharPtrTy))
assert(buf == b"HELLO")
