cmake_minimum_required(VERSION 3.18)

set(THREAD_SAFE_TEST_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/thread_safe_test.cpp)
add_executable(THREAD_SAFE_TEST ${THREAD_SAFE_TEST_SRCS})
target_include_directories(THREAD_SAFE_TEST PUBLIC ${LRUCACHE_PATH})
target_compile_features(THREAD_SAFE_TEST PUBLIC cxx_std_17)