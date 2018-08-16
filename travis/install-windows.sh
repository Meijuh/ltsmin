#!/bin/bash
set -e
#set -o xtrace

# set correct compiler
export BUILD_HOST="x86_64-w64-mingw32.static"

echo "deb http://pkg.mxe.cc/repos/apt/debian wheezy main" \
    | sudo tee /etc/apt/sources.list.d/mxeapt.list
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 \
    --recv-keys D43A795B73B16ABE9643FE1AFD8FFF16DB45C6AB

sudo apt-get update

sudo apt-get -yq --no-install-suggests --no-install-recommends install wine \
    mxe-x86-64-w64-mingw32.static-zlib \
    mxe-x86-64-w64-mingw32.static-popt \
    mxe-x86-64-w64-mingw32.static-mman-win32

export PATH=/usr/lib/mxe/usr/bin:$PATH

travis/install-generic.sh

set +e
