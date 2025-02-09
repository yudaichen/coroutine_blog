module;

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"


import std;
export module log;


// 定义 ANSI 颜色转义序列
namespace ansi_colors {
    const std::string reset = "\033[0m";
    const std::string green = "\033[32m";
    const std::string cyan = "\033[36m";
    const std::string yellow = "\033[33m";
    const std::string red = "\033[31m";
    const std::string bold_on_red = "\033[1;41m";
}

export namespace fast::log
{

// 初始化日志系统
inline void initialize_logger(const std::string &log_file)
{
    try
    {
        // 创建控制台彩色日志接收器，使用自动颜色模式
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(spdlog::color_mode::always);

        console_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e]%$ [%^%l%$] %v");


        // 自定义颜色映射，为不同日志级别设置颜色
        /*console_sink->set_color(spdlog::level::trace, console_sink->green);
        console_sink->set_color(spdlog::level::debug, console_sink->cyan);
        console_sink->set_color(spdlog::level::info, console_sink->green);
        console_sink->set_color(spdlog::level::warn, console_sink->yellow);
        console_sink->set_color(spdlog::level::err, console_sink->red);
        console_sink->set_color(spdlog::level::critical, console_sink->bold_on_red);*/

        console_sink->set_level(spdlog::level::trace); // 允许输出 trace 级别的日志

        // 创建文件接收器
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
        file_sink->set_level(spdlog::level::trace);

        // 组合接收器
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

        // 创建日志记录器
        auto logger = std::make_shared<spdlog::logger>("multi_sink_logger", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace);

        // 设置默认日志记录器
        spdlog::set_default_logger(logger);
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}

// 获取默认日志记录器
inline std::shared_ptr<spdlog::logger> get_default_logger()
{
    return spdlog::default_logger();
}


// 封装 spdlog::flush_all()
inline void flush_all()
{
    get_default_logger()->flush();
}

// 手动关闭日志
inline void shutdown_logger()
{
    spdlog::shutdown();
}

// 记录 trace 级别的日志
template <typename... Args> inline void trace(const std::string &format, Args &&...args)
{
    std::string colored_format = ansi_colors::green + format + ansi_colors::reset;
    spdlog::trace(colored_format, std::forward<Args>(args)...);
    flush_all();
}

// 记录 debug 级别的日志
template <typename... Args> inline void debug(const std::string &format, Args &&...args)
{
    std::string colored_format = ansi_colors::cyan + format + ansi_colors::reset;
    spdlog::debug(colored_format, std::forward<Args>(args)...);
    flush_all();
}

// 记录 info 级别的日志
template <typename... Args> inline void info(const std::string &format, Args &&...args)
{
    std::string colored_format = ansi_colors::green + format + ansi_colors::reset;
    spdlog::info(colored_format, std::forward<Args>(args)...);
    flush_all();
}

// 记录 warn 级别的日志
template <typename... Args> inline void warn(const std::string &format, Args &&...args)
{
    std::string colored_format = ansi_colors::yellow + format + ansi_colors::reset;
    spdlog::warn(colored_format, std::forward<Args>(args)...);
    flush_all();
}

// 记录 error 级别的日志
template <typename... Args> inline void error(const std::string &format, Args &&...args)
{
    std::string colored_format = ansi_colors::red + format + ansi_colors::reset;
    spdlog::error(colored_format, std::forward<Args>(args)...);
    flush_all();
}

// 记录 critical 级别的日志
template <typename... Args> inline void critical(const std::string &format, Args &&...args)
{
    std::string colored_format = ansi_colors::bold_on_red + format + ansi_colors::reset;
    spdlog::critical(colored_format, std::forward<Args>(args)...);
    flush_all();
}

} // namespace log
