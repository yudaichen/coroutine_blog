#include <cpp-dump.hpp>
#include <icecream.hpp>

#include "common/crash_handler.hpp"
import log;

/**
*[
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
]
 */
import std;
import run_server;

// 打印用户自定义类型
// CPP_DUMP_DEFINE_EXPORT_OBJECT_GENERIC(i, str());

// 自定义结构体
struct Person
{
    std::string name;
    int age;
    double height;
    /**friend std::ostream &operator<<(std::ostream &os, const Person &obj)
    {
        return os << "name: " << obj.name << " age: " << obj.age << " height: " << obj.height;
    }*/
};

int main()
{
    CrashHandler handler;
    CPP_DUMP_SET_OPTION(cont_indent_style, cpp_dump::types::cont_indent_style_t::when_non_tuples_nested);

    try
    {
        std::locale locale = std::locale("zh_CN.UTF-8");
        // 设置全局 locale
        std::locale::global(locale);
        // 将 std::cout 的区域设置为 UTF-8
        std::cout.imbue(locale);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }

    auto lang = "C++";
    std::cout << "Hello and welcome to " << lang << "!\n";

    std::cout << "Hello and welcome to 控制台乱码 问题解决 " << lang << "!\n";

    // 初始化日志系统
    fast::log::initialize_logger("logs/app.log");

    // 记录日志
    fast::log::info("Hello, {}!", "spdlog");
    fast::log::error("This is an error message with value: {}", 42);
    std::vector<std::vector<int>> my_vector{{3, 5, 8, 9, 7}, {9, 3, 2, 3, 8}};
    cpp_dump(my_vector);
    IC(my_vector);
    Person p = {"Alice", 25, 1.65};
    cpp_dump(p);
    IC(p);
    // 创建一个存储 Person 结构体的 std::map
    std::map<std::string, Person> personMap;

    // 向 map 中插入一些 Person 结构体实例
    personMap["Alice"] = {"Alice字符", 25, 1.65};
    personMap["Bob"] = {"Bob字符", 30, 1.80};
    personMap["Charlie"] = {"Charlie字符2323", 35, 1.75};

    // 使用 cpp-dump 打印 std::map
    cpp_dump(personMap);
    IC(personMap);

     run_server();
    fast::log::shutdown_logger();
    return 0;
}

// TIP See CLion help at <a
// href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>.
//  Also, you can try interactive lessons for CLion by selecting
//  'Help | Learn IDE Features' from the main menu.
