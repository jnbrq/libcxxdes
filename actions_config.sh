#!/bin/bash

export CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$MATRIX_BUILD_TYPE"

# if [[ $MATRIX_OS =~ ubuntu-(.+) ]]
if [[ $MATRIX_OS =~ ubuntu-22.04 ]]
then
    if [[ $MATRIX_COMPILER =~ clang-([0-9]+) ]]
    then
        version=${BASH_REMATCH[1]}
        export CC=clang-$version
        export CXX=clang++-$version
        export CMAKE_ARGS="$CMAKE_ARGS -DCXXDES_SANITIZE_UNDEFINED=OFF"
    elif [[ $MATRIX_COMPILER =~ g\+\+-([0-9]+) ]]
    then
        version=${BASH_REMATCH[1]}
        export CC=gcc-$version
        export CXX=g++-$version
        export CMAKE_ARGS="$CMAKE_ARGS"
    else
        echo "error: not supported compiler."
        exit 1
    fi
else
    echo "error: not supported OS."
    exit 1
fi
