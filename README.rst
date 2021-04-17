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

Supported OSes/architectures, with `Python wheels precompiled and uploaded to
PyPI <https://pypi.org/project/pydffi/#files>`_:

* Linux i386/x64, with bindings for Python 2/3
* Linux/AArch64. with bindings for Python 3
* OSX x64, with bindings for Python 2/3
* Windows x64, with bindings for Python 3

Documentation
=============

* `Why another FFI? <docs/intro.rst>`_
* `Installation <docs/install.rst>`_
* `Building from source <docs/building.rst>`_
* `purectypes generator <docs/purectypes.rst>`_

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

  FFI = pydffi.FFI(CXX=pydffi.CXXMode.Std17)
  CU = FFI.compile('''
  template <class T>
  static T foo(T a, T b) { return a+b; }
  extern "C" int foo_int(int a, int b) { return foo(a,b); }
  ''')
  CU.funcs.foo_int(4,5)


More advanced usage examples are provided in the examples directory.


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
