.. raw:: html

   <p align="center">
   <img src="https://raw.githubusercontent.com/aguinet/dragonffi/master/logo.jpg" width=200px alt="logo" />
   </p>
   <p align="right"><i>Logo by <a href="https://twitter.com/pwissenlit">@pwissenlit</a></i></p>


DragonFFI
=========

.. image:: https://img.shields.io/gitter/room/gitterHQ/gitter.svg
    :target: https://gitter.im/dragonffi

.. image:: https://github.com/aguinet/dragonffi/workflows/Tests%20Linux/badge.svg?branch=master
    :target: https://github.com/aguinet/dragonffi/actions

.. image:: https://github.com/aguinet/dragonffi/workflows/Tests%20OSX/badge.svg?branch=master
    :target: https://github.com/aguinet/dragonffi/actions

.. image:: https://github.com/aguinet/dragonffi/workflows/Tests%20Windows/badge.svg?branch=master
    :target: https://github.com/aguinet/dragonffi/actions

.. image:: https://github.com/aguinet/dragonffi/workflows/Python%20wheels%20(&%20publish)/badge.svg
    :target: https://github.com/aguinet/dragonffi/actions


DragonFFI is a C Foreign Function Interface (FFI) library written in C++ and
based on Clang/LLVM. It allows any language to call C functions throught the
provided APIs and bindings.

Feel free to join the `Gitter chat <https://gitter.im/dragonffi>`_ for any questions/remarks!

For now, only python bindings and a C++ API are provided.

Please note that this project is still in alpha stage. Documentation is far
from complete and, although many efforts have been put into it, its APIs aren't
considered stable yet!

Supported OSes/architectures, with `Python wheels precompiled and uploaded to
PyPI <https://pypi.org/project/pydffi/#files>`_:

* Linux i386/x64, with bindings for Python 2/3
* Linux/AArch64. with bindings for Python 3
* OSX x64, with bindings for Python 2/3
* Windows x64, with bindings for Python 3

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

LLVM 11 compilation
-------------------

If your system already provides LLVM development package (e.g. on Debian-based
system), you might be able to use them directly. Otherwise, you can compile
Clang/LLVM from sources like this:

.. code:: bash

  $ cd /path/to/llvm
  $ wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.1.0/llvm-11.1.0.src.tar.xz
  $ wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.1.0/clang-11.1.0.src.tar.xz
  $ tar xf llvm-11.1.0.src.tar.xz && tar xf clang-11.1.0.src.tar.xz
  $ ln -s $PWD/clang-11.1.0.src llvm-11.1.0.src/tools/clang
  $ mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=release -DBUILD_SHARED_LIBS=OFF -DLLVM_BUILD_TOOLS=ON -DLLVM_ENABLE_TERMINFO=OFF -DLLVM_ENABLE_LIBEDIT=OFF -DLLVM_ENABLE_ZLIB=OFF ..
  $ make

LLVM development packages
-------------------------

Debian-based system
~~~~~~~~~~~~~~~~~~~

Debian-based system provides development packages for clang & llvm:

.. code:: bash

   $ sudo apt install llvm-11-dev libclang-11-dev llvm-11-tools

The path to ``llvm-config`` can be found with ``which llvm-config-11``, and used directly in the CMake command line below.


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

C++ can be compiled, and used through ``extern C`` functions:

.. code:: python

  import pydffi

  F = pydffi.FFI(CXX=pydffi.CXXMode.Std17)
  CU = FFI.compile('''
  template <class T>
  static T foo(T a, T b) { return a+b; }
  extern "C" int foo_int(int a, int b) { return foo(a,b); }
  ''')
  CU.funcs.foo_int(4,5)


More advanced usage examples are provided in the examples directory.

purectypes generator
====================

DragonFFI can generate `purectypes <https://github.com/aguinet/purectypes>`
types from any C type. The main use case for this is to be able to parse and
generate C structures for a given ABI in a portable way. For instance, you
could generate the `purectypes <https://github.com/aguinet/purectypes>` version
of the `DXGI_ADAPTER_DESC3
<https://docs.microsoft.com/en-us/windows/win32/api/dxgi1_6/ns-dxgi1_6-dxgi_adapter_desc3>`
DirectX structure, and then parse a blob of data that represents this structure under any OS.

To do such a thing, we first need to generate the `purectypes`-related code
under Windows. Let's install the relevant packages:

.. code:: bash

  > pip install purectypes pydffi

And then export our structure using this Python code:

.. code:: python

  import pydffi
  import purectypes

  FFI = pydffi.FFI()
  CU = FFI.cdef("#include <dxgi1_6.h>")
  G = purectypes.generators.pydffi()
  T = G(CU.types.DXGI_ADAPTER_DESC3)
  open("DXGI_ADAPTER_DESC3.py", "w").write(purectypes.dump(T))

We can now import this Python file from any system (for instance under Linux)
and parse/generate such structures. For instance, this code will unpack a bunch
of bytes:

.. code:: python

   import purectypes
   from DXGI_ADAPTER_DESC3 import DXGI_ADAPTER_DESC3

   Data = bytes.fromhex("...")
   Obj = purectypes.unpack(DXGI_ADAPTER_DESC3, Data)

We can for instance modify `Obj` and regenerate the packed structure:

.. code:: python

   Obj.SharedSystemMemory = 0
   Data = purectypes.pack(DXGI_ADAPTER_DESC3, Obj)
   hexdump(Data)


`purectypes <https://github.com/aguinet/purectypes>` is a pure Python module,
and does not depend on `DragonFFI` per se.

Current limitations
===================

Some C features are still not supported by dffi (but will be in future releases):

* C structures with bitfields
* functions with the noreturn attribute
* support for atomic operations

The python bindings also does not support yet:

* proper ``int128_t`` support (need support in pybind11)

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
