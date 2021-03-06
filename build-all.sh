#!/bin/bash

#
# Create 'build' directory
#
rm -rf build || exit 1
mkdir build || exit 1
cd build || exit 1

#
# Run cmake
#

cmake -DBUILD_GTEST=ON -DBUILD_GMOCK=OFF -DCMAKE_BUILD_TYPE=Debug -Dbuild_tests=ON .. || exit 1
make -j4 || exit 1


#
# Run test
#
# ctest --verbose
ctest


