#!/bin/bash

BASEPATH="${BASH_SOURCE[0]}"
BASEPATH=$(dirname $BASEPATH)
BASEPATH=$(realpath $BASEPATH)
echo BASEPATH=$BASEPATH

BUILDDIR=$(realpath .)/build
echo BUILDDIR=$BUILDDIR

PARAMS=""
PARAMS="$PARAMS -DCMAKE_BUILD_TYPE=RelWithDebInfo"
PARAMS="$PARAMS -DCMAKE_INSTALL_PREFIX=$BUILDDIR"
echo PARAMS=$PARAMS

if [ -d build ]; then
    make -C build clean
else
    mkdir build
fi

cd build
cmake -G"Unix Makefiles" $PARAMS $BASEPATH
make && make install

