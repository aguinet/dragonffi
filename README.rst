DragonFFI
=========

.. image:: https://travis-ci.org/aguinet/dragonffi.svg?branch=master
    :target: https://travis-ci.org/aguinet/dragonffi

DragonFFI is a C Foreign Function Interface (FFI) library written in C++ and
based on Clang/LLVM. It allows any language to call C functions throught the
provided APIs and bindings.

For now, only python bindings and a C++ API are provided.

Please note that this project is still in alpha stage. Documentation is far
from complete and, although many efforts have been put into it, its APIs aren't
considered stable yet!

Supported OSes/architectures:

* Linux i386/x64, with bindings for python 2/3
* Linux/AArch64. Python bindings are known to work but aren't tested yet with Travis.
* OSX i386/x64, with bindings for python 2/3
* Windows x64, with bindings for python 3

Why another FFI?
================

`libffi <https://sourceware.org/libffi/>`_ is a famous library that provides
FFI for the C language. `cffi <https://cffi.readthedocs.io/en/latest/>`_ are
python bindings around this library that also uses pycparser to be able to
easily declare interfaces and types.

``libffi``  has the issue that it doesn't support recent calling conventions
(for instance the MS x64 ABI under Linux x64 OSes), and every ABI has to be hand written
in assembly. Moreover, ABIs can become really complex (especially for instance when
structure are passed/returned by values).

``cffi`` has the disadvantage of using a C parser that does not support
includes and some function attributes. Thus, using a C library usually means
adapting by hand the library's headers, which isn't always easily maintainable.

``DragonFFI`` is based on Clang/LLVM, and thanks to that is able to get around
these issues:

* it uses Clang to parse header files, allowing direct usage of a C library
  headers without adaptation
* Clang and LLVM allows on-the-fly compilation of C functions
* support as many calling conventions and function attributes as Clang/LLVM do

Moreover, in theory, thanks to the LLVM jitter, it would be possible for every
language bindings to JIT the glue code needed for every function interface, so
that the cost of going from on a language to another could be as small as
possible. This is not yet implemented but an idea for future versions!

Installation
============

Python wheels are provided for Linux. Simply use pip to install the
``pydffi`` package:

.. code:: bash

  $ pip install pydffi

Compilation from source
=======================

This is based on a patched version of LLVM5, which needs to be compiled with RTTI enabled.

LLVM5 compilation
-----------------

.. code:: bash

  $ cd /path/to/llvm
  $ wget http://releases.llvm.org/5.0.1/llvm-5.0.1.src.tar.xz
  $ wget http://releases.llvm.org/5.0.1/cfe-5.0.1.src.tar.xz
  $ tar xf llvm-5.0.1.src.tar.xz && tar xf cfe-5.0.1.src.tar.xz
  $ ln -s $PWD/cfe-5.0.1.src llvm-5.0.1.src/tools/clang
  $ cd llvm-5.0.1.src && patch -p1 </path/to/dragonffi/third-party/cc-llvm.patch && cd -
  $ cd llvm-5.0.1.src/tools/clang && patch -p1 </path/to/dragonffi/third-party/cc-clang.patch && cd -
  $ mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=release -DBUILD_SHARED_LIBS="OFF" -DLLVM_ENABLE_RTTI=ON -DLLVM_BUILD_TOOLS=ON -DLLVM_ENABLE_TERMINFO=OFF -DLLVM_ENABLE_LIBEDIT=OFF -DLLVM_ENABLE_ZLIB=OFF ..
  $ make

DragonFFI compilation
---------------------

After compiling LLVM, DragonFFI can be build:

.. code:: bash

  $ cd /path/to/dragonffi
  $ mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=release -DLLVM_CONFIG=/path/to/llvm/build/bin/llvm-config ..
  $ cd build && make

Usage examples
==============

Let's compile a C function that performs an addition:

.. code:: python
  
  import pydffi

  # First, declare an FFI context
  F = pydffi.FFI()

  # Then, compile a module and get a compilation unit
  CU = F.compile("int add(int a, int b) { return a+b; }")

  # And call the function
  print(int(CU.funcs.add(4, 5)))

The ``compile`` API exposes every defined functions . Declared-only functions won't
be exposed. ``cdef`` can be used for this case, like in this example:

.. code:: python

  import pydffi

  F = pydffi.FFI()
  CU = F.cdef("#include <stdio.h>")
  CU.funcs.puts("hello world!")

Structures can also be used:

.. code:: python

  import pydffi

  F = pydffi.FFI()
  CU = F.compile('''
  #include <stdio.h>
  struct A {
    int a;
    int b;
  };

  void print_struct(struct A a) {
    printf("%d %d\\n", a.a, a.b);
  }
  ''')
  a = CU.types.A(a=1,b=2)
  CU.funcs.print_struct(a)

More advanced usage examples are provided in the examples directory.

Current limitations
===================

Some C features are still not supported by dffi (but will be in future releases):

* C structures with bitfields
* functions with the noreturn attribute
* support for atomic operations

The python bindings also does not support yet:

* proper ``int128_t`` support (need support in pybind11)
* proper memoryview for multi-dimensional arrays

Do not hesitate to report bugs!

Roadmap
=======

See TODO

Related work
============

* `libffi <https://sourceware.org/libffi/>`_
* `cffi <https://cffi.readthedocs.io/en/latest/>`_
* `Skip the FFI: Embedding Clang for C Interoperability (LLVM developer meeting 2014) <https://llvm.org/devmtg/2014-10/#talk18>`_

Contact
=======

* adrien@guinet.me

Authors
=======

* Adrien Guinet (`@adriengnt <https://twitter.com/adriengnt>`_)
