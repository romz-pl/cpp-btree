cmake_minimum_required (VERSION 3.0)
project (cpp-btree)

SET( ROMZDB_CXX_FLAGS -Wall -pedantic -pthread -g -O0 -fprofile-arcs -ftest-coverage -std=c++11 )

include(CTest)
enable_testing(true)

add_subdirectory(3rdparty/gflags)
add_subdirectory(3rdparty/googletest)

add_subdirectory(src)

# add_subdirectory(tests)
