purectypes generator
====================

DragonFFI can generate `purectypes <https://github.com/aguinet/purectypes>`_
types from any C type. The main use case for this is to be able to parse and
generate C structures for a given ABI in a portable way. For instance, you
could generate the `purectypes <https://github.com/aguinet/purectypes>`_ version
of the `DXGI_ADAPTER_DESC3
<https://docs.microsoft.com/en-us/windows/win32/api/dxgi1_6/ns-dxgi1_6-dxgi_adapter_desc3>`_
DirectX structure, and then parse a blob of data that represents this structure under any OS.

To do such a thing, we first need to generate the ``purectypes``-related code
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


`purectypes <https://github.com/aguinet/purectypes>`_ is a pure Python module,
and does not depend on ``DragonFFI`` per se.
