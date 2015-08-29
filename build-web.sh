#!/bin/bash

BASEPATH="${BASH_SOURCE[0]}"
BASEPATH=$(dirname $BASEPATH)
BASEPATH=$(realpath $BASEPATH)
echo BASEPATH=$BASEPATH

BUILDDIR=$(realpath .)/build-web
echo BUILDDIR=$BUILDDIR

PARAMS=""
PARAMS="$PARAMS -DCMAKE_BUILD_TYPE=Release"
PARAMS="$PARAMS -DCMAKE_INSTALL_PREFIX=$BUILDDIR"
echo PARAMS=$PARAMS

if [ -d build-web ]; then
    make -C build-web clean
else
    mkdir build-web
fi

cd build-web
cmake -DCMAKE_TOOLCHAIN_FILE="$BASEPATH/CMakeModules/Emscripten.cmake" -G"Unix Makefiles" $PARAMS $BASEPATH
make && make install

