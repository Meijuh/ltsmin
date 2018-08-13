#!/bin/bash
set -e
#set -o xtrace

# set correct compiler
export BUILD_HOST="x86_64-w64-mingw32"
export CC="x86_64-w64-mingw32-gcc"
export CXX="x86_64-w64-mingw32-g++-posix"

export POPT_VERSION="1.16"
export POPT_NAME="popt-$POPT_VERSION"
export POPT_URL="http://rpm5.org/files/popt/$POPT_NAME.tar.gz"

export ZLIB_VERSION="1.2.11"
export ZLIB_NAME="zlib-$ZLIB_VERSION"
export ZLIB_URL="https://zlib.net/$ZLIB_NAME.tar.gz"

export MMAN_URL="https://github.com/witwall/mman-win32/archive/master.zip"

# install popt from source
if [ ! -f "$HOME/ltsmin-deps/lib/libpopt.a" ]; then
    wget "$POPT_URL" -P /tmp
    tar xf "/tmp/$POPT_NAME.tar.gz" -C /tmp
    pushd "/tmp/$POPT_NAME"
    wget https://raw.githubusercontent.com/mxe/mxe/master/src/popt-1-win32.patch
    patch < popt-1-win32.patch
    ./configure --disable-dependency-tracking --enable-static --disable-shared --prefix="$HOME/ltsmin-deps" --host="$BUILD_HOST"
    make install
    popd
fi

# install zlib from source
if [ ! -f "$HOME/ltsmin-deps/lib/libz.a" ]; then
    wget "$ZLIB_URL" -P /tmp
    tar xf "/tmp/$ZLIB_NAME.tar.gz" -C /tmp
    pushd "/tmp/$ZLIB_NAME"
    ./configure --static --prefix="$HOME/ltsmin-deps"
    make -f win32/Makefile.gcc PREFIX="$BUILD_HOST-"
    make install
    popd
fi

# install mman from source
if [ ! -f "$HOME/ltsmin-deps/lib/libmman.a" ]; then
    wget "$MMAN_URL" -P /tmp
    pushd /tmp
    unzip "master.zip"
    pushd "mman-win32-master"
    ./configure --prefix="$HOME/ltsmin-deps" --cross-prefix="$BUILD_HOST-" --bindir="$HOME/ltsmin-deps/bin" --libdir="$HOME/ltsmin-deps/lib" --incdir="$HOME/ltsmin-deps/include" --disable-shared --enable-static
    make
    make install
    popd
    popd
fi

travis/install-generic.sh

set +e
