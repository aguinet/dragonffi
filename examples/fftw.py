import math
import pydffi

pydffi.dlopen("/usr/lib/x86_64-linux-gnu/libfftw3.so")
FFI = pydffi.FFI()
FFT = FFI.cdef("#include <fftw3.h>")

# Adapted from https://github.com/undees/fftw-example/blob/master/fftw_example.c
NUM_POINTS = 64
fftw_complex = FFT.types.fftw_complex
signal = FFI.arrayType(fftw_complex, NUM_POINTS)();
result = FFI.arrayType(fftw_complex, NUM_POINTS)();

def acquire_from_somewhere(signal):
    for i in range(NUM_POINTS):
        theta = float(i) / float(NUM_POINTS) * math.pi;

        signal[i][0] = 1.0 * math.cos(10.0 * theta) + \
                          0.5 * math.cos(25.0 * theta);

        signal[i][1] = 1.0 * math.sin(10.0 * theta) + \
                          0.5 * math.sin(25.0 * theta);

def do_something_with(result):
    for i in range(NUM_POINTS):
        mag = math.sqrt(result[i][0] * result[i][0] + \
                      result[i][1] * result[i][1]);
        print("%0.4f" % mag);


plan = FFT.funcs.fftw_plan_dft_1d(NUM_POINTS, signal, result, -1, 1<<6)
acquire_from_somewhere(signal)
FFT.funcs.fftw_execute(plan)
do_something_with(result)
