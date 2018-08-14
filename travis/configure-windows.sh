#!/bin/bash
set -e
#set -o xtrace

export LTSMIN_NUM_CPUS=2
export LD_LIBRARY_PATH="$HOME/ltsmin-deps/lib"

# set correct compiler
export BUILD_HOST="x86_64-w64-mingw32.static"

export MCRL2_LIB_DIR=""

. travis/configure-generic.sh "--host=$BUILD_HOST $@"

set +e

