# RUN: "%python" "%s" | "%FileCheck" "%s"
#

import pydffi
from common import getFFI

F = getFFI()
CU = F.cdef('''
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

void print(const char* prefix, ...)
{
  va_list args;
  va_start(args, prefix);
  while (1) {
    int16_t v = va_arg(args, int16_t);
    if (v == 0) break;
    printf("%s: %d\\n", prefix, v);
  }
  va_end(args);
}
''')

print_ = getattr(CU.funcs, "print")
# CHECK: pref: 1
print_("pref", F.Int16Ty(1), F.Int16Ty(0))
# CHECK: pref: -1
print_("pref", F.Int16Ty(-1), F.Int16Ty(0))
# CHECK: pref: 1
# CHECK: pref: -1
print_("pref", F.Int16Ty(1), F.Int16Ty(0))
print_("pref", F.Int16Ty(-1), F.Int16Ty(0))
# CHECK: pref: 1
# CHECK: pref: -1
print_("pref", F.Int16Ty(1), F.Int16Ty(0))
print_("pref", F.Int16Ty(-1), F.Int16Ty(0))
