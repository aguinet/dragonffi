import pydffi
import unittest
import six
import platform
import subprocess

try:
    import purectypes
    purectypes.generators.pydffi
    has_purectypes = True
except (ImportError, AttributeError):
    has_purectypes = False

def getFFI(options = None):
   if options is None:
       options = {}
   if platform.system() == "Darwin":
       options["sysroot"] = subprocess.check_output(["xcrun","--show-sdk-path"]).strip()
   return pydffi.FFI(**options)

class DFFITest(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(DFFITest, self).__init__(*args, **kwargs)
        self.options = {}

    def setUp(self):
        self.FFI = getFFI(self.options)

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
