#!/bin/bash
set -e
#set -o xtrace

export LTSMIN_LDFLAGS="-static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -Wl,-lwinpthread -lmman -Wl,--no-whole-archive"
export LTSMIN_CFLAGS=""
export LTSMIN_CXXFLAGS=""
export STRIP_FLAGS="-s"
export MCRL2_LIB_DIR=""
export STRIP="x86_64-w64-mingw32.static-strip"

. travis/build-release-generic.sh

set +e

