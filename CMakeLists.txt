cmake_minimum_required(VERSION 3.30.5)
project(asio_blog)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Currently CMake requires extensions enabled when using import std.
# https://gitlab.kitware.com/cmake/cmake/-/issues/25916
# https://gitlab.kitware.com/cmake/cmake/-/issues/25539
set(CMAKE_CXX_EXTENSIONS ON)


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftime-trace")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftime-trace")
endif ()

# 启用增量编译
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wextra")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE) # 避免全局优化影响增量编译

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

# 定义宏
add_definitions(-DBOOST_ASIO_DISABLE_VECTORIO)
add_definitions(-DBOOST_ASIO_HAS_FILE)
add_definitions(-DBOOST_ASIO_DISABLE_EPOLL)
add_definitions(-DBOOST_ASIO_HAS_CO_AWAIT)
add_definitions(-DASIO_HAS_FILE)
add_definitions(-DASIO_DISABLE_EPOLL)

# 判断操作系统
if (UNIX)
    # Boost 头路径
    include_directories("/mnt/f/runtime/dep_lib/ubuntu24/clang-boost/release")
    # Boost 库路径
    set(BOOST_LIB_PATH "/mnt/f/runtime/dep_lib/ubuntu24/clang-boost/boost_1_87_0/stage/lib")
    add_definitions(-DBOOST_ASIO_HAS_IO_URING)
    add_definitions(-DASIO_HAS_IO_URING)
    include_directories(/usr/include)
    include_directories(/usr/local/include)
    include_directories(/usr/include/liburing)
    link_directories(/usr/lib/x86_64-linux-gnu)
elseif (MSVC)
    # Boost 头路径
    include_directories("F:\\runtime\\dep_lib\\win\\release")
    # Boost 库路径
    set(BOOST_LIB_PATH "F:/runtime/dep_lib/win/boost_1_87_0/stage/lib")
endif ()


find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    message("set_property GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM}")
    message("GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PROGRAM})
endif ()

# 添加子模块目录
add_subdirectory(third_party/json)

# 设置 JWT_EXTERNAL_NLOHMANN_JSON 为 ON
set(JWT_EXTERNAL_NLOHMANN_JSON ON CACHE BOOL "Use external nlohmann-json library" FORCE)
add_subdirectory(third_party/jwt-cpp)


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

# 解决clion报错的问题
if (UNIX)
    set(LIBCXX_MODULE_DIRS "/usr/lib/llvm-20/share/libc++/v1")
    add_library(unused_std_target STATIC)
    target_sources(unused_std_target
            PRIVATE
            FILE_SET CXX_MODULES
            BASE_DIRS ${LIBCXX_MODULE_DIRS}
            FILES ${LIBCXX_MODULE_DIRS}/std.cppm ${LIBCXX_MODULE_DIRS}/std.compat.cppm)
elseif (WIN32)
    set(LIBCXX_MODULE_DIRS "F:/program/visul_studio/idea/VC/Tools/MSVC/14.42.34433/modules")
    add_library(unused_std_target STATIC)
    target_sources(unused_std_target
            PRIVATE
            FILE_SET CXX_MODULES
            BASE_DIRS ${LIBCXX_MODULE_DIRS}
            FILES ${LIBCXX_MODULE_DIRS}/std.ixx ${LIBCXX_MODULE_DIRS}/std.compat.ixx)

endif ()

# Boost 库列表
set(BOOST_LIBS
        boost_context
        boost_charconv
        boost_thread
        boost_fiber
        boost_system
        boost_date_time
        boost_timer
        boost_coroutine
        boost_cobalt
        boost_json
        boost_container
        boost_filesystem
        boost_url
)
if (UNIX)
    # 添加库文件并链接 Boost 库
    target_link_libraries(asio_blog
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
    # 添加库文件并链接 Boost 库
    target_link_libraries(asio_blog
            ${OPENSSL_LIBRARIES}
    )
endif ()

# 链接 jwt-cpp 库到你的目标可执行文件或库
target_link_libraries(asio_blog jwt-cpp nlohmann_json::nlohmann_json)


# 链接 Boost 库
foreach (lib ${BOOST_LIBS})
    if (UNIX)
        set(LIB_PATH ${BOOST_LIB_PATH}/lib${lib}.a)
    elseif (MSVC)
        string(REGEX REPLACE "^lib" "" lib_name ${lib})
        set(LIB_PATH ${BOOST_LIB_PATH}\\${lib_name}-vc143-mt-gd-x64-1_87.lib)
    endif ()

    if (EXISTS ${LIB_PATH})
        message(STATUS "Found library: ${LIB_PATH}")
        target_link_libraries(asio_blog ${LIB_PATH})
    else ()
        message(FATAL_ERROR "Library not found: ${LIB_PATH}")
    endif ()
endforeach ()

