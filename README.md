# CMake配置详解：基于LLVM、C++23和Boost.Asio的HTTP博客项目

## 项目概述
本项目名为`blog`，是一个基于`LLVM`、`C++23`、`Boost.Asio`、`Boost.Mysql`的`HTTP协程`博客项目。项目支持使用C++20 Modules特性，包括`import std;`和`import asio;`。项目使用CMake作为构建系统，并通过Ninja进行构建。
     
CMake 最低版本要求：3.30.5

项目名称：import_std_23

C++ 标准：C++23，且强制要求支持该标准。

编译器：llvm-20 clang-20

## 构建方法

    cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=./cmake/clang.toolchain.cmake -S . -B build


## 设置链接器标志以使用libc++
## 启用 C++20 模块支持
```cmake
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
    set(CMAKE_CXX_MODULE_STD ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
```

---

## CMake配置文件详解：开启`import std;`支持及解决CLion识别问题

### 1. 开启`import std;`支持

为了在项目中使用`import std;`，需要确保CMake配置正确地支持C++20 Modules，并且正确地引用了标准库模块（`std`模块）。以下是关键的CMake配置：

#### 1.1 启用C++20 Modules
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

## Boost 库使用 Clang 编译器的编译方法

### 一、编译准备
在开始编译 Boost 之前，请确保已经安装了 Clang 编译器，并且版本支持 C++20 标准。同时，从 [Boost 官方网站](https://www.boost.org/) 下载所需的 Boost 源码包，并解压到指定目录。

### 二、使用 `bootstrap.sh` 脚本初始化
#### 2.1 基础初始化
在解压后的 Boost 源码根目录下，打开终端，执行以下命令使用 `bootstrap.sh` 脚本来初始化编译环境，指定使用 Clang 工具集：
```bash
./bootstrap.sh --with-toolset=clang
```
这个命令会根据 Clang 工具集生成必要的构建文件，为后续的编译做准备。

#### 2.2 特定库初始化（可选）
如果你只需要编译特定的库，例如 `cobalt` 库，可以使用以下命令：
```bash
./bootstrap.sh --with-libraries=cobalt
```
此命令会在初始化时仅针对 `cobalt` 库进行准备。

### 三、配置 `project-config.jam` 文件
#### 3.1 编辑文件
使用文本编辑器打开 `project-config.jam` 文件，该文件通常位于 Boost 源码根目录下。

#### 3.2 指定 Clang 编译器及相关参数
在 `project-config.jam` 文件中添加以下内容，以指定使用 Clang 编译器，并设置必要的编译和链接标志：
```plaintext
using clang : : clang++-20 : <cxxflags>"-stdlib=libc++" <linkflags>"-stdlib=libc++ -lc++abi" ;
```
这里的 `clang++-20` 表示使用 Clang 20 版本的 C++ 编译器，`-stdlib=libc++` 用于指定使用 `libc++` 标准库，`-lc++abi` 用于链接 `libc++` 的 ABI 库。

### 四、使用 `b2` 进行编译

#### 4.1 完整编译
执行以下命令进行完整的编译：
```bash
./b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++ -lc++abi" --build-type=complete stage threading=multi cxxstd=20 --layout=versioned
```
- `toolset=clang`：指定使用 Clang 作为编译工具集。
- `cxxflags="-stdlib=libc++"`：设置 C++ 编译标志，使用 `libc++` 标准库。
- `linkflags="-stdlib=libc++ -lc++abi"`：设置链接标志，使用 `libc++` 标准库并链接其 ABI 库。
- `--build-type=complete`：进行完整的构建，编译所有可用的库配置。
- `stage`：将编译好的库文件安装到 `stage` 目录下。
- `threading=multi`：启用多线程支持。
- `cxxstd=20`：指定使用 C++20 标准进行编译。
- `--layout=versioned`：使用版本化的目录布局，便于区分不同版本的库。

#### 4.2 特定库编译（对应步骤 2.2）
如果在步骤 2.2 中仅针对 `cobalt` 库进行了初始化，那么可以使用以下命令编译 `cobalt` 库：
```bash
./b2 --with-libraries=cobalt link=shared threading=multi cxxstd=20
```
- `--with-libraries=cobalt`：仅编译 `cobalt` 库。
- `link=shared`：编译为共享库。
- `threading=multi`：启用多线程支持。
- `cxxstd=20`：指定使用 C++20 标准进行编译。

## ubuntu mysql启动

    sudo systemctl status mysql
    sudo systemctl start mysql

## 如果你使用conan
```bash
   debug:   cd cmake/debug  conan install ../.. -of . --build missing -s build_type=Debug
   release: cd cmake/debug  conan install ../.. -of . --build missing -s build_type=Release
```