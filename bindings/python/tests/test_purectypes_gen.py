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

import pydffi
import unittest
import struct

from common import GenCTypesTest

class GenTest(GenCTypesTest):
    def test_basic(self):
        FFI = pydffi.FFI()
        Obj = FFI.UIntTy(10)
        for T in self.generated_types(FFI.UIntTy, "__UIntTy"):
            V = self.purectypes.unpack(T, bytes(pydffi.view_as_bytes(Obj)))
            self.assertEqual(V, 10)

    def test_struct(self):
        FFI = pydffi.FFI()
        CU = FFI.cdef('''
typedef struct {
  unsigned char a;
  int b;
  int c;
  short d;
} A;
''')
        Obj = CU.types.A(a=1,b=2,c=10,d=20)
        for A in self.generated_types(CU.types.A, "A"):
            V = self.purectypes.unpack(A, bytes(pydffi.view_as_bytes(Obj)))
            self.assertEqual(V.a, 1)
            self.assertEqual(V.b, 2)
            self.assertEqual(V.c, 10)
            self.assertEqual(V.d, 20)
            V = self.purectypes.pack(A, V)
            V = pydffi.view_as(CU.types.A, V)
            self.assertEqual(Obj.a, V.a)
            self.assertEqual(Obj.b, V.b)
            self.assertEqual(Obj.c, V.c)
            self.assertEqual(Obj.d, V.d)

    def test_union(self):
        FFI = pydffi.FFI()
        CU = FFI.cdef('''
#include <stdint.h>
typedef union {
  struct {
    uint8_t v8[4];
  };
  uint32_t v32;
} IP;
''')
        Obj = CU.types.IP()
        Obj.v32 = 0xAABBCCDD
        for IP in self.generated_types(CU.types.IP, "IP"):
            V = self.purectypes.unpack(IP, bytes(pydffi.view_as_bytes(Obj)))
            self.assertEqual(V.v32, 0xAABBCCDD)
            for i in range(4):
                self.assertEqual(V.v8[i], Obj.v8[i])
            V = self.purectypes.pack(IP, V)
            V = pydffi.view_as(pydffi.const(CU.types.IP), V)
            self.assertEqual(V.v32, 0xAABBCCDD)

    def test_recursive_struct(self):
        FFI = pydffi.FFI()
        CU = FFI.cdef('''
typedef struct {
  unsigned char a;
  int b;
  int c;
  short d;
} A;

typedef struct _Node Node;

struct _Node {
    A v;
    struct _Node* next;
};
''')
        A = CU.types.A
        Node = CU.types.Node
        A0 = A(a=0,b=1,c=10,d=20)
        A1 = A(a=1,b=2,c=-10,d=-20)
        N1 = Node(v=A1,next=pydffi.ptr(Node)())
        N0 = Node(v=A0,next=pydffi.ptr(N1))

        for T in self.generated_types(Node, "_Node"):
            V = self.purectypes.unpack(T, bytes(pydffi.view_as_bytes(N0)))
            for attr in ("a","b","c","d"):
                self.assertEqual(getattr(V.v, attr), getattr(N0.v, attr))
            self.assertEqual(V.next, int(N0.next))

    def test_recursive_union(self):
        FFI = pydffi.FFI()
        CU = FFI.cdef('''
typedef struct {
  unsigned char a;
  int b;
  int c;
  short d;
} A;

typedef union _Node Node;

union _Node {
    A v;
    Node* next;
};
''')
        A = CU.types.A
        Node = CU.types.Node
        A0 = A(a=0,b=1,c=10,d=20)
        A1 = A(a=1,b=2,c=-10,d=-20)
        N1 = Node()
        N1.next = pydffi.ptr(Node)()
        N0 = Node()
        N0.next = pydffi.ptr(N1)

        for T in self.generated_types(Node, "_Node"):
            V = self.purectypes.unpack(T, bytes(pydffi.view_as_bytes(N0)))
            self.assertEqual(V.next, int(N0.next))

if __name__ == "__main__":
    unittest.main()
