#include <asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/mysql.hpp>
#include <boost/redis.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

using namespace std::chrono_literals;
using namespace boost;
namespace beast = boost::beast;
namespace net = boost::asio;
namespace mysql = boost::mysql;
namespace redis = boost::redis;


struct OptionParams
{
    std::string host = "localhost";
    unsigned short port = 3306;
    std::string username;
    std::string password;
    std::string database;
    boost::mysql::ssl_mode ssl{boost::mysql::ssl_mode::disable};
    bool multi_queries{true};
    std::size_t initial_size{1};
    std::size_t max_size{2};
    std::chrono::seconds connect_timeout{1};
    std::chrono::seconds retry_interval{1};
    std::chrono::hours ping_interval{1};
    std::chrono::seconds ping_timeout{1};
    bool thread_safe{true};
};

boost::mysql::pool_params CreateMysqlOption(const OptionParams &params)
{
    boost::mysql::pool_params mysql_params;
    mysql_params.server_address.emplace_host_and_port(params.host, params.port);
    mysql_params.username = params.username;
    mysql_params.password = params.password;
    mysql_params.database = params.database;
    mysql_params.ssl = params.ssl;
    mysql_params.multi_queries = params.multi_queries;
    mysql_params.initial_size = params.initial_size;
    mysql_params.max_size = params.max_size;
    mysql_params.connect_timeout = params.connect_timeout;
    mysql_params.retry_interval = params.retry_interval;
    mysql_params.ping_interval = params.ping_interval;
    mysql_params.ping_timeout = params.ping_timeout;
    mysql_params.thread_safe = params.thread_safe;
    return mysql_params;
}

struct Session
{
    ~Session()
    {
        try
        {
            // No need to explicitly disconnect conn, connection_pool manages connections
            if (redis_client.is_connected())
                redis_client.disconnect();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Session cleanup failed: " << e.what() << std::endl;
        }
    }

   boost::mysql::connection_pool mysql_pool;
    boost::redis::client redis_client;

    Session(net::io_context &ioc)
    {
        try
        {
            OptionParams params;
            params.host = "127.0.0.1";    // 配置 MySQL 主机
            params.port = 3306;           // 配置 MySQL 端口
            params.username = "user";     // 配置 MySQL 用户名
            params.password = "password"; // 配置 MySQL 密码
            params.database = "dbname";   // 配置 MySQL 数据库名
            // 可以根据需要配置 OptionParams 中的其他参数，例如连接池大小、超时等

            boost::mysql::connection_pool pool(ioc, CreateMysqlOption(params));

            mysql_pool.async_run(asio::detached); // Start the connection pool

            redis_client.connect("tcp://127.0.0.1:6379");
        }
        catch (const std::exception &e)
        {
            std::cerr << "Session init failed: " << e.what() << std::endl;
        }
    }
};

enum class Protocol
{
    HTTP,
    HTTPS,
    WS,
    WSS
};

class RouteManager
{
  public:
    using HttpRequest = boost::beast::http::request<boost::beast::string_body>;
    using WebSocketFrame = boost::beast::websocket::frame<boost::beast::string_body>;

    using HttpHandler = std::function<asio::awaitable<void(HttpRequest &, Session &)>>;
    using WsHandler = std::function<asio::awaitable<void(WebSocketFrame &, Session &)>>;

    void add_route(const std::string &path, HttpHandler http, WsHandler ws)
    {
        http_routes[path] = std::move(http);
        ws_routes[path] = std::move(ws);
    }

    asio::awaitable<void> process_request(boost::beast::http::request<boost::beast::string_body> &req,
                                          boost::beast::websocket::frame<boost::beast::string_body> &frame, Session &session,
                                          Protocol proto)
    {

        try
        {
            if (proto == Protocol::HTTP || proto == Protocol::HTTPS)
            {
                auto it = http_routes.find(req.target());
                if (it != http_routes.end())
                {
                    co_await it->second(req, session);
                    return; // Important: Return after successful handler execution
                }

                boost::beast::http::response<boost::beast::string_body> res{boost::beast::http::status::not_found};
                res.body() = "404 Not Found";
                res.set(boost::beast::http::field::content_type, "text/plain"); // Set content type
                res.keep_alive(req.keep_alive());                        // Keep alive as requested
                res.version(req.version());                              // Set HTTP version
                co_await boost::beast::http::async_write(boost::beast::tcp_stream(req.socket().get_executor()),
                                                  res); // Use tcp_stream
            }
            else
            {
                auto it = ws_routes.find(frame.payload());
                if (it != ws_routes.end())
                {
                    co_await it->second(frame, session);
                    co_return; // Important: Return after successful handler execution
                }

                frame.payload() = "404 Unknown Command";
                boost::beast::websocket::stream<boost::beast::tcp_stream> ws_stream(
                    boost::beast::tcp_stream(req.socket().get_executor())); // Create ws_stream
                co_await ws_stream.async_write(frame);               // Use ws_stream for websocket write
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Request error: " << e.what() << std::endl;
            co_return;
        }
    }

  private:
    std::unordered_map<std::string, HttpHandler> http_routes;
    std::unordered_map<std::string, WsHandler> ws_routes;
};

template <typename SocketType> class ProtocolHandler
{
  public:
    explicit ProtocolHandler(net::io_context &io, RouteManager &router) : io(io), router(router)
    {
    }

    asio::awaitable<void> handle(std::shared_ptr<SocketType> socket, Protocol proto, std::shared_ptr<Session> session)
    {

        try
        {
            if (proto == Protocol::HTTP || proto == Protocol::HTTPS)
            {
                boost::beast::http::request<boost::beast::http::string_body> req;
                boost::beast::flat_buffer buffer; // Buffer for reading HTTP request
                co_await boost::beast::http::async_read(*socket, buffer, req);

                co_await router.process_request(req, boost::beast::websocket::frame<boost::beast::http::string_body>(), *session, proto);

                // HTTP Response is handled in process_request now, no default response here
                // boost::beast::http::response<boost::beast::string_body> res{boost::beast::http::status::ok};
                // res.body() = "Hello from " + std::string(proto == Protocol::HTTPS ? "HTTPS" : "HTTP");
                // res.set(boost::beast::http::field::content_type, "text/plain");
                // res.keep_alive(req.keep_alive());
                // res.version(req.version());
                // co_await boost::beast::http::async_write(*socket, res);
            }
            else
            {
                auto stream = std::make_shared<boost::asio::ssl::stream<SocketType>>(std::move(*socket));
                co_await stream->handshake(
                    boost::asio::ssl::stream_base::server); // Use server handshake for SSL server

                boost::beast::websocket::stream<boost::asio::ssl::stream<SocketType>> ws(*stream);
                boost::beast::flat_buffer buffer; // Buffer for websocket

                while (true)
                {
                    if (g_signal_received)
                        break;

                    boost::beast::websocket::frame<boost::beast::http::string_body> frame;
                    net::error_code ec;
                    co_await ws.async_read(buffer, frame, ec); // Use buffer for websocket read

                    if (ec == net::error::eof || ec == boost::beast::websocket::error::closed ||
                        frame.opcode() == boost::beast::websocket::opcode::close)
                    {
                        break;
                    }
                    if (ec)
                    {
                        std::cerr << "WebSocket read error: " << ec.message() << std::endl;
                        break;
                    }

                    co_await router.process_request(boost::beast::http::request<boost::beast::string_body>(), frame, *session, proto);

                    boost::beast::flat_buffer write_buffer;   // Separate buffer for write
                    frame.payload() = frame.payload(); // Echo back the payload
                    co_await ws.async_write(
                        net::buffer(frame.payload().data(), frame.payload().size())); // Write using buffer
                }
            }
        }
        catch (const boost::system::error_code &ec)
        {
            if (ec != net::error::operation_aborted && ec != boost::beast::websocket::error::closed)
            {
                std::cerr << "Protocol error: " << ec.message() << std::endl;
            }
        }
    }

  private:
    net::io_context &io;
    RouteManager &router;
};

class Server
{
  public:
    Server(net::io_context &io, RouteManager &router) : io(io), router(router)
    {
    }

    template <typename SocketType> void listen(uint16_t port, Protocol proto)
    {
        auto acceptor =
            std::make_shared<net::ip::tcp::acceptor>(io, {net::ip::tcp::v4(), port}); // Bind acceptor to port
        acceptor->listen();                                                           // Start listening
        boost::asio::co_spawn(io, [this, port, acceptor, proto](auto self) {
            while (true)
            {
                auto socket = std::make_shared<SocketType>(io);
                net::error_code ec;
                co_await acceptor->async_accept(*socket, net::use_awaitable); // Use awaitable for accept

                if (!ec)
                {
                    std::cout << "Accepted connection on port " << port << " protocol " << static_cast<int>(proto)
                              << std::endl;
                    handle_connection(std::move(socket), proto);
                }
                else if (ec != net::error::operation_aborted)
                {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                }
            }
        });
    }

    void handle_connection(std::shared_ptr<net::ip::tcp::socket> socket, Protocol proto)
    {
        auto session = std::make_shared<Session>(io); // Pass io_context to Session
        if (proto == Protocol::HTTPS || proto == Protocol::WSS)
        {
            auto ssl_context = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_server);
            ssl_context->use_certificate_chain_file("path/to/your/server.crt"); // Replace with your certificate path
            ssl_context->use_private_key_file("path/to/your/server.key",
                                              boost::asio::ssl::context::pem); // Replace with your key path

            auto ssl_socket = std::make_shared<boost::asio::ssl::stream<net::ip::tcp::socket>>(*socket, *ssl_context);

            co_spawn(io, [this, socket = ssl_socket, proto, session](auto) {
                ProtocolHandler<boost::asio::ssl::stream<net::ip::tcp::socket>> handler(io, router);
                co_await handler.handle(socket, proto, session);
            });
        }
        else
        {
            co_spawn(io, [this, socket, proto, session](auto) {
                ProtocolHandler<net::ip::tcp::socket> handler(io, router);
                co_await handler.handle(socket, proto, session);
            });
        }
    }

  private:
    net::io_context &io;
    RouteManager &router;
};



asio::awaitable<void> mysql_handler(RouteManager::HttpRequest &req, Session &session)
{
    auto conn = session.mysql_pool.get_connection(); // Get connection from pool
    if (!conn)
    {
        req.body() = "Failed to get MySQL connection from pool.";
        co_return;
    }
    try
    {
        boost::mysql::statement stmt = conn->prepare_statement("SELECT 'Hello from boost.mysql pool using OptionParams!'");
        auto result = conn->execute(stmt);
        std::string mysql_response;
        for (const auto &row : result)
        {
            mysql_response += boost::mysql::to_string(row.at(0)) + "\n";
        }
        req.body() = "MySQL Data: " + mysql_response;
    }
    catch (const std::exception &e)
    {
        req.body() = "MySQL Query Error: " + std::string(e.what());
    }
    // conn is automatically returned to the pool when it goes out of scope
    co_return;
}

asio::awaitable<void> ws_echo_handler(RouteManager::WebSocketFrame &frame, Session &session)
{
    session.redis_client.set("websocket_data", frame.payload());
    frame.payload() = "Echo: " + frame.payload();
    co_return;
}

int main()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    net::io_context io;
    RouteManager router;
    Server server(io, router);

    router.add_route("/api/data",
                     mysql_handler,  // Use the mysql_handler
                     ws_echo_handler // Use the ws_echo_handler
    );

    server.listen<net::ip::tcp::socket>(8080, Protocol::HTTP);
    server.listen<net::ip::tcp::socket>(8081, Protocol::WS);

    // For HTTPS and WSS, you need to provide certificate and key paths
    server.listen<boost::asio::ssl::stream<net::ip::tcp::socket>>(8443, Protocol::HTTPS);
    server.listen<boost::asio::ssl::stream<net::ip::tcp::socket>>(8444, Protocol::WSS);

    std::cout << "Server listening on ports 8080 (HTTP), 8081 (WS), 8443 (HTTPS), 8444 (WSS)" << std::endl;
    while (!g_signal_received && !io.stopped())
    {                 // Check if io_context is stopped as well
        io.run_one(); // Use run_one to allow io_context to stop
        std::this_thread::sleep_for(1ms);
    }

    io.stop(); // Explicitly stop io_context
    std::cout << "Server stopped" << std::endl;
    return 0;
}