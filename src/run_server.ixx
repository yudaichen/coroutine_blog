module;



// #include "common/boost_head.h"
/*#include <boost/asio/io_context.hpp>
#include <boost/mysql/connection_pool.hpp>*/
#include <common/yaml/yaml.hpp>

// common module;
import BS.thread_pool;
import beast_net_server;
import mysql_connect;
import asio;

// setting controller module;
import blog_request;

export module run_server;

void CreateProfile(){

}


export void run_server(){
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
    mysql::connection_pool pool(
        db_ctx, fast::mapper::CreateMysqlOption(params));


    // 初始化服务器配置
    std::string address = config["server"]["address"].As<std::string>();
    unsigned short port = config["server"]["port"].As<unsigned short>();
    std::string doc_root = config["server"]["doc_root"].As<std::string>();
    int threads = config["server"]["threads"].As<int>();
    std::string protocol_str = config["server"]["protocol"].As<std::string>();
    fast::net::Server::Protocol protocol;
    if (protocol_str == "HTTP") {
        protocol = fast::net::Server::Protocol::HTTP;
    } else if (protocol_str == "HTTPS") {
        protocol = fast::net::Server::Protocol::HTTPS;
    } else if (protocol_str == "WebSocket") {
        protocol = fast::net::Server::Protocol::WebSocket;
    } else if (protocol_str == "WebSockets") {
        protocol = fast::net::Server::Protocol::WebSockets;
    } else {
        throw std::runtime_error("Unknown protocol: " + protocol_str);
    }

    // 创建服务器对象
    fast::net::Server server(
        asio::net::ip::make_address(address), port,
        doc_root, threads, protocol, pool);
    fast::blog::set_routines(server);

    BS::thread_pool<2> bs_pool;
    bs_pool.detach_task([&server]() { server.run(); });
    bs_pool.detach_task([&db_ctx, &pool]() {
         db_ctx.run();
         pool.cancel();
       });

    bs_pool.wait();
}