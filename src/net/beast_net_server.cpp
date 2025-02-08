module;

#include "common/server_certi.hpp"
#include <boost/asio/error.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/system/error_code.hpp>

import std;
import mysql_connect;
import log; // 导入 log 模块
module beast_net_server;
import session_data;
import router_process;
import net_common;
import boost;

namespace fast::net
{
task_group::task_group(asio::any_io_executor exec) : cv_{std::move(exec), asio::steady_timer::time_point::max()}
{
}

template <typename CompletionToken> auto task_group::adapt(CompletionToken &&completion_token)
{
    auto lg = std::lock_guard{mtx_};
    auto cs = css_.emplace(css_.end());

    class remover
    {
        task_group *tg_;
        decltype(css_)::iterator cs_;

      public:
        remover(task_group *tg, decltype(css_)::iterator cs) : tg_{tg}, cs_{cs}
        {
        }

        remover(remover &&other) noexcept : tg_{std::exchange(other.tg_, nullptr)}, cs_{other.cs_}
        {
        }

        ~remover()
        {
            if (tg_)
            {
                auto lg = std::lock_guard{tg_->mtx_};
                tg_->css_.erase(cs_);  // 删除取消槽
                if (tg_->css_.empty()) // 正确判断 css_ 是否为空
                    tg_->cv_.cancel(); // 如果 css_ 为空，则发送信号
            }
        }
    };

    return asio::bind_cancellation_slot(
        cs->slot(), asio::consign(std::forward<CompletionToken>(completion_token), remover{this, cs}));
}

void task_group::emit(asio::cancellation_type type)
{
    auto lg = std::lock_guard{mtx_};
    for (auto &cs : css_)
        cs.emit(type);
}

template <typename CompletionToken> auto task_group::async_wait(CompletionToken &&completion_token)
{
    return asio::async_compose<CompletionToken, void(boost::system::error_code)>(
        [this, scheduled = false](auto &&self, boost::system::error_code ec = {}) mutable {
            if (!scheduled)
                self.reset_cancellation_state(asio::enable_total_cancellation());

            if (!self.cancelled() && ec == boost::asio::error::operation_aborted)
                ec = {};

            {
                auto lg = std::lock_guard{mtx_};
                if (!css_.empty() && !ec)
                {
                    scheduled = true;
                    return cv_.async_wait(std::move(self));
                }
            }

            if (!std::exchange(scheduled, true))
                return asio::post(asio::append(std::move(self), ec));

            self.complete(ec);
        },
        completion_token, cv_);
}

Server::Server(const asio::address &address, unsigned short port, const std::string &doc_root, int threads,
               Protocol protocol, mysql::connection_pool &pool)
    : endpoint_(address, port), doc_root_(doc_root), threads_(threads), protocol_(protocol), ioc_(threads),
      ctx_(asio::ssl::context::tlsv12), task_group_(ioc_.get_executor()), database_(pool), session_(ioc_, database_)
{
    if (protocol_ == Protocol::HTTPS || protocol_ == Protocol::WebSockets)
    {
        load_server_certificate(ctx_);
    }
}

void Server::run()
{
    log::info("启动 - 服务器运行 - 正在进行");
    asio::co_spawn(asio::make_strand(ioc_), listen(task_group_, ctx_, endpoint_, doc_root_, protocol_),
                   task_group_.adapt([](std::exception_ptr e) {
                       if (e)
                       {
                           try
                           {
                               std::rethrow_exception(e);
                           }
                           catch (std::exception &e)
                           {
                               log::error("错误 - 监听器 - {}", e.what());
                           }
                       }
                   }));

    asio::co_spawn(asio::make_strand(ioc_), handle_signals(task_group_, ioc_), asio::detached);

    std::vector<std::thread> v;
    v.reserve(threads_ - 1);
    for (auto i = threads_ - 1; i > 0; --i)
        v.emplace_back([&ioc = ioc_] { ioc.run(); });

    ioc_.run();

    for (auto &t : v)
        t.join();
    log::info("启动 - 服务器运行 - 已完成");
}

asio::awaitable<void> fast::net::Server::detect_session(beast::tcp_stream stream, asio::ssl::context &ctx,
                                                        beast::string_view doc_root, Protocol protocol)
{
    beast::flat_buffer buffer;

    co_await asio::this_coro::reset_cancellation_state(asio::enable_total_cancellation(),
                                                       asio::enable_terminal_cancellation());

    co_await asio::this_coro::throw_if_cancelled(false);

    stream.expires_after(std::chrono::seconds(30));

    auto remote_ip = beast::get_lowest_layer(stream).socket().remote_endpoint().address().to_string();
    log::info("检测 - 会话检测 - 正在对地址 {} 进行会话检测", remote_ip);

    if (protocol == Protocol::HTTPS || protocol == Protocol::WebSockets)
    {
        asio::ssl::stream<beast::tcp_stream> ssl_stream{std::move(stream), ctx};

        co_await ssl_stream.async_handshake(asio::ssl::stream_base::server, buffer.data());

        if (protocol == Protocol::WebSockets)
        {
            beast::http::request<beast::http::string_body> req;
            co_await run_websocket_session(ssl_stream, buffer, std::move(req), doc_root, *this);
        }
        else
        {
            co_await run_session(ssl_stream, buffer, doc_root, *this);
        }

        if (!ssl_stream.lowest_layer().is_open())
        {
            log::info("检测 - 会话检测 - 地址 {} 的会话检测结束，连接已关闭", remote_ip);
            co_return;
        }

        auto [ec] = co_await ssl_stream.async_shutdown(asio::as_tuple);
        if (ec && ec != boost::asio::ssl::error::stream_truncated)
        {
            log::error("错误 - 会话检测 - 地址 {} 的 SSL 关闭出错: {}", remote_ip, ec.message());
            throw boost::system::system_error{ec};
        }
    }
    else
    {
        if (protocol == Protocol::WebSocket)
        {
            beast::http::request<beast::http::string_body> req;
            co_await run_websocket_session(stream, buffer, std::move(req), doc_root, *this);
        }
        else
        {
            co_await run_session(stream, buffer, doc_root, *this);
        }

        if (!stream.socket().is_open())
        {
            log::info("检测 - 会话检测 - 地址 {} 的会话检测结束，连接已关闭", remote_ip);
            co_return;
        }

        stream.socket().shutdown(asio::net::tcp::socket::shutdown_send);
    }
    log::info("检测 - 会话检测 - 地址 {} 的会话检测完成", remote_ip);
}

template <fast::net::ValidStream Stream>
asio::awaitable<void> fast::net::Server::run_session(Stream &stream, beast::flat_buffer &buffer,
                                                     beast::string_view doc_root, Server &server)
{
    asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;

    // 获取客户端 IP 地址
    auto remote_ip = beast::get_lowest_layer(stream).socket().remote_endpoint().address().to_string();

    // 获取或创建会话
    fast::net::SessionData *session_ptr = co_await server.session_.get_or_create_session(remote_ip);

    // 将裸指针转换为引用
    fast::net::SessionData &session_ref = *session_ptr;
    auto uuid = session_ref.get_uuid();
    log::info("上线 - 用户上线 - 地址 {}，UUID {} 上线", remote_ip, uuid);

    while (cs.cancelled() == asio::cancellation_type::none)
    {
        beast::http::request_parser<beast::http::string_body> parser;
        parser.body_limit(10000);

        // 设置超时
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

        // 异步读取请求
        auto [ec, _] = co_await beast::http::async_read(stream, buffer, parser, asio::as_tuple);

        // 处理超时或连接关闭
        if (ec == boost::beast::http::error::end_of_stream || ec == boost::asio::error::operation_aborted)
        {
            // 移除会话
            co_await server.session_.remove_session(remote_ip);
            log::info("下线 - 用户下线 - 地址 {}，UUID {} 下线", remote_ip, uuid);
            co_return;
        }

        if (ec)
        {
            // 处理其他错误
            beast::http::response<beast::http::string_body> res{beast::http::status::internal_server_error,
                                                                parser.get().version()};
            res.body() = "Read error: " + ec.message();
            res.prepare_payload();
            log::error("错误 - 会话处理 - 地址 {}，UUID {} 读取请求出错: {}", remote_ip, uuid, ec.message());
            co_await beast::http::async_write(stream, res);
            co_return;
        }

        if (beast::websocket::is_upgrade(parser.get()))
        {
            // 升级到 WebSocket
            beast::get_lowest_layer(stream).expires_never();
            beast::http::request<beast::http::string_body> req = parser.release();
            co_await run_websocket_session(stream, buffer, std::move(req), doc_root, server);
            co_return;
        }

        // 处理普通 HTTP 请求
        auto req = parser.release();

        if (auto target = std::string(req.target()); target.find("/static/") == 0)
        {
            co_await server.router_.handle_static_request(stream, buffer, doc_root, std::move(req));
            continue;
        }

        beast::http::response<beast::http::string_body> res{beast::http::status::ok, req.version()};

        try
        {
            co_await server.router_.handle_request(req, res, session_ref);
        }
        catch (const std::exception &e)
        {
            res = server_error(std::move(req), e.what());
            log::error("错误 - 会话处理 - 地址 {}，UUID {} 处理请求出错: {}", remote_ip, uuid, e.what());
        }

        if (!res.keep_alive())
        {
            co_await beast::http::async_write(stream, std::move(res));
            beast::get_lowest_layer(stream).close(); // 及时关闭连接
            log::info("下线 - 用户下线 - 地址 {}，UUID {} 因连接无保持而关闭下线", remote_ip, uuid);
            co_return;
        }

        co_await beast::http::async_write(stream, std::move(res));
    }

    // 移除会话
    co_await server.session_.remove_session(remote_ip);
    log::info("下线 - 用户下线 - 地址 {}，UUID {} 下线", remote_ip, uuid);
    beast::get_lowest_layer(stream).close(); // 及时关闭连接
}

template <fast::net::ValidStream Stream>
asio::awaitable<void> fast::net::Server::run_websocket_session(Stream &stream, beast::flat_buffer &buffer,
                                                               beast::http::request<beast::http::string_body> req,
                                                               beast::string_view doc_root, Server &server)
{

    auto cs = co_await asio::this_coro::cancellation_state;
    auto ws = beast::websocket::stream<Stream&>{stream};

    ws.set_option(beast::websocket::stream_base::decorator([](beast::websocket::response_type& res) {
        res.set(beast::http::field::server, "asio advanced-server-flex");
    }));

    // 获取客户端 IP 地址
    auto remote_ip = beast::get_lowest_layer(stream).socket().remote_endpoint().address().to_string();

    // 获取或创建会话
    fast::net::SessionData* session_ptr = co_await server.session_.get_or_create_session(remote_ip);

    // 将裸指针转换为引用
    fast::net::SessionData& session_ref = *session_ptr;
    auto uuid = session_ref.get_uuid();

    log::info("上线 - 用户上线 - 地址 {}，UUID {} 以 WebSocket 方式上线", remote_ip, uuid);

    while (cs.cancelled() == asio::cancellation_type::none)
    {
        buffer.clear();
        auto [ec, bytes_transferred] = co_await ws.async_read(buffer, asio::as_tuple);

        if (ec == boost::beast::websocket::error::closed || ec == boost::asio::ssl::error::stream_truncated)
        {
            // 移除会话
            co_await server.session_.remove_session(remote_ip);
            log::info("下线 - 用户下线 - 地址 {}，UUID {} 以 WebSocket 方式下线", remote_ip, uuid);
            co_return;
        }

        if (ec)
        {
            log::error("错误 - WebSocket 会话处理 - 地址 {}，UUID {} 读取 WebSocket 消息出错: {}", remote_ip, uuid, ec.message());
            throw boost::system::system_error{ec};
        }

        // 处理 WebSocket 消息
        beast::http::request<beast::http::string_body> ws_req;
        ws_req.method(beast::http::verb::post);
        ws_req.target("/ws");
        ws_req.body() = beast::buffers_to_string(buffer.data());
        ws_req.set(beast::http::field::content_type, "text/plain");

        beast::http::response<beast::http::string_body> ws_res{beast::http::status::ok, ws_req.version()};

        try
        {
            co_await server.router_.handle_request(ws_req, ws_res, session_ref);
        }
        catch (std::exception& e)
        {
            ws_res = fast::net::server_error(std::move(ws_req), e.what());
            log::error("错误 - WebSocket 会话处理 - 地址 {}，UUID {} 处理 WebSocket 请求出错: {}", remote_ip, uuid, e.what());
        }

        if (ws_res.body().size() > 0)
            co_await ws.async_write(asio::buffer(ws_res.body()));
    }

    // 移除会话
    co_await server.session_.remove_session(remote_ip);
    log::info("下线 - 用户下线 - 地址 {}，UUID {} 以 WebSocket 方式下线", remote_ip, uuid);

    auto [ec_close] = co_await ws.async_close(beast::websocket::close_code::normal, asio::as_tuple);

    if (ec_close && ec_close != boost::asio::ssl::error::stream_truncated)
    {
        log::error("错误 - WebSocket 会话处理 - 地址 {}，UUID {} 关闭 WebSocket 连接出错: {}", remote_ip, uuid, ec_close.message());
        throw boost::system::system_error{ec_close};
    }
    beast::get_lowest_layer(stream).close(); // 及时关闭连接
    co_return;
}

asio::awaitable<void> fast::net::Server::listen(fast::net::task_group &task_group, asio::ssl::context &ctx,
                                                asio::net::tcp::endpoint endpoint, beast::string_view doc_root,
                                                Protocol protocol)
{
    log::info("启动 - 监听服务 - 正在进行");

    asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;
    auto executor = co_await asio::this_coro::executor;

    log::info("创建 - 监听器 - 正在创建");
    // auto acceptor = asio::net::tcp::acceptor{executor, endpoint};
    acceptor_ = std::make_unique<asio::net::tcp::acceptor>(executor, endpoint);
    log::info("创建 - 监听器 - 已完成");

    co_await asio::this_coro::reset_cancellation_state(asio::enable_total_cancellation());

    log::info("启动 - 监听服务 - 开始监听连接");
    while (cs.cancelled() == asio::cancellation_type::none)
    {
        auto socket_executor = asio::make_strand(executor);

        log::info("接受 - 新连接 - 正在等待新连接");
        asio::net::tcp::socket socket = co_await acceptor_->async_accept(socket_executor, asio::use_awaitable);
        auto remote_ip = socket.remote_endpoint().address().to_string();
        log::info("接受 - 新连接 - 已接受来自 {} 的连接", remote_ip);

        log::info("管理 - 会话 - 正在为 {} 创建或获取会话", remote_ip);
        auto session = session_.get_or_create_session(remote_ip);
        log::info("管理 - 会话 - 已为 {} 创建或获取会话", remote_ip);

        log::info("启动 - 会话处理 - 正在为 {} 启动会话处理协程", remote_ip);
        asio::co_spawn(std::move(socket_executor),
                       detect_session(beast::tcp_stream(std::move(socket)), ctx, doc_root, protocol),
                       task_group.adapt([remote_ip, this](std::exception_ptr e) {
                           asio::co_spawn(ioc_, session_.remove_session(remote_ip), asio::detached);
                           if (e)
                           {
                               try
                               {
                                   std::rethrow_exception(e);
                               }
                               catch (std::exception &e)
                               {
                                   log::error("错误 - 会话处理 - 处理 {} 的会话时出错: {}", remote_ip, e.what());
                               }
                           }
                       }));
        log::info("启动 - 会话处理 - 已为 {} 启动会话处理协程", remote_ip);
    }
    log::info("停止 - 监听服务 - 监听服务已停止");
}

asio::awaitable<void> fast::net::Server::handle_signals(fast::net::task_group &task_group, asio::io_context &ioc)
{
    auto executor = co_await asio::this_coro::executor;
    log::info("初始化 - 信号处理 - 正在创建信号集");
    auto signal_set = asio::signal_set{executor, SIGINT, SIGTERM};
    log::info("初始化 - 信号处理 - 信号集创建完成");

    log::info("等待 - 信号接收 - 正在等待信号");
    auto sig = co_await signal_set.async_wait();
    log::info("接收 - 信号接收 - 收到信号: {}", sig);

    log::info("执行 - 任务取消 - 正在优雅地取消子任务");
    task_group.emit(asio::cancellation_type::terminal);
    log::info("执行 - 任务取消 - 已发出全面取消信号");

    log::info("等待 - 任务完成 - 正在等待所有子任务完成");
    auto [ec] = co_await task_group.async_wait(asio::as_tuple(asio::cancel_after(std::chrono::seconds{10})));

    if (ec == boost::asio::error::operation_aborted)
    {
        log::info("超时 - 任务完成 - 优雅取消在 10 秒后超时");
        log::info("执行 - 任务取消 - 正在发送终止取消信号");
        task_group.emit(asio::cancellation_type::terminal);
        log::info("执行 - 任务取消 - 已发出终止取消信号");
        log::info("等待 - 任务完成 - 再次等待所有子任务完成");
        co_await task_group.async_wait();
    }

    log::info("完成 - 任务完成 - 子任务已完成");

    log::info("停止 - io_context - 正在停止 io_context");
    ioc.stop();
    log::info("停止 - io_context - io_context 已停止");
}

} // namespace fast::net
// namespace fast::net
