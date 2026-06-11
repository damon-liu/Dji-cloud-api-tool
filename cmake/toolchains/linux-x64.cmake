# ============================================
# Linux x86_64 交叉编译工具链文件 (基于 Zig)
#
# 用法:
#   export ZIG_PATH=/d/software/c++/lib/zig/zig-windows-x86_64-0.13.0/zig.exe
#   cmake -B build_linux -G "MinGW Makefiles" \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-x64.cmake
#   cmake --build build_linux
# ============================================

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_SYSTEM_VERSION 1)

# ============================================
# Zig 路径查找
# 优先级: CMake -DZIG_PATH=... > 环境变量 ZIG_PATH > PATH
# ============================================
# try_compile 子项目需要继承 ZIG_PATH，否则 ABI 检测阶段找不到编译器
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ZIG_PATH)

if(NOT ZIG_PATH)
    if(DEFINED ENV{ZIG_PATH})
        set(ZIG_PATH "$ENV{ZIG_PATH}" CACHE FILEPATH "Zig 编译器路径")
    else()
        find_program(ZIG_PATH zig)
    endif()
endif()

if(NOT ZIG_PATH)
    message(FATAL_ERROR
        "未找到 zig 编译器！请设置环境变量:\n"
        "  export ZIG_PATH=/path/to/zig/zig.exe\n"
        "或下载: https://ziglang.org/download/")
endif()

message(STATUS "Zig 编译器: ${ZIG_PATH}")

# ============================================
# 目标平台 (Ubuntu 24 = glibc 2.39)
# ============================================
set(TARGET_TRIPLE "x86_64-linux-gnu.2.39")

# ============================================
# 编译器设置
# ============================================
set(CMAKE_C_COMPILER "${ZIG_PATH}")
set(CMAKE_CXX_COMPILER "${ZIG_PATH}")
set(CMAKE_C_COMPILER_ARG1 "cc")
set(CMAKE_CXX_COMPILER_ARG1 "c++")

set(CMAKE_C_FLAGS_INIT "-target ${TARGET_TRIPLE}")
set(CMAKE_CXX_FLAGS_INIT "-target ${TARGET_TRIPLE}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-target ${TARGET_TRIPLE}")

# ============================================
# 交叉编译设置
# ============================================
set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
