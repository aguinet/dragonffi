Building DragonFFI
==================

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

Building DragonFFI (Windows specific instructions)
==================================================

Prerequisites for Windows
-------------------------

This chapter goes through compiling DragonFFI on Windows with Microsoft's build tools. Note that it is possible to 
compile it on Windows with `mingw` although in this case you should follow the Linux how-to.

To complete this walk-through, you must have either installed `Visual Studio <https://visualstudio.microsoft.com/downloads/>`_ 
**and** the optional Visual C++ components, or the `Build Tools for Visual Studio <https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019>`_.

Besides the already aforementioned prerequisites (see _general_prerequ), the following are also required:

* As stated above, a Microsoft compilation chain.
* `7-Zip <https://www.7-zip.org/>`_ (or any program able to extract `.xz` and `.tar` files)

Ensure all the above programs are in your `%PATH%` environment variable.


LLVM and Clang
--------------

## Source installation

Download the required sources archives as mentioned previously. Create a directory that will hosts the LLVM and Clang 
source code.

Extract the `.xz` archive using 7-zip (you'll obtain a .tar file named `llvm-10.0.1.src.tar`):

.. code:: powershell

    PS> 7z.exe x .\llvm-10.0.1.src.tar.xz


Now extract LLVM to a chosen directory (note that the directory **must be a full path**):

.. code:: powershell

    PS> 7z.exe x -oD:\programming\libraries\llvm .\llvm-10.0.1.src.tar


The directory will now look like this:

.. code::

    D:\programming\libraries\llvm\llvm-10.0.1.src


Do the same for clang (the source package is named `cfe`), so you should now have:

.. code:: powershell

    PS D:\programming\libraries\llvm> ls
    Directory: D:\programming\libraries\llvm

    Mode                LastWriteTime         Length Name
    ----                -------------         ------ ----
    d-----       19/07/2019     16:16                cfe-10.0.1.src
    d-----       09/08/2019     14:56                llvm-10.0.1.src


We'll now create a soft symbolic link from the `\llvm\llvm-10.0.1.src\tools\clang` directory to the clang source (note 
that this command requires at least PowerShell 5.0 and *may* require administrator privileges):

.. code:: powershell

    PS C:\WINDOWS\system32> New-Item -ItemType SymbolicLink -Path D:\programming\libraries\llvm\llvm-10.0.1.src\tools\clang -Value D:\programming\libraries\llvm\cfe-10.0.1.src


    Directory: D:\programming\libraries\llvm\llvm-10.0.1.src\tools


    Mode                LastWriteTime         Length Name
    ----                -------------         ------ ----
    d----l       20/09/2019     09:34                clang


Building LLVM
~~~~~~~~~~~~~

Create a `build` directory in the LLVM source:

.. code:: powershell

    PS D:\programming\libraries\llvm> cd D:\programming\libraries\llvm\llvm-10.0.1.src
    PS D:\programming\libraries\llvm\llvm-8.0.1.src> mkdir build


Start a developer command prompt (be sure to pick the right one for your architecture or the architecture your are 
targeting) to build the LLVM source; also ensure that at least `cmake` and `ninja` utilities are in your PATH 
environment variable:

.. code:: cmd

    **********************************************************************
    ** Visual Studio 2019 Developer Command Prompt v16.2.3
    ** Copyright (c) 2019 Microsoft Corporation
    **********************************************************************
    [vcvarsall.bat] Environment initialized for: 'x64'

    D:\programming\Microsoft Visual Studio\2019\Enterprise> cd D:\programming\libraries\llvm\llvm-8.0.1.src\build
    D:\programming\libraries\llvm\llvm-8.0.1.src\build> cmake -DCMAKE_BUILD_TYPE=release -DBUILD_SHARED_LIBS=OFF -DLLVM_BUILD_TOOLS=ON -DLLVM_ENABLE_TERMINFO=OFF -DLLVM_ENABLE_LIBEDIT=OFF -DLLVM_ENABLE_ZLIB=OFF .. -G Ninja
    D:\programming\libraries\llvm\llvm-8.0.1.src\build> ninja


Note that the build will definitely take some time, depending on your machine processing power.

Building DragonFFI
------------------

Compiling
~~~~~~~~~

Once LLVM have been compiled - and still with your developer command prompt opened - clone `DragonFFI`:

.. code:: powershell

    D:\programming\libraries\llvm\llvm-8.0.1.src\build> powershell
    PS D:\programming\libraries\llvm\llvm-8.0.1.src\build> cd k:\projects
    PS k:\projects> git clone https://github.com/aguinet/dragonffi.git
    Cloning into 'dragonffi'...
    ...

Go into the newly cloned `dragonffi` directory then create and go to the `build` directory, generate the `ninja` project
 files and build using `ninja`:

.. code:: powershell

    PS k:\projects>cd dragonffi
    PS k:\projects\dragonffi>mkdir build; cd build
    PS k:\projects\dragonffi\build> cmake -DCMAKE_BUILD_TYPE=release -DLLVM_CONFIG=D:\programming\libraries\llvm\llvm-10.0.1.src\build\bin\llvm-config.exe .. -G Ninja
    PS k:\projects\dragonffi\build> ninja

----

You may also generate the necessary files using the `setup.py` file located in `\bindings\python\`. 

Ensure that you have an environment variable named `LLVM_CONFIG` which points to  the `llvm-config.exe` binary:

.. code:: powershell

    PS k:\projects\dragonffi> Remove-Item .\build -Recurse -ErrorAction Ignore
    PS k:\projects\dragonffi> $env:LLVM_CONFIG="D:\programming\libraries\llvm\llvm-10.0.1.src\build\bin\llvm-config.exe"
    PS k:\projects\dragonffi> mkdir build; cd build
    PS k:\projects\dragonffi\build> python ..\bindings\python\setup.py build

Installing
~~~~~~~~~~

If you wish to install `dragonffi`, just issue the `install` command - using `setup.py` - rather than the `build` one:

.. code:: powershell

    PS k:\projects\dragonffi> python ..\bindings\python\setup.py install

You might want to use a virtual environment if you want to keep your main Python installation clean.
