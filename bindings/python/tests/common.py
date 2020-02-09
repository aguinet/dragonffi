import pydffi
import unittest

class DFFITest(unittest.TestCase):
    def setUp(self):
        self.FFI = pydffi.FFI()

    def cstr_from_array(self, ar):
        return pydffi.cast(pydffi.ptr(ar), self.FFI.CharPtrTy).cstr
