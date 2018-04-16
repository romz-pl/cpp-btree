cmake_minimum_required (VERSION 3.0)
project (cpp-btree)

SET( ROMZ_CXX_FLAGS -Wall -pedantic -pthread -g -O0 -fprofile-arcs -ftest-coverage -std=c++11 )

include(CTest)
enable_testing(true)

add_subdirectory(3rdparty/gflags)
add_subdirectory(3rdparty/googletest)

# add_subdirectory(tests/bench)
add_subdirectory(tests/btree)
add_subdirectory(tests/safe_btree)

# add_subdirectory(tests)
