# 指定系统名称为 Linux
set(CMAKE_SYSTEM_NAME Linux)

# 设置 C 编译器及其选项
set(CMAKE_C_COMPILER "/usr/bin/clang-20")
set(CMAKE_C_FLAGS "-Wall -std=c99")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")  # 注意：-O4 不是标准优化级别，通常使用 -O3
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -march=native -flto")

# 设置 C++ 编译器及其选项，并指定使用 libc++
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-20")
set(CMAKE_CXX_FLAGS "-Wall -stdlib=libc++")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")  # 同样，这里使用 -O3 而不是 -O4  -march=native 进行优化
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -march=native -flto")

# 设置链接器标志以使用 libc++
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_CXX_STANDARD_LIBRARIES "-lc++ -lc++abi")

# 设置 LLVM 工具链路径
set(CMAKE_AR      "/usr/bin/llvm-ar-20" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB  "/usr/bin/llvm-ranlib-20" CACHE FILEPATH "Ranlib")
set(CMAKE_NM      "/usr/bin/llvm-nm-20" CACHE FILEPATH "NM")
set(CMAKE_OBJDUMP "/usr/bin/llvm-objdump-20" CACHE FILEPATH "Objdump")
set(CMAKE_LINKER  "/usr/bin/llvm-ld-20" CACHE FILEPATH "Linker")

# 确保 CMake 使用正确的库进行链接
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# 启用 C++20 模块支持
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

# 为所有目标添加编译选项
add_compile_options(
        -Wno-unused-const-variable  # 禁用未使用的常量变量警告
        -Wno-unused-private-field   # 禁用未使用的私有字段警告
)

function(setting)
endfunction()