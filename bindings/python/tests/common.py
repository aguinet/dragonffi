import pydffi
import unittest
import six

try:
    import purectypes
    purectypes.generators.pydffi
    has_purectypes = True
except (ImportError, AttributeError):
    has_purectypes = False

class DFFITest(unittest.TestCase):
    def setUp(self):
        self.FFI = pydffi.FFI()

    def cstr_from_array(self, ar):
        return pydffi.cast(pydffi.ptr(ar), self.FFI.CharPtrTy).cstr

@unittest.skipIf(not has_purectypes, "purectypes not installed")
class GenCTypesTest(unittest.TestCase):
    def setUp(self):
        self.purectypes = purectypes

    def generated_types(self, T, name):
        G = purectypes.generators.pydffi()
        A = G(T)
        yield A

        # Dump, eval and retry
        D = purectypes.dump(A)
        globals_ = {}
        O = six.exec_(D, globals_)
        yield globals_[name]
