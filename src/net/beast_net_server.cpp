// beast_net_server.cpp
module;

#include "common/server_certi.hpp"
#include <boost/asio/error.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/system/error_code.hpp>

import std;
import mysql_connect;
module beast_net_server;
import session_data;
import router_process;
import net_common;
import boost;

namespace fast::net
{
    task_group::task_group(asio::any_io_executor exec)
        : cv_{std::move(exec), asio::steady_timer::time_point::max()}
    {
    }

    template <typename CompletionToken>
    auto task_group::adapt(CompletionToken&& completion_token)
    {
        auto lg = std::lock_guard{mtx_};
        auto cs = css_.emplace(css_.end());

        class remover
        {
            task_group* tg_;
            decltype(css_)::iterator cs_;

        public:
            remover(task_group* tg, decltype(css_)::iterator cs) : tg_{tg}, cs_{cs}
            {
            }

            remover(remover&& other) noexcept
                : tg_{std::exchange(other.tg_, nullptr)}, cs_{other.cs_}
            {
            }

            ~remover()
            {
                if (tg_)
                {
                    auto lg = std::lock_guard{tg_->mtx_};
                    tg_->css_.erase(cs_); // 删除取消槽
                    if (tg_->css_.empty()) // 正确判断 css_ 是否为空
                        tg_->cv_.cancel(); // 如果 css_ 为空，则发送信号
                }
            }
        };

        return asio::bind_cancellation_slot(
            cs->slot(), asio::consign(std::forward<CompletionToken>(completion_token),
                                      remover{this, cs}));
    }

    void task_group::emit(asio::cancellation_type type)
    {
        auto lg = std::lock_guard{mtx_};
        for (auto& cs : css_)
            cs.emit(type);
    }

    template <typename  CompletionToken>
    auto task_group::async_wait(CompletionToken&& completion_token)
    {
        return asio::async_compose<CompletionToken, void(boost::system::error_code)>(
            [this, scheduled = false](auto&& self,
                                      boost::system::error_code ec = {}) mutable
            {
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


    Server::Server(const asio::address& address,
                   unsigned short port,
                   const std::string& doc_root,
                   int threads,
                   Protocol protocol,
                   mysql::connection_pool& pool)
        : endpoint_(address, port), doc_root_(doc_root), threads_(threads),
          protocol_(protocol), ioc_(threads), ctx_(asio::ssl::context::tlsv12),
          task_group_(ioc_.get_executor()),database_(pool), session_(ioc_,database_)
    {
        if (protocol_ == Protocol::HTTPS || protocol_ == Protocol::WebSockets)
        {
            load_server_certificate(ctx_);
        }
    }

    void Server::run()
    {
        asio::co_spawn(asio::make_strand(ioc_),
                       listen(task_group_, ctx_, endpoint_, doc_root_, protocol_),
                       task_group_.adapt([](std::exception_ptr e)
                       {
                           if (e)
                           {
                               try
                               {
                                   std::rethrow_exception(e);
                               }
                               catch (std::exception& e)
                               {
                                   std::cerr << "Error in listener: " << e.what() << "\n";
                               }
                           }
                       }));

        asio::co_spawn(asio::make_strand(ioc_), handle_signals(task_group_, ioc_),
                       asio::detached);

        std::vector<std::thread> v;
        v.reserve(threads_ - 1);
        for (auto i = threads_ - 1; i > 0; --i)
            v.emplace_back([&ioc = ioc_] { ioc.run(); });

        ioc_.run();

        for (auto& t : v)
            t.join();
    }

    asio::awaitable<void> fast::net::Server::detect_session(beast::tcp_stream stream,
                                                            asio::ssl::context& ctx,
                                                            beast::string_view doc_root,
                                                            Protocol protocol)
    {
        beast::flat_buffer buffer;

        co_await asio::this_coro::reset_cancellation_state(
            asio::enable_total_cancellation(), asio::enable_terminal_cancellation());

        co_await asio::this_coro::throw_if_cancelled(false);

        stream.expires_after(std::chrono::seconds(30));

        if (protocol == Protocol::HTTPS || protocol == Protocol::WebSockets)
        {
            asio::ssl::stream<beast::tcp_stream> ssl_stream{std::move(stream), ctx};

            co_await ssl_stream.async_handshake(asio::ssl::stream_base::server,
                                                buffer.data());

            if (protocol == Protocol::WebSockets)
            {
                beast::http::request<beast::http::string_body> req;
                co_await run_websocket_session(ssl_stream, buffer, std::move(req),
                                               doc_root, *this);
            }
            else
            {
                co_await run_session(ssl_stream, buffer, doc_root, *this);
            }

            if (!ssl_stream.lowest_layer().is_open())
                co_return;

            auto [ec] = co_await ssl_stream.async_shutdown(asio::as_tuple);
            if (ec && ec !=boost::asio::ssl::error::stream_truncated)
                throw boost::system::system_error{ec};
        }
        else
        {
            if (protocol == Protocol::WebSocket)
            {
                beast::http::request<beast::http::string_body> req;
                co_await run_websocket_session(stream, buffer, std::move(req), doc_root,
                                               *this);
            }
            else
            {
                co_await run_session(stream, buffer, doc_root, *this);
            }

            if (!stream.socket().is_open())
                co_return;

            stream.socket().shutdown(asio::net::tcp::socket::shutdown_send);
        }
    }

    template <fast::net::ValidStream Stream>
    asio::awaitable<void>
    fast::net::Server::run_session(Stream& stream, beast::flat_buffer& buffer,
                                   beast::string_view doc_root, Server& server)
    {
        static_assert(std::is_same_v<Stream, beast::tcp_stream> ||
                      std::is_same_v<Stream, asio::ssl::stream<beast::tcp_stream>>,
                      "Unsupported stream type");

         asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;

        while (cs.cancelled()== asio::cancellation_type::none)
        {
            beast::http::request_parser<beast::http::string_body> parser;
            parser.body_limit(10000);

            auto [ec, _] =
                co_await beast::http::async_read(stream, buffer, parser, asio::as_tuple);

            if (ec == boost::beast::http::error::end_of_stream)
                co_return;

            if (ec)
            {
                beast::http::response<beast::http::string_body> res{
                    beast::http::status::internal_server_error,
                    parser.get().version()
                };
                res.body() = "Read error: " + ec.message();
                res.prepare_payload();
                co_await beast::http::async_write(stream, res);
                co_return;
            }

            if (beast::websocket::is_upgrade(parser.get()))
            {
                beast::get_lowest_layer(stream).expires_never();
                beast::http::request<beast::http::string_body> req = parser.release();
                co_await run_websocket_session(stream, buffer, std::move(req), doc_root,
                                               server);
                co_return;
            }

            auto req = parser.release();

            if (auto target = std::string(req.target());
                target.find("/static/") == 0)
            {
                co_await server.router_.handle_static_request(stream, buffer, doc_root,
                                                              std::move(req));
                continue;
            }

            auto session = server.session_.get_or_create_session(beast::get_lowest_layer(stream)
                                                        .socket()
                                                        .remote_endpoint()
                                                        .address()
                                                        .to_string());

            beast::http::response<beast::http::string_body> res{beast::http::status::ok, req.version()};

            try
            {
                co_await server.router_.handle_request(req, res, session);
            }
            catch (std::exception& e)
            {
                res = server_error(std::move(req), e.what());
            }

            if (!res.keep_alive())
            {
                co_await beast::http::async_write(stream, std::move(res));
                beast::get_lowest_layer(stream).close(); // 及时关闭连接
                co_return;
            }

            co_await beast::http::async_write(stream, std::move(res));
        }
        beast::get_lowest_layer(stream).close(); // 及时关闭连接
    }

    template <fast::net::ValidStream Stream>
    asio::awaitable<void>
    fast::net::Server::run_websocket_session(Stream& stream, beast::flat_buffer& buffer,
                                             beast::http::request<beast::http::string_body> req,
                                             beast::string_view doc_root, Server& server)
    {
        static_assert(std::is_same_v<Stream, beast::tcp_stream> ||
                      std::is_same_v<Stream, asio::ssl::stream<beast::tcp_stream>>,
                      "Unsupported stream type");

        auto cs = co_await asio::this_coro::cancellation_state;
        auto ws = beast::websocket::stream<Stream&>{stream};

        ws.set_option(
            beast::websocket::stream_base::timeout::suggested(beast::role_type::server));

        ws.set_option(
            beast::websocket::stream_base::decorator([](beast::websocket::response_type& res)
            {
                res.set(beast::http::field::server,
                        "asio advanced-server-flex");
            }));

        co_await ws.async_accept(req);

        while (cs.cancelled()== asio::cancellation_type::none)
        {
            buffer.clear();
            auto [ec, bytes_transferred] =
                co_await ws.async_read(buffer, asio::as_tuple);

            if (ec == boost::beast::websocket::error::closed || ec == boost::asio::ssl::error::stream_truncated)
                co_return;

            if (ec)
                throw boost::system::system_error{ec};

            beast::http::request<beast::http::string_body> ws_req;
            ws_req.method(beast::http::verb::post);
            ws_req.target("/ws");
            ws_req.body() = beast::buffers_to_string(buffer.data());
            ws_req.set(beast::http::field::content_type, "text/plain");

            beast::http::response<beast::http::string_body> ws_res{
                beast::http::status::ok,
                ws_req.version()
            };
            auto session = server.session_.get_or_create_session(beast::get_lowest_layer(stream)
                                                        .socket()
                                                        .remote_endpoint()
                                                        .address()
                                                        .to_string());

            try
            {
                co_await server.router_.handle_request(ws_req, ws_res, session);
            }
            catch (std::exception& e)
            {
                ws_res = fast::net::server_error(std::move(ws_req), e.what());
            }

            if (ws_res.body().size() > 0)
                co_await ws.async_write(asio::buffer(ws_res.body()));
        }

        auto [ec_close] =
            co_await ws.async_close(beast::websocket::close_code::normal, asio::as_tuple);

        if (ec_close && ec_close != boost::asio::ssl::error::stream_truncated)
            throw boost::system::system_error{ec_close};
        beast::get_lowest_layer(stream).close(); // 及时关闭连接
        co_return;
    }

    asio::awaitable<void> fast::net::Server::listen(fast::net::task_group& task_group, asio::ssl::context& ctx,
                                                    asio::net::tcp::endpoint endpoint,
                                                    beast::string_view doc_root,
                                                    Protocol protocol)
    {
        asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;
        auto executor = co_await asio::this_coro::executor;
        //auto acceptor = asio::net::tcp::acceptor{executor, endpoint};
        acceptor_ = std::make_unique<asio::net::tcp::acceptor>(executor, endpoint);

        co_await asio::this_coro::reset_cancellation_state(
            asio::enable_total_cancellation());

        while (cs.cancelled()== asio::cancellation_type::none)
        {
            auto socket_executor = asio::make_strand(executor);
            asio::net::tcp::socket socket = co_await acceptor_->async_accept(
                socket_executor, asio::use_awaitable);

            auto remote_ip = socket.remote_endpoint().address().to_string();
            auto session = session_.create_session(remote_ip);

            asio::co_spawn(std::move(socket_executor),
                           detect_session(beast::tcp_stream(std::move(socket)), ctx,
                                          doc_root, protocol),
                           task_group.adapt([remote_ip, this](std::exception_ptr e)
                           {
                               asio::co_spawn(ioc_, session_.remove_session(remote_ip),
                                              asio::detached);
                               if (e)
                               {
                                   try
                                   {
                                       std::rethrow_exception(e);
                                   }
                                   catch (std::exception& e)
                                   {
                                       // 不管这个错误
                                       std::cerr << "Error in session: " << e.what();
                                   }
                               }
                           }));
        }
    }

    asio::awaitable<void> fast::net::Server::handle_signals(fast::net::task_group& task_group,
                                                            asio::io_context& ioc)
    {
        auto executor = co_await asio::this_coro::executor;
        auto signal_set = asio::signal_set{executor, SIGINT, SIGTERM};

        auto sig = co_await signal_set.async_wait();

        std::cout << "Received signal: " << sig << "\n";
        std::cout << "Gracefully cancelling child tasks...\n";

        task_group.emit(asio::cancellation_type::terminal);
        std::cout << "Emitted total cancellation signal.\n"; // Logging (Improvement 4)


        // 等待所有任务完成
        auto [ec] = co_await task_group.async_wait(
            asio::as_tuple(asio::cancel_after(std::chrono::seconds{10})));

        if (ec == boost::asio::error::operation_aborted)
        {
            std::cout << "Graceful cancellation timed out after 5 seconds.\n"; // Logging (Improvement 4)
            std::cout << "Sending a terminal cancellation signal...\n";
            task_group.emit(asio::cancellation_type::terminal);
            std::cout << "Emitted terminal cancellation signal.\n"; // Logging (Improvement 4)
            co_await task_group.async_wait();
        }

        std::cout << "Child tasks completed.\n"; // Logging (Improvement 4)

        ioc.stop();
        std::cout << "io_context stopped.\n"; // Logging (Improvement 4)
    }
}
// namespace fast::net
