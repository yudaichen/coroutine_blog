module;

#include <common/yaml/yaml.hpp>

// common module;
import BS.thread_pool;
import beast_net_server;
import mysql_connect;
import boost;
import log;

// setting controller module;
import blog_request;

export module run_server;

void CreateProfile()
{
}

export void run_server()
{
    asio::io_context db_ctx(2);

    // 解析 YAML 配置文件
    Yaml::Node config;
    Yaml::Parse(config, "config.yaml");

    // 初始化数据库配置
    fast::mapper::OptionParams params;
    params.host = config["database"]["host"].As<std::string>();
    params.port = config["database"]["port"].As<unsigned short>();
    params.username = config["database"]["username"].As<std::string>();
    params.password = config["database"]["password"].As<std::string>();
    params.database = config["database"]["database"].As<std::string>();

    // 创建数据库连接池
    mysql::connection_pool pool(db_ctx, fast::mapper::CreateMysqlOption(params));

    // 初始化服务器配置
    std::string address = config["server"]["address"].As<std::string>();
    unsigned short port = config["server"]["port"].As<unsigned short>();
    std::string doc_root = config["server"]["doc_root"].As<std::string>();
    int threads = config["server"]["threads"].As<int>();
    std::string protocol_str = config["server"]["protocol"].As<std::string>();
    fast::net::Server::Protocol protocol;
    if (protocol_str == "HTTP")
    {
        protocol = fast::net::Server::Protocol::HTTP;
    }
    else if (protocol_str == "HTTPS")
    {
        protocol = fast::net::Server::Protocol::HTTPS;
    }
    else if (protocol_str == "WebSocket")
    {
        protocol = fast::net::Server::Protocol::WebSocket;
    }
    else if (protocol_str == "WebSockets")
    {
        protocol = fast::net::Server::Protocol::WebSockets;
    }
    else
    {
        throw std::runtime_error("Unknown protocol: " + protocol_str);
    }

    // 记录创建服务器对象的日志
    fast::log::info("创建 - 服务器对象 - 正在进行");
    fast::net::Server server(asio::net::ip::make_address(address), port, doc_root, threads, protocol, pool);
    fast::log::info("创建 - 服务器对象 - 已完成");

    // 记录设置路由的日志
    fast::log::info("设置 - 服务器路由 - 正在进行");
    fast::blog::set_routines(server);
    fast::log::info("设置 - 服务器路由 - 已完成");

    // 记录线程池创建的日志
    fast::log::info("创建 - 线程池 - 正在进行");
    BS::thread_pool<2> bs_pool;
    fast::log::info("创建 - 线程池 - 已完成，线程池大小为 2");

    // 记录启动服务器运行任务的日志
    fast::log::info("启动 - 服务器运行任务 - 正在进行");
    bs_pool.detach_task([&server]() {
        fast::log::info("执行 - 服务器运行任务 - 已启动");
        server.run();
        fast::log::info("执行 - 服务器运行任务 - 已完成");
    });

    // 记录启动数据库上下文运行任务的日志
    fast::log::info("启动 - 数据库上下文运行任务 - 正在进行");
    bs_pool.detach_task([&db_ctx, &pool]() {
        fast::log::info("执行 - 数据库上下文运行任务 - 已启动");
        db_ctx.run();
        pool.cancel();
        db_ctx.stop();
        fast::log::info("执行 - 数据库上下文运行任务 - 已完成");
    });

    // 记录等待所有任务完成的日志
    fast::log::info("等待 - 线程池所有任务 - 正在进行");
    bs_pool.wait();
    fast::log::info("等待 - 线程池所有任务 - 已完成");
}