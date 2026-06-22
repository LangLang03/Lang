cmake_minimum_required(VERSION 4.0)

project(TortureLang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TORTURE_BUILD_TESTS "Build Torture tests" ON)
option(TORTURE_ENABLE_CLANG_TIDY "启用 clang-tidy 静态检查" OFF)

set(TORTURE_BYTECODE_FORMAT_VERSION 3)

# clang-tidy 集成：在目标创建之前设置 CMAKE_CXX_CLANG_TIDY
if(TORTURE_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_BIN clang-tidy)
    if(CLANG_TIDY_BIN)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_BIN};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
        message(STATUS "clang-tidy 已启用: ${CLANG_TIDY_BIN}")
    else()
        message(WARNING "TORTURE_ENABLE_CLANG_TIDY 已开启但未找到 clang-tidy")
    endif()
endif()

# 平台选择逻辑：根据 CMAKE_SYSTEM_NAME 和 CMAKE_SYSTEM_PROCESSOR 选择目标平台
# 与 src/vm/platform.h 中的预处理器逻辑保持一致
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(TORTURE_PLATFORM_NAME "android_arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
        set(TORTURE_PLATFORM_NAME "android_arm32")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        set(TORTURE_PLATFORM_NAME "android_x64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i486|i586|i686")
        set(TORTURE_PLATFORM_NAME "android_x86")
    else()
        message(FATAL_ERROR "unsupported Android architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
        set(TORTURE_PLATFORM_NAME "windows_arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^ARM")
        set(TORTURE_PLATFORM_NAME "windows_arm32")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
        set(TORTURE_PLATFORM_NAME "windows_x64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i486|i586|i686|x86")
        set(TORTURE_PLATFORM_NAME "windows_x86")
    else()
        message(FATAL_ERROR "unsupported Windows architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(TORTURE_PLATFORM_NAME "linux_arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
        set(TORTURE_PLATFORM_NAME "linux_arm32")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        set(TORTURE_PLATFORM_NAME "linux_x64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i486|i586|i686")
        set(TORTURE_PLATFORM_NAME "linux_x32")
    else()
        message(FATAL_ERROR "unsupported Linux architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
else()
    message(FATAL_ERROR "unsupported target platform: only Android / Windows / Linux are supported")
endif()

message(STATUS "TortureLang target platform: ${TORTURE_PLATFORM_NAME}")

string(SHA256 TORTURE_BUILD_ENVIRONMENT_BINDING
    "${CMAKE_SOURCE_DIR}|${CMAKE_BINARY_DIR}|${CMAKE_CXX_COMPILER}|${CMAKE_CXX_COMPILER_ID}|${CMAKE_CXX_COMPILER_VERSION}|${CMAKE_SYSTEM_NAME}|${CMAKE_SYSTEM_PROCESSOR}|${CMAKE_CXX_STANDARD}"
)

add_library(torture_common
    src/common/diagnostic.cpp
    src/common/source.cpp
    src/common/sha256.cpp
    src/common/sha512.cpp
)
target_include_directories(torture_common PUBLIC src)

add_library(torture_vm
    src/vm/platform.cpp
    src/vm/bytecode.cpp
    src/vm/environment.cpp
    src/vm/opcode.cc
    src/vm/value.cpp
    src/vm/memory.cpp
    src/vm/vm.cpp
    src/vm/platforms/platform_bytecode_${TORTURE_PLATFORM_NAME}.cpp
    src/vm/platforms/platform_vm_${TORTURE_PLATFORM_NAME}.cpp
    src/vm/platforms/bytecode_binary_${TORTURE_PLATFORM_NAME}.cc
)
target_include_directories(torture_vm PUBLIC src)
target_compile_definitions(torture_vm PUBLIC
    TORTURE_BYTECODE_FORMAT_VERSION=${TORTURE_BYTECODE_FORMAT_VERSION}
    TORTURE_PLATFORM_BYTECODE_HEADER="vm/platforms/platform_bytecode_${TORTURE_PLATFORM_NAME}.h"
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

# 代码格式检查目标
find_program(CLANG_FORMAT_BIN clang-format)
if(CLANG_FORMAT_BIN)
    file(GLOB_RECURSE TORTURE_FORMAT_SOURCES
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.h"
        "${CMAKE_SOURCE_DIR}/src/*.cc"
    )
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_BIN} --dry-run --Werror
            ${TORTURE_FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "检查代码格式是否符合 .clang-format 规范"
        VERBATIM
    )
else()
    message(STATUS "未找到 clang-format，format-check 目标不可用")
endif()
