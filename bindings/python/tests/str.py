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

# RUN: "%python" "%s" | "%FileCheck" "%s"

import pydffi
import random

J=pydffi.FFI()

CU = J.compile('''
#include <stdio.h>
void print(const char* msg) {
    puts(msg);
}
const char* get_str() { return "hello"; }
''')
print_ = CU.getFunction("print")
get_str = CU.getFunction("get_str")

# CHECK: coucou
print_.call("coucou")
# CHECK: héllo 
print_.call("héllo")

assert((get_str().cstr).tobytes() == b"hello")
