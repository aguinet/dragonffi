# manylinux1 docker
FROM quay.io/pypa/manylinux1_x86_64

# Packages
RUN yum install -y xz

# Compile a recent cmake version
ENV CMAKE_ROOT /opt/cmake-3.10.2
RUN cd /opt && wget --no-check-certificate https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz && tar xzf cmake-3.10.2.tar.gz
RUN cd "$CMAKE_ROOT" && ./bootstrap && make -j8 && make install

# Compile a patched version of llvm 5.0.1
ENV LLVM_ROOT /opt/llvm
ENV LLVM_SRC_ROOT /opt/llvm/llvm-5.0.1.src
ENV LLVM_BUILD_DIR $LLVM_SRC_ROOT/build

RUN mkdir -p "$LLVM_ROOT" && mkdir -p "$LLVM_BUILD_DIR"
ADD cc-llvm.patch $LLVM_ROOT/cc-llvm.patch
ADD cc-clang.patch $LLVM_ROOT/cc-clang.patch
RUN cd "$LLVM_ROOT" && wget http://releases.llvm.org/5.0.1/llvm-5.0.1.src.tar.xz && wget http://releases.llvm.org/5.0.1/cfe-5.0.1.src.tar.xz
RUN cd "$LLVM_ROOT" && tar --use-compress-program=xz -xf llvm-5.0.1.src.tar.xz && \
    tar --use-compress-program=xz -xf cfe-5.0.1.src.tar.xz && ln -s "$PWD/cfe-5.0.1.src" "$LLVM_SRC_ROOT/tools/clang" && \
    cd "$LLVM_SRC_ROOT" && patch -p1 <"$LLVM_ROOT/cc-llvm.patch" && \
    cd "$LLVM_SRC_ROOT/tools/clang" && patch -p1 <"$LLVM_ROOT/cc-clang.patch"

RUN cd "$LLVM_BUILD_DIR" && /usr/local/bin/cmake -DPYTHON_EXECUTABLE=/opt/python/cp27-cp27m/bin/python -DCMAKE_BUILD_TYPE=release -DBUILD_SHARED_LIBS="OFF" -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_ENABLE_RTTI=ON -DLLVM_BUILD_TOOLS=ON -DLLVM_ENABLE_TERMINFO=OFF -DLLVM_ENABLE_LIBEDIT=OFF -DLLVM_ENABLE_ZLIB=OFF "$LLVM_SRC_ROOT"
RUN cd "$LLVM_BUILD_DIR" && make -j8
