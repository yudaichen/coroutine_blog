// asio.ixx
module; // 全局模块片段

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/use_awaitable.hpp>

// 引入 Boost.Beast 头文件
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>

// 引入 Boost.MySQL 头文件
#include <boost/mysql.hpp>
#include <boost/mysql/connection.hpp>
#include <boost/mysql/handshake_params.hpp>
#include <boost/mysql/results.hpp>

// 引入 Boost.Redis 头文件
#include <boost/redis.hpp>
#include <boost/redis/connection.hpp>
#include <boost/redis/request.hpp>
#include <boost/redis/response.hpp>

export module asio;

// ==================== 主命名空间 ====================
export namespace asio {
// --- 核心组件导出 ---
using boost::asio::any_completion_executor;
using boost::asio::any_io_executor;
using boost::asio::async_connect;
using boost::asio::awaitable;
using boost::asio::bad_executor;
using boost::asio::buffer;
using boost::asio::cancellation_signal;
using boost::asio::cancellation_slot;
using boost::asio::cancellation_state;
using boost::asio::cancellation_type;
using boost::asio::co_spawn;
using boost::asio::connect;
using boost::asio::coroutine;
using boost::asio::deferred;
using boost::asio::detached;
using boost::asio::detached_t;
using boost::asio::dynamic_buffer;
using boost::asio::execution_context;
using boost::asio::executor;
using boost::asio::executor_arg_t;
using boost::asio::invalid_service_owner;
using boost::asio::io_context;
using boost::asio::multiple_exceptions;
using boost::asio::post;
using boost::asio::service_already_exists;
using boost::asio::socket_base;
using boost::asio::static_thread_pool;
using boost::asio::steady_timer;
using boost::asio::system_context;
using boost::asio::system_executor;
using boost::asio::thread_pool;
using boost::asio::ip::address;

using boost::asio::append;
using boost::asio::as_tuple;
using boost::asio::async_compose;
using boost::asio::bind_cancellation_slot;
using boost::asio::cancel_after;
using boost::asio::consign;
using boost::asio::default_completion_token_t;
using boost::asio::detached;
using boost::asio::enable_terminal_cancellation;
using boost::asio::enable_total_cancellation;
using boost::asio::make_strand;
using boost::asio::signal_set;

// --- 错误处理子命名空间 ---
namespace error {
using boost::asio::error::make_error_code;
}

// --- 协程相关子命名空间 ---
namespace this_coro {

BOOST_ASIO_INLINE_VARIABLE constexpr boost::asio::this_coro::
    cancellation_state_t cancellation_state;
BOOST_ASIO_INLINE_VARIABLE constexpr boost::asio::this_coro::executor_t
    executor;
// using boost::asio::this_coro::cancellation_state;
// using boost::asio::this_coro::executor;
using boost::asio::this_coro::reset_cancellation_state;
using boost::asio::this_coro::throw_if_cancelled;
} // namespace this_coro

// ==================== 封装 use_awaitable ====================
#if defined(GENERATING_DOCUMENTATION)
BOOST_ASIO_INLINE_VARIABLE constexpr boost::asio::use_awaitable_t<>
    use_awaitable;
#else
BOOST_ASIO_INLINE_VARIABLE constexpr boost::asio::use_awaitable_t<>
    use_awaitable(0, 0, 0);
#endif

// ==================== 网络支持 ====================
namespace net {
namespace ip {
address make_address(const std::string &str) {
  return boost::asio::ip::make_address(str);
}
} // namespace ip

// --- TCP 协议实现 ---
namespace tcp {
using boost::asio::ip::tcp::socket::shutdown_send;

// 使用 Boost.Asio 的 tcp 类
using protocol = boost::asio::ip::tcp;

// 核心套接字类型
template <typename Protocol = protocol>
using basic_socket = boost::asio::basic_socket<Protocol>;

template <typename Protocol = protocol>
using basic_socket_acceptor = boost::asio::basic_socket_acceptor<Protocol>;

template <typename Protocol = protocol>
using basic_stream_socket = boost::asio::basic_stream_socket<Protocol>;

template <typename Protocol = protocol>
using basic_resolver = boost::asio::ip::basic_resolver<Protocol>;

// 预定义实例
using socket = basic_stream_socket<protocol>;
using acceptor = basic_socket_acceptor<protocol>;
using endpoint = boost::asio::ip::basic_endpoint<protocol>;
using resolver = basic_resolver<protocol>;
using resolver_query = boost::asio::ip::basic_resolver_query<protocol>;
using resolver_results = boost::asio::ip::basic_resolver_results<protocol>;

// 工厂函数
inline socket make_socket(io_context &ctx) { return socket(ctx); }

inline acceptor make_acceptor(io_context &ctx, const endpoint &ep) {
  acceptor a(ctx);
  a.open(ep.protocol());
  a.set_option(socket_base::reuse_address(true));
  a.bind(ep);
  a.listen();
  return a;
}

// 增强操作函数
template <typename Protocol, typename... Args>
auto async_connect(basic_stream_socket<Protocol> &sock, Args &&...args) {
  return boost::asio::async_connect(sock, std::forward<Args>(args)...);
}

// 封装 async_read_until
template <typename AsyncReadStream, typename DynamicBuffer,
          typename CompletionToken>
auto async_read_until(AsyncReadStream &stream, DynamicBuffer &&buffer,
                      const std::string &delim, CompletionToken &&token) {
  return boost::asio::async_read_until(
      stream, std::forward<DynamicBuffer>(buffer), delim,
      std::forward<CompletionToken>(token));
}

// 封装 async_write
template <typename AsyncWriteStream, typename ConstBufferSequence,
          typename CompletionToken>
auto async_write(AsyncWriteStream &stream, const ConstBufferSequence &buffers,
                 CompletionToken &&token) {
  return boost::asio::async_write(stream, buffers,
                                  std::forward<CompletionToken>(token));
}

// 导出 no_delay 选项
#if defined(GENERATING_DOCUMENTATION)
typedef implementation_defined no_delay;
#else
typedef boost::asio::detail::socket_option::boolean<
    BOOST_ASIO_OS_DEF(IPPROTO_TCP), BOOST_ASIO_OS_DEF(TCP_NODELAY)>
    no_delay;
#endif
} // namespace tcp

// --- UDP 协议实现 ---
namespace udp {
using protocol = boost::asio::ip::udp;

template <typename Protocol = protocol>
using basic_socket = boost::asio::basic_socket<Protocol>;

template <typename Protocol = protocol>
using basic_endpoint = boost::asio::ip::basic_endpoint<Protocol>;

using socket = basic_socket<protocol>;
using endpoint = basic_endpoint<protocol>;
} // namespace udp
} // namespace net

// ==================== SSL/TLS 支持 ====================
namespace ssl {
using boost::asio::ssl::context;
using boost::asio::ssl::context_base;
using boost::asio::ssl::host_name_verification;
using boost::asio::ssl::stream;
using boost::asio::ssl::stream_base;
using boost::asio::ssl::verify_context;

// SSL over TCP 特化类型
template <typename Protocol = net::tcp::protocol>
using ssl_socket = stream<net::tcp::basic_stream_socket<Protocol>>;

// SSL 工厂函数
template <typename Protocol>
ssl_socket<Protocol>
make_ssl_socket(net::tcp::basic_stream_socket<Protocol> &sock, context &ctx) {
  return ssl_socket<Protocol>(std::move(sock), ctx);
}

// SSL 错误处理
namespace error {
using boost::asio::ssl::error::make_error_code;
using boost::asio::ssl::error::stream_errors;
} // namespace error
} // namespace ssl
} // namespace asio

// ==================== Boost.Beast 支持 ====================
export namespace beast {
// --- 核心组件导出 ---
using boost::beast::error_code;
using boost::beast::file_mode;
using boost::beast::flat_buffer;
using boost::beast::role_type;
using boost::beast::string_view;
using boost::beast::tcp_stream;

using boost::beast::buffers_to_string;
using boost::beast::get_lowest_layer;

// --- HTTP 支持 ---
namespace http {
using boost::beast::http::dynamic_body;
using boost::beast::http::empty_body;
using boost::beast::http::field;
using boost::beast::http::file_body;
using boost::beast::http::request;
using boost::beast::http::response;
using boost::beast::http::status;
using boost::beast::http::string_body;
using boost::beast::http::verb;

using boost::beast::http::request_parser;
// HTTP 工厂函数
template <typename Body = string_body,
          typename Fields = boost::beast::http::fields>
auto make_request(verb method, std::string target, unsigned version = 11) {
  return request<Body, Fields>(method, target, version);
}

// HTTP 异步操作
template <typename Stream, typename Request, typename CompletionToken>
auto async_write(Stream &stream, Request &&req, CompletionToken &&token) {
  return boost::beast::http::async_write(stream, std::forward<Request>(req),
                                         std::forward<CompletionToken>(token));
}

template <typename Stream, typename Request>
auto async_write(Stream &stream, Request &&req) {
  return boost::beast::http::async_write(stream, std::forward<Request>(req),
                                         asio::use_awaitable);
}

template <typename Stream, typename Response, typename CompletionToken>
auto async_read(Stream &stream, flat_buffer &buffer, Response &res,
                CompletionToken &&token) {
  return boost::beast::http::async_read(stream, buffer, res,
                                        std::forward<CompletionToken>(token));
}

template <typename Stream, typename Response>
auto async_read(Stream &stream, flat_buffer &buffer, Response &res) {
  return boost::beast::http::async_read(stream, buffer, res,
                                        asio::use_awaitable);
}
} // namespace http

// --- WebSocket 支持 ---
namespace websocket {
using boost::beast::role_type;
using boost::beast::websocket::close_code;
using boost::beast::websocket::is_upgrade;
using boost::beast::websocket::request_type;
using boost::beast::websocket::response_type;
using boost::beast::websocket::stream;
using boost::beast::websocket::stream_base;

// WebSocket 工厂函数
template <typename NextLayer>
auto make_websocket_stream(NextLayer &next_layer) {
  return stream<NextLayer>(next_layer);
}
} // namespace websocket

// --- SSL 支持 ---
namespace ssl {
using boost::beast::ssl_stream;

// SSL 工厂函数
template <typename Protocol>
auto make_ssl_stream(asio::net::tcp::basic_stream_socket<Protocol> &sock,
                     asio::ssl::context &ctx) {
  return ssl_stream<asio::net::tcp::basic_stream_socket<Protocol>>(
      std::move(sock), ctx);
}
} // namespace ssl
} // namespace beast

// ==================== Boost.MySQL 支持 ====================
export namespace mysql {
using boost::mysql::connection;
using boost::mysql::connection_pool;
using boost::mysql::diagnostics;
using boost::mysql::error_code;
using boost::mysql::handshake_params;
using boost::mysql::pool_params;
using boost::mysql::pooled_connection;
using boost::mysql::results;
using boost::mysql::row_view;
using boost::mysql::ssl_mode;
using boost::mysql::statement;
using boost::mysql::tcp_ssl_connection;
using boost::mysql::with_params;

// MySQL 工厂函数
inline auto
make_connection(connection<connection<boost::asio::ssl::stream<
                    boost::asio::basic_stream_socket<boost::asio::ip::tcp>>>>
                    ctx) {
  return connection<tcp_ssl_connection>(std::move(ctx));
}

// MySQL 异步操作
template <typename CompletionToken>
auto async_connect(connection<tcp_ssl_connection> &conn,
                   const std::string &host, const std::string &user,
                   const std::string &password, const std::string &database,
                   CompletionToken &&token) {
  return conn.async_connect(host, user, password, database,
                            std::forward<CompletionToken>(token));
}

template <typename CompletionToken>
auto async_execute(connection<tcp_ssl_connection> &conn,
                   const std::string &query, results &result,
                   CompletionToken &&token) {
  return conn.async_execute(query, result,
                            std::forward<CompletionToken>(token));
}
} // namespace mysql

// ==================== Boost.Redis 支持 ====================
export namespace redis {
using boost::redis::connection;
using boost::redis::request;
using boost::redis::response;

// Redis 工厂函数
inline auto make_connection(asio::io_context &ctx) { return connection(ctx); }

// Redis 异步操作
template <typename CompletionToken>
auto async_execute(connection &conn, const request &req,
                   boost::redis::response<boost::redis::ignore_t> &res,
                   CompletionToken &&token) {
  return conn.async_exec(req, res, std::forward<CompletionToken>(token));
}
} // namespace redis

