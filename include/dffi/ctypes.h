#ifndef DFFI_CTYPES
#define DFFI_CTYPES

// Nothing states in the C++ standard that C basic types are the same than C++
// basic types. We thus typedef them using extern "C" here!

extern "C" {
#include <stdbool.h>
typedef bool c_bool;
typedef char c_char;
typedef signed char c_signed_char;
typedef unsigned char c_unsigned_char;
typedef short c_short;
typedef unsigned short c_unsigned_short;
typedef int c_int;
typedef unsigned int c_unsigned_int;
typedef long c_long;
typedef unsigned long c_unsigned_long;
typedef long long c_long_long;
typedef unsigned long long c_unsigned_long_long;
typedef float c_float;
typedef double c_double;
typedef long double c_long_double;
#ifdef DFFI_SUPPORT_COMPLEX
typedef _Complex float c_complex_float;
typedef _Complex double c_complex_double;
typedef _Complex long double c_complex_long_double;
#endif
}

#endif
