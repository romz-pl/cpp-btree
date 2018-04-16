set( TEST_NAME btree )

add_executable( ${TEST_NAME}
    btree_test.cpp
    ../common/btree_test_flags.cpp
    main.cpp
)

target_compile_options( ${TEST_NAME} PRIVATE ${ROMZ_CXX_FLAGS} )

target_include_directories( ${TEST_NAME} PRIVATE 
    ${gtest_SOURCE_DIR}/include 
    ${gtest_SOURCE_DIR}
    ${gflags_BINARY_DIR}/include 
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries( ${TEST_NAME} 
    -fprofile-arcs 
    gflags
    gtest 
    gtest_main 
    pthread
)


add_test( NAME ${TEST_NAME} COMMAND ${TEST_NAME} )

#set_tests_properties ( ${TEST_NAME} PROPERTIES DEPENDS test-hf )

