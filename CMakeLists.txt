# Library Project: cypheri
cmake_minimum_required(VERSION 3.24)

project(cypheri VERSION 1.0.0 LANGUAGES CXX)

set(
    CYPHERI_SRCS
    src/token.cpp
	src/nametable.cpp
	src/errors.cpp
	src/parse.cpp
	src/bytecode.cpp
)

# Define _CRT_SECURE_NO_WARNINGS
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

add_library(cypheri ${CYPHERI_SRCS})
target_compile_features(cypheri PRIVATE cxx_std_20)
target_include_directories(cypheri PUBLIC include)

# Test Executables: cypheri_test_*
add_executable(cypheri_test_tokenize tests/test_tokenize.cpp)
target_link_libraries(cypheri_test_tokenize PRIVATE cypheri)
target_include_directories(cypheri_test_tokenize PRIVATE include)
target_compile_features(cypheri_test_tokenize PRIVATE cxx_std_20)

add_executable(cypheri_test_parse tests/test_parse.cpp)
target_link_libraries(cypheri_test_parse PRIVATE cypheri)
target_include_directories(cypheri_test_parse PRIVATE include)
target_compile_features(cypheri_test_parse PRIVATE cxx_std_20)

