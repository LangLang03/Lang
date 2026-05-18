cmake_minimum_required(VERSION 4.0)

project(TortureLang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TORTURE_BUILD_TESTS "Build Torture tests" ON)

set(TORTURE_BYTECODE_FORMAT_VERSION 3)
string(SHA256 TORTURE_BUILD_ENVIRONMENT_BINDING
    "${CMAKE_SOURCE_DIR}|${CMAKE_BINARY_DIR}|${CMAKE_CXX_COMPILER}|${CMAKE_CXX_COMPILER_ID}|${CMAKE_CXX_COMPILER_VERSION}|${CMAKE_SYSTEM_NAME}|${CMAKE_SYSTEM_PROCESSOR}|${CMAKE_CXX_STANDARD}"
)

add_library(torture_common
    src/common/diagnostic.cpp
    src/common/source.cpp
    src/common/sha256.cpp
)
target_include_directories(torture_common PUBLIC src)

add_library(torture_vm
    src/vm/bytecode_binary.cc
    src/vm/bytecode.cpp
    src/vm/environment.cpp
    src/vm/opcode.cc
    src/vm/vm.cpp
)
target_include_directories(torture_vm PUBLIC src)
target_compile_definitions(torture_vm PUBLIC
    TORTURE_BYTECODE_FORMAT_VERSION=${TORTURE_BYTECODE_FORMAT_VERSION}
    "TORTURE_BUILD_COMPILER_ID=\"${CMAKE_CXX_COMPILER_ID}\""
    "TORTURE_BUILD_COMPILER_VERSION=\"${CMAKE_CXX_COMPILER_VERSION}\""
    "TORTURE_BUILD_SYSTEM_NAME=\"${CMAKE_SYSTEM_NAME}\""
    "TORTURE_BUILD_SYSTEM_PROCESSOR=\"${CMAKE_SYSTEM_PROCESSOR}\""
    "TORTURE_BUILD_CONFIGURATION=\"$<CONFIG>\""
    "TORTURE_BUILD_ENVIRONMENT_BINDING=\"${TORTURE_BUILD_ENVIRONMENT_BINDING}\""
)
target_link_libraries(torture_vm PUBLIC torture_common)

add_library(torture_compiler
    src/compiler/ast.cpp
    src/compiler/bytecode_compiler.cpp
    src/compiler/indentation.cpp
    src/compiler/lexer.cpp
    src/compiler/parser.cpp
    src/compiler/sema.cpp
)
target_include_directories(torture_compiler PUBLIC src)
target_link_libraries(torture_compiler PUBLIC torture_common torture_vm)

add_executable(torturec apps/torturec/main.cpp)
target_link_libraries(torturec PRIVATE torture_compiler)

add_executable(torturevm apps/torturevm/main.cpp)
target_link_libraries(torturevm PRIVATE torture_vm)

if(TORTURE_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.1
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

    enable_testing()

    add_executable(torture_tests
        tests/test_lexer.cpp
        tests/test_parser_sema.cpp
        tests/test_vm_smoke.cpp
    )
    target_link_libraries(torture_tests PRIVATE torture_compiler Catch2::Catch2WithMain)

    include(CTest)
    include(Catch)
    catch_discover_tests(torture_tests)
endif()
