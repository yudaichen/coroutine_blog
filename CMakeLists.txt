cmake_minimum_required(VERSION 3.30)
project(asio_blog)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Currently CMake requires extensions enabled when using import std.
# https://gitlab.kitware.com/cmake/cmake/-/issues/25916
# https://gitlab.kitware.com/cmake/cmake/-/issues/25539
set(CMAKE_CXX_EXTENSIONS ON)

setting()

#如果是动态库必须开启
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 根据编译器设置 C++20 选项
#[[if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23")
elseif (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++23")
endif ()]]

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
# 本地项目-包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# 模块文件
file(GLOB_RECURSE MODULES_FILES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx
)

# 头文件
file(GLOB_RECURSE CURRENT_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

# 源文件
file(GLOB_RECURSE CURRENT_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

message("MODULES_FILES : ${MODULES_FILES}")

add_definitions(-DBOOST_ASIO_DISABLE_VECTORIO)
add_definitions(-DBOOST_ASIO_HAS_FILE)
add_definitions(-DBOOST_ASIO_DISABLE_EPOLL)
add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
#add_definitions(-DBOOST_ASIO_SEPARATE_COMPILATION)

# 判断操作系统
if (UNIX)
    # 定义宏
    add_definitions(-DASIO_HAS_FILE)
    add_definitions(-DASIO_DISABLE_EPOLL)

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftime-trace")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftime-trace")
    endif ()

    # 启用增量编译
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wextra")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE) # 避免全局优化影响增量编译

    # # 指定包含 BoostConfig.cmake 文件的目录
    set(Boost_DIR "/mnt/f/runtime/dep_lib/ubuntu24/clang-boost/boost_1_87_0/stage/lib/cmake/Boost-1.87.0")
    add_definitions(-DBOOST_ASIO_HAS_IO_URING)
    add_definitions(-DASIO_HAS_IO_URING)
    #include_directories(/usr/include)
    include_directories(/usr/local/include)
    include_directories(/usr/include/liburing)
    link_directories(/usr/lib/x86_64-linux-gnu)
elseif (MSVC)
    # Boost 头路径
    #include_directories("F:\\runtime\\dep_lib\\win\\release")
    # Boost 库路径
    set(Boost_DIR "F:/runtime/dep_lib/win/boost_1_87_0/stage/lib/cmake/Boost-1.87.0")
endif ()

#include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/debug/conan_toolchain.cmake)

# 列出需要的 Boost 组件
set(BOOST_COMPONENTS
        context
        charconv
        thread
        fiber
        system
        date_time
        timer
        coroutine
        cobalt
        json
        container
        filesystem
        url
)


if (MSVC)
    set(Boost_USE_MULTITHREADED ON)
    # 设置使用共享库
    set(Boost_USE_STATIC_LIBS OFF)
else ()
    # 设置 Boost 不使用多线程库
    set(Boost_USE_MULTITHREADED OFF)
endif ()
# 查找 Boost 库，使用 NO_DEFAULT_PATH 选项
find_package(Boost 1.87.0 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
if (Boost_FOUND)
    message(STATUS "Boost found: ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
else ()
    message(FATAL_ERROR "Boost not found")
endif ()


find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    message("set_property GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM}")
    message("GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PROGRAM})
endif ()

# 查找 Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    # 设置 Doxygen 配置文件路径
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # 配置 Doxyfile，将变量替换为实际值
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    # 添加自定义目标来生成文档
    add_custom_target(
            docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
    )
else(DOXYGEN_FOUND)
    message("Doxygen not found, documentation will not be generated.")
endif(DOXYGEN_FOUND)

# 添加子模块目录
add_subdirectory(third_party/nlohmann_json)
if (NOT TARGET nlohmann_json)
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json)
endif ()

# 设置 JWT_EXTERNAL_NLOHMANN_JSON 为 ON
#et(JWT_EXTERNAL_NLOHMANN_JSON ON CACHE BOOL "Use external nlohmann-json library" FORCE)
set(JWT_BUILD_TESTS OFF CACHE BOOL "Stop jwt-cpp test " FORCE)
set(JWT_BUILD_EXAMPLES OFF)
add_subdirectory(third_party/jwt-cpp)

# 设置 SPDLOG_USE_STD_FORMAT 选项为 ON
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "Use std::format instead of fmt library." FORCE)
add_subdirectory(third_party/spdlog)

add_subdirectory(third_party/googletest)

add_subdirectory(third_party/cpp-dump)
include_directories(third_party/cpp-dump)

add_subdirectory(third_party/icecream-cpp)
include_directories(third_party/icecream-cpp)
# 添加可执行文件
add_executable(asio_blog ${CMAKE_CURRENT_SOURCE_DIR}/run/main.cpp ${MODULES_FILES} ${CURRENT_HEADERS} ${CURRENT_SOURCE})
# 设置模块代码
target_sources(asio_blog
        PUBLIC
        FILE_SET core_cxx_modules TYPE CXX_MODULES FILES
        ${MODULES_FILES}
)

# 分离模块生成与编译
set_property(TARGET asio_blog PROPERTY
        CXX_MODULE_GENERATION_MODE "SEPARATE"
)

if (UNIX)
    target_link_libraries(asio_blog PRIVATE
            pthread
            c++
            c++abi
            -lcrypto
            -lssl
            quadmath
            uring
    )
elseif (WIN32)
    find_package(OpenSSL REQUIRED)

    include_directories("C:/Program Files/OpenSSL-Win64/include")
    include_directories(${OPENSSL_INCLUDE_DIRS})
    target_link_libraries(asio_blog PRIVATE
            ${OPENSSL_LIBRARIES}
    )
endif ()

target_link_libraries(asio_blog PRIVATE jwt-cpp nlohmann_json::nlohmann_json spdlog GTest::gtest
        GTest::gtest_main ${Boost_LIBRARIES})



# 解决clion 无法识别import std
if (UNIX)
    set(LIBCXX_MODULE_DIRS "/usr/lib/llvm-21/share/libc++/v1")
    add_library(unused_std_target STATIC)
    target_sources(unused_std_target
            PRIVATE
            FILE_SET CXX_MODULES
            BASE_DIRS ${LIBCXX_MODULE_DIRS}
            FILES ${LIBCXX_MODULE_DIRS}/std.cppm ${LIBCXX_MODULE_DIRS}/std.compat.cppm)

    target_compile_features(unused_std_target PRIVATE cxx_std_23)
elseif (WIN32)
    set(LIBCXX_MODULE_DIRS "F:/program/visul_studio/idea/VC/Tools/MSVC/14.42.34433/modules")
    add_library(unused_std_target STATIC)
    target_sources(unused_std_target
            PRIVATE
            FILE_SET CXX_MODULES
            BASE_DIRS ${LIBCXX_MODULE_DIRS}
            FILES ${LIBCXX_MODULE_DIRS}/std.ixx ${LIBCXX_MODULE_DIRS}/std.compat.ixx)
    target_compile_features(unused_std_target PRIVATE cxx_std_23)
endif ()

# 为特定目标设置编译器标志
#[[
if(MSVC)

    target_compile_options(unused_std_target PRIVATE "/std:c++latest")
    target_compile_options(asio_blog PRIVATE "/std:c++latest")
endif()]]