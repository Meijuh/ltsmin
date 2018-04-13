#!/bin/bash
set -e
#set -o xtrace

TAG_OR_BRANCH=${TRAVIS_TAG:-$TRAVIS_BRANCH}

. travis/configure-$TRAVIS_OS_NAME.sh --disable-doxygen-doc \
    "--prefix=/tmp/$TAG_OR_BRANCH --enable-pkgconf-static" "$@"

export MAKEFLAGS=-j2

# make sure we compile LTSmin with a patched Boost
export CPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH:$TRAVIS_BUILD_DIR/travis/include-fix"

make LDFLAGS="-flto -O3 $LTSMIN_LDFLAGS" CFLAGS="-flto -O3 $LTSMIN_CFLAGS" \
    CPPFLAGS="-DNDEBUG" CXXFLAGS="-flto -O3 $LTSMIN_CXXFLAGS"
make install

# install DiVinE so that it is included in the distribution
. travis/install-DiVinE.sh

strip "$STRIP_FLAGS" /tmp/$TAG_OR_BRANCH/bin/* || true
cp "$HOME/ltsmin-deps/bin/divine" "/tmp/$TAG_OR_BRANCH/bin"
cp "$HOME/ltsmin-deps$MCRL2_LIB_DIR/bin/txt2lps" "/tmp/$TAG_OR_BRANCH/bin"
cp "$HOME/ltsmin-deps$MCRL2_LIB_DIR/bin/txt2pbes" "/tmp/$TAG_OR_BRANCH/bin"
export LTSMIN_DISTNAME="ltsmin-$TAG_OR_BRANCH-$TRAVIS_OS_NAME"
pushd /tmp
tar cfz "$LTSMIN_DISTNAME.tgz" "$TAG_OR_BRANCH"
popd

set +e

