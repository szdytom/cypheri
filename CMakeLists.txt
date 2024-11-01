# Library Project: cypheri
cmake_minimum_required(VERSION 3.24)

project(cypheri VERSION 1.0.0 LANGUAGES CXX)

set(
    CYPHERI_SRCS
    src/token.cpp
	src/nametable.cpp
	src/errors.cpp
)

# Define _CRT_SECURE_NO_WARNINGS
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

add_library(cypheri ${CYPHERI_SRCS})
target_compile_features(cypheri PRIVATE cxx_std_20)
target_include_directories(cypheri PUBLIC include)

# Test CLI
add_executable(cypheri_test tests/test_tokenize.cpp)
target_link_libraries(cypheri_test PRIVATE cypheri)
target_include_directories(cypheri_test PRIVATE include)
target_compile_features(cypheri_test PRIVATE cxx_std_20)
