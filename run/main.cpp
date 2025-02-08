import std;
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

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or
// click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
int main()
{
    // TIP Press <shortcut actionId="RenameElement"/> when your caret is at the
    // <b>lang</b> variable name to see how CLion can help you rename it.
    // std::locale::global(std::locale("zh_CN.UTF-8")); // 设置全局 locale
    std::cout.imbue(std::locale()); // 使 cout 使用全局 locale

    auto lang = "C++";
    std::cout << "Hello and welcome to " << lang << "!\n";

    std::cout << "Hello and welcome to 控制台乱码 问题解决 " << lang << "!\n";

    // 初始化日志系统
    log::initialize_logger("logs/app.log");

    // 记录日志
    log::info("Hello, {}!", "spdlog");
    log::error("This is an error message with value: {}", 42);

    run_server();
    log::shutdown_logger();
    return 0;
}

// TIP See CLion help at <a
// href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>.
//  Also, you can try interactive lessons for CLion by selecting
//  'Help | Learn IDE Features' from the main menu.
