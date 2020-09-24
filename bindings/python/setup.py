# Copyright 2018 Adrien Guinet <adrien@guinet.me>
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import platform
import glob
import os
import subprocess
import sys
import errno

link_args = []
libraries = []
if platform.system() in ("Linux","Darwin"):
    # This will work w/ GCC and clang
    compile_args = ['-std=c++14','-ffunction-sections','-fdata-sections']
    if platform.system() == "Linux":
        # Stripping the library makes us win 20mb..!
        link_args = ["-static-libstdc++","-Wl,--strip-all","-Wl,-gc-sections"]
elif platform.system() == "Windows":
    compile_args = ['/TP', '/EHsc', '/GL-', '/MD']
    libraries = ['Mincore']
else:
    raise RuntimeError("unsupported platform '%s'!" % os.platform)

this_dir =  os.path.dirname(os.path.realpath(__file__))

class build_ext_dffi(build_ext):
    def run(self):
        # Compile dffi with cmake using the LLVM provided by LLVM_CONFIG,
        # creating a full static library with LLVM inside!
        # We then patch the extensions to link with this static library.
        # Inspired by https://stackoverflow.com/questions/42585210/extending-setuptools-extension-to-use-cmake-in-setup-py#
        # TODO: this is the less painfull and integrated setup I managed to do, if
        # someone as a better idea, please let me know :)
        LLVM_CONFIG = os.getenv("LLVM_CONFIG")
        if LLVM_CONFIG is None:
            raise RuntimeError("The LLVM_CONFIG environment variable must be set to a valid path to an llvm-config binary!")

        cwd = os.path.abspath(os.getcwd())

        source_dir = os.path.join(this_dir, "../..")
        build_temp = os.path.abspath(self.build_temp)
        try:
            os.makedirs(build_temp)
        except OSError as e:
            if e.errno != errno.EEXIST or not os.path.isdir(build_temp):
                raise

        os.chdir(build_temp)
        # Use Ninja if it is available
        try:
            subprocess.check_call(['ninja','--version'])
            use_ninja = True
        except:
            use_ninja = False
        cmake_args = ['-DLLVM_CONFIG=%s' % LLVM_CONFIG, "-DCMAKE_BUILD_TYPE=release", "-DDFFI_STATIC_LLVM=ON", "-DPYTHON_BINDINGS=OFF", "-DBUILD_TESTS=OFF", "-DLLVM_TEMPORARILY_ALLOW_OLD_TOOLCHAIN=YES", source_dir]
        if use_ninja:
            cmake_args.extend(("-G","Ninja"))
        system = platform.system()
        if system == "Darwin":
            # Compile for both 32 and 64 bits
            cmake_args.append("-DCMAKE_OSX_ARCHITECTURES='x86_64;i386'")
        elif system == "Windows":
            # We need to force the usage the dynamic version of the C runtime,
            # as we are not compiling a shared library.
            cmake_args.append("-DCMAKE_CXX_FLAGS='/MD'")
        subprocess.check_call(['cmake'] + cmake_args)
        subprocess.check_call(['cmake','--build','.'])
        # Get static library path from cmake
        # TODO: get encoding from the current environment?
        vars_ = subprocess.check_output(['cmake','-L','-N','.']).decode("utf8")
        for v in vars_.split("\n"):
            v = v.strip()
            if v.startswith("DFFI_STATIC_LLVM_PATH:"):
                DFFI_STATIC_LLVM_PATH = v[v.index('=')+1:]
                break
        else:
            raise RuntimeError("unable to get DFFI_STATIC_LLVM_PATH from cmake! This is an internal error, please submit a bug report!")

        for ext in self.extensions:
            ext.include_dirs.append(os.path.join(build_temp, "include"))
            ext.extra_link_args.append(DFFI_STATIC_LLVM_PATH)

        os.chdir(cwd)
        build_ext.run(self)

module = Extension('pydffi.backend',
                    include_dirs = [os.path.join(this_dir, '../../include'),
                        os.path.join(this_dir, '../../third-party')],
                    define_macros = [('dffi_STATIC', None),('PYDFFI_EXT_NAME','backend')],
                    extra_compile_args = compile_args,
                    extra_link_args = link_args,
                    library_dirs = [],
                    libraries = libraries,
                    sources = glob.glob(os.path.join(this_dir, '*.cpp'))) 
setup(name = 'pydffi',
    version = '0.5.1',
    description = 'dragonffi static python bindings',
    author = 'Adrien Guinet',
    author_email = 'adrien@guinet.me',
    long_description = '''
Static python bindings for dragonffi. API isn't yet stable and is subject to change!
''',
    classifiers=[
	'Development Status :: 3 - Alpha',
	'Intended Audience :: Science/Research',
	'Intended Audience :: Developers',
	'Topic :: Software Development :: Build Tools',
	'Topic :: Scientific/Engineering',
	'License :: OSI Approved :: Apache Software License',
	'Programming Language :: Python :: 2',
	'Programming Language :: Python :: 2.7',
	'Programming Language :: Python :: 3',
	'Programming Language :: Python :: 3.4',
	'Programming Language :: Python :: 3.5',
	'Programming Language :: Python :: 3.6',
	'Programming Language :: Python :: 3.7'
    ],
    keywords='ffi clang llvm',
    license='Apache 2.0',
    url = "https://github.com/aguinet/dragonffi",
    download_url = "https://github.com/aguinet/dragonffi/tarball/dffi-0.5.1",
    entry_points = {"purectypes.generators": ".pydffi = pydffi.purectypes_generator:GenPureCType"},
    ext_modules = [module],
    cmdclass={
        'build_ext': build_ext_dffi,
    },
    packages = ["pydffi", "pydffi.purectypes_generator"],
    tests_require = ["six"],
    python_requires='>=2.7, !=3.0.*, !=3.1.*, !=3.2.*, <4'
)
