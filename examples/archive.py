import pydffi
import sys

pydffi.dlopen("/usr/lib/x86_64-linux-gnu/libarchive.so")
D = pydffi.FFI()
CU=D.cdef('''
#include <archive.h>
#include <archive_entry.h>
''')

funcs = CU.funcs
archive_read_next_header = funcs.archive_read_next_header
archive_entry_pathname_utf8 = funcs.archive_entry_pathname_utf8
archive_read_data_skip = funcs.archive_read_data_skip

a = funcs.archive_read_new()
funcs.archive_read_support_filter_all(a)
funcs.archive_read_support_format_all(a)
r = funcs.archive_read_open_filename(a, sys.argv[1], 10240)
if r != 0:
    raise RuntimeError("unable to open archive")
entry = pydffi.ptr(CU.types.archive_entry)()
while archive_read_next_header(a, pydffi.ptr(entry)) == 0:
    pathname = archive_entry_pathname_utf8(entry)
    print(pathname.cstr.tobytes().decode("utf8"))
    archive_read_data_skip(a)
funcs.archive_read_free(a)
