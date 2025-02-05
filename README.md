# CMake配置详解：基于LLVM、C++23和Boost.Asio的HTTP博客项目

## 项目概述
本项目名为`blog`，是一个基于`LLVM`、`C++23`、`Boost.Asio`、`Boost.Mysql`的`HTTP协程`博客项目。项目支持使用C++20 Modules特性，包括`import std;`和`import asio;`。项目使用CMake作为构建系统，并通过Ninja进行构建。
### CMake 最低版本要求：3.30.5
### 项目名称：import_std_23
### C++ 标准：C++23，且强制要求支持该标准。
### llvm-20 clang-20

### 编译器和链接器标志

# 设置链接器标志以使用libc++
# 启用 C++20 模块支持
```cmake
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
    set(CMAKE_CXX_MODULE_STD ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
```

---

# CMake配置文件详解：开启`import std;`支持及解决CLion识别问题

## 1. 开启`import std;`支持

为了在项目中使用`import std;`，需要确保CMake配置正确地支持C++20 Modules，并且正确地引用了标准库模块（`std`模块）。以下是关键的CMake配置：

### 1.1 启用C++20 Modules
```cmake
cmake_minimum_required(VERSION 3.30.5)
project(import_std_23)

# 设置C++标准为C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用C++20 Modules支持
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 当前CMake要求启用扩展才能使用import std
set(CMAKE_CXX_EXTENSIONS ON)
```

### 1.2 配置标准库模块路径
为了支持`import std;`，需要告诉CMake标准库模块文件的位置。这可以通过创建一个静态库目标来实现，该目标仅用于引用标准库模块文件。

#### 1.2.1 对于Unix系统
```cmake
if (UNIX)
    # 设置标准库模块目录路径
    set(LIBCXX_MODULE_DIRS "/usr/lib/llvm-20/share/libc++/v1")
    
    # 创建一个静态库目标，用于引用标准库模块
    add_library(unused_std_target STATIC)
    target_sources(unused_std_target
            PRIVATE
            FILE_SET CXX_MODULES
            BASE_DIRS ${LIBCXX_MODULE_DIRS}
            FILES ${LIBCXX_MODULE_DIRS}/std.cppm ${LIBCXX_MODULE_DIRS}/std.compat.cppm)
endif ()
```

#### 1.2.2 对于Windows系统
```cmake
elseif (WIN32)
    # 设置标准库模块目录路径
    set(LIBCXX_MODULE_DIRS "/visul_studio/idea/VC/Tools/MSVC/14.42.34433/modules")
    
    # 创建一个静态库目标，用于引用标准库模块
    add_library(unused_std_target STATIC)
    target_sources(unused_std_target
            PRIVATE
            FILE_SET CXX_MODULES
            BASE_DIRS ${LIBCXX_MODULE_DIRS}
            FILES ${LIBCXX_MODULE_DIRS}/std.ixx ${LIBCXX_MODULE_DIRS}/std.compat.ixx)
endif ()
```

### 1.3 解释
- **`LIBCXX_MODULE_DIRS`**：变量用于存储标准库模块文件的路径。路径根据操作系统和使用的编译器版本而有所不同。
- **`add_library(unused_std_target STATIC)`**：创建一个静态库目标`unused_std_target`，该目标不会生成实际的库文件，但会告诉CMake标准库模块文件的位置。
- **`target_sources`**：通过`FILE_SET CXX_MODULES`指定标准库模块文件的位置。`BASE_DIRS`用于指定模块文件的基目录，`FILES`用于指定具体的模块文件。

## 2. 解决CLion无法识别`import std;`的问题

CLion目前对C++20 Modules的支持有限，尤其是在代码补全和语法高亮方面。通过上述配置，虽然可以确保项目能够正确编译和链接，但CLion可能仍然无法识别`import std;`。

### 2.1 解决方法
- **创建一个“占位”目标**：通过创建一个静态库目标（如`unused_std_target`），并将其添加到项目中，可以“欺骗”CLion，使其认为项目中存在标准库模块文件。
- **确保路径正确**：确保`LIBCXX_MODULE_DIRS`变量指向正确的标准库模块文件路径。

### 2.2 注意事项
- **路径问题**：如果CLion仍然无法识别`import std;`，可以尝试手动检查`LIBCXX_MODULE_DIRS`路径是否正确，并确保路径中的模块文件存在。
- **编译器版本**：确保使用的编译器版本支持C++20 Modules，并且CMake版本至少为3.30.5。

通过上述配置，项目可以正确使用`import std;`，并且在一定程度上解决了CLion无法识别该模块的问题。

## import std
```
  {
    "feature": "import std;",
    "imported_content": "仅 C++ 标准库命名空间 std 中的内容。",
    "c_runtime_function_access": "需要使用 std:: 前缀（例如 std::printf）。",
    "global_namespace_pollution": "无污染。",
    "use_case": "新代码或对代码清晰度要求较高的项目。",
    "detail": "只导入 C++ 标准库命名空间 `std` 中定义的声明和名称，
    例如 `std::vector`、`std::cout`、`std::string` 等。C 运行时库的函数（例如 `printf`、`fopen`、`malloc` 等）
    **不会**被导入到全局命名空间。
    这样做的好处是避免了全局命名空间的污染，使得代码更加清晰和易于维护。"
  },
  {
    "feature": "import std.compat;",
    "imported_content": "C++ 标准库命名空间 std 和 C 运行时库的全局符号。",
    "c_runtime_function_access": "可以直接使用（例如 printf）。",
    "global_namespace_pollution": "有污染。",
    "use_case": "从旧代码迁移到模块化的代码。",
    "detail": "同时导入 C++ 标准库命名空间 `std` 中的所有内容，**并且**
    将 C 运行时库的函数导入到**全局命名空间**。
    这意味着你可以像在 C 语言中一样直接使用 `printf`、`fopen`、`malloc` 等函数，而无需 `std::` 前缀。
    这样做是为了方便从旧的、大量使用 C 运行时库函数的 C++ 代码迁移到模块化的代码。
    它可以最大程度地减少迁移过程中需要修改的代码量。"
  }
```

# ubuntu24 安装：

## clang安装

https://apt.llvm.org/


编辑 APT 源列表。你可以使用文本编辑器（如 nano 或 vim）来编辑 /etc/apt/sources.list 文件，或者在 /etc/apt/sources.list.d/ 目录中创建一个新的文件。例如，使用 nano 创建一个名为 llvm.list 的文件：

    sudo nano /etc/apt/sources.list.d/llvm.list
添加以下内容到文件中：

    # Noble (24.04) - Last update : Fri, 20 Dec 2024 01:33:14 UTC / Revision: 20241219100923+5f096fd22160
    deb http://apt.llvm.org/noble/ llvm-toolchain-noble main
    deb-src http://apt.llvm.org/noble/ llvm-toolchain-noble main
    # 19
    deb http://apt.llvm.org/noble/ llvm-toolchain-noble-19 main
    deb-src http://apt.llvm.org/noble/ llvm-toolchain-noble-19 main
    # 20
    deb http://apt.llvm.org/noble/ llvm-toolchain-noble-20 main
    deb-src http://apt.llvm.org/noble/ llvm-toolchain-noble-19 main
    保存并退出编辑器。对于 nano，你可以按 CTRL + O 保存，然后按 CTRL + X 退出。

更新 APT 包索引：

    sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 15CF4D18AF4F7421
    sudo apt update
    sudo apt install clang-19 libclang-common-19-dev

## 编译Boost库
### Linux：
使用b2工具编译Boost库，指定Clang作为编译器，并使用libc++：

    ./b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++"
如果需要编译特定的Boost库，可以使用--with-<library>参数，例如：

    ./b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" --with-thread --with-system --with-regex
### Windows：
使用b2工具编译Boost库，指定Clang作为编译器，并使用libc++：

    b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++"
如果需要编译特定的Boost库，可以使用--with-<library>参数：

    b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" --with-thread --with-system --with-regex
### 安装Boost库
   编译完成后，可以将生成的库文件安装到指定目录。例如：

    ./b2 install --prefix=/path/to/boost/install
   或在Windows上：
    b2 install --prefix=C:\path\to\boost\install