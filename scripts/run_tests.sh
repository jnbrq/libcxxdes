#!/bin/bash

# silent pushd
function pushd() {
    command pushd "$@" > /dev/null
}

# silent popd
function popd() {
    command popd "$@" > /dev/null
}

function print_var() {
    if [ "$#" -eq 1 ]
    then
        VAR_NAME="$1"
        eval "echo \"$VAR_NAME=\\\"\$$VAR_NAME\\\"\""
    fi
}

function success_msg() {
    if [ "$#" -eq 1 ]
    then
        echo "[SUCCESS] $1"
    else
        echo "[SUCCESS]"
    fi
}

function fail_msg() {
    if [ "$#" -eq 1 ]
    then
        echo "[FAIL] $1"
    else
        echo "[FAIL]"
    fi

    exit 1
}

echo "Running tests..."

GENERATOR=${GENERATOR:-Ninja}
BUILD_CMD=${BUILD_CMD:-ninja}

print_var "GENERATOR"
print_var "BUILD_CMD"

echo "==="

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
SOURCE_DIR="$SCRIPT_DIR/.."
BUILD_DIR=${BUILD_DIR:-"$SCRIPT_DIR/build"}

for cfg in "$@"
do
    cfg_basename=$(basename "$cfg" .cfg)
    cfg_dirname=$(dirname "$cfg")
    pushd "$cfg_dirname"
        source "$cfg_basename.cfg"
    popd

    echo "Configuration: $cfg_basename"
    print_var "CXX"
    print_var "CC"
    print_var "CMAKE_ARGS"

    mkdir -p "build/$cfg_basename"
    pushd "build/$cfg_basename"
        cmake $SOURCE_DIR -G Ninja $CMAKE_ARGS 2>cmake_err.log 1>cmake_out.log && success_msg "cmake" || fail_msg "cmake"
        "$BUILD_CMD" 2>build_err.log 1>build_out.log && success_msg "build" || fail_msg "build"
        ctest 2>ctest_err.log 1>ctest_out.log && success_msg "ctest" || "ctest"
    popd
    
    echo "== DONE =="
done
