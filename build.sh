#!/bin/bash

# Define versions for g++ and libraries
GXX_VERSION="11"

build_app() {
    g++-"${GXX_VERSION}" -v -std=c++17 -o shannon main.cpp
}

build_tests() {
    g++-"${GXX_VERSION}" -v -std=c++17 -DTESTING -o tests test/test.cpp main.cpp \
        -lgtest -lgtest_main -lpthread
}
pwd
case "$1" in
    "test") build_tests ;;
    *) build_app ;;
esac
