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

class MDArrayTest(DFFITest):
    def test_mdarray(self):
        N = 10
        FFI = self.FFI
        ArTy = FFI.arrayType(FFI.DoubleTy, N)
        Ar = ArTy()
        for i in range(len(Ar)):
            Ar[i] = i
        Ref = [float(i) for i in range(len(Ar))]
        self.assertEqual(list(Ar), Ref)
        ArV = memoryview(Ar)
        self.assertEqual(ArV.format, "d")
        def upck_double(v):
            if isinstance(v,float):
                return v
            return struct.unpack("d",v)[0]
        self.assertTrue(all(upck_double(a) == b for a,b in zip(ArV,Ref)))

        Ar2Ty = FFI.arrayType(FFI.arrayType(FFI.DoubleTy, 10), 2)
        Ar = Ar2Ty()
        ArV = memoryview(Ar)
        self.assertEqual(ArV.format, "d")
        self.assertEqual(ArV.ndim, 2)
        self.assertEqual(ArV.shape, (2,10))
        self.assertEqual(ArV.strides, (80,8))

        try:
            import numpy as np
        except ImportError:
            return

        ar = np.ndarray(N)
        car = pydffi.view_as(ArTy, ar)
        self.assertTrue(all(a == b for a,b in zip(ar,car)))

if __name__ == '__main__':
    unittest.main()
