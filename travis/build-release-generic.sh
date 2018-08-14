#!/bin/bash
set -e
set -o xtrace

export LDFLAGS="-flto -O3 $LTSMIN_LDFLAGS"

. travis/configure-$BUILD_TARGET.sh --disable-doxygen-doc \
    "--prefix=/tmp/$TAG_OR_BRANCH --enable-pkgconf-static"  "$@"

unset LDFLAGS

export MAKEFLAGS=-j2

# make sure we compile LTSmin with a patched Boost
export CPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH:$TRAVIS_BUILD_DIR/travis/include-fix"

make CFLAGS="-flto -O3 $LTSMIN_CFLAGS" \
    CPPFLAGS="-DNDEBUG" CXXFLAGS="-flto -O3 $LTSMIN_CXXFLAGS"
make install

if [ "$BUILD_TARGET" != "windows" ]; then
    # install DiVinE so that it is included in the distribution
    . travis/install-DiVinE.sh
fi

$STRIP "$STRIP_FLAGS" /tmp/$TAG_OR_BRANCH/bin/* || true
if [ "$BUILD_TARGET" != "windows" ]; then
    cp "$HOME/ltsmin-deps/bin/divine" "/tmp/$TAG_OR_BRANCH/bin"
    cp "$HOME/ltsmin-deps$MCRL2_LIB_DIR/bin/txt2lps" "/tmp/$TAG_OR_BRANCH/bin"
    cp "$HOME/ltsmin-deps$MCRL2_LIB_DIR/bin/txt2pbes" "/tmp/$TAG_OR_BRANCH/bin"
fi

pushd /tmp
tar cfz "$LTSMIN_DISTNAME.tgz" "$TAG_OR_BRANCH"
popd

set +e

