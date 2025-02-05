// beast_net_server.ixx
module;

import boost;
export module beast_net_server;

import std;
import mysql_connect;
import small_utils;
import session_data;
import router_process;

export namespace fast::net {


    class task_group {
        std::mutex mtx_;
        asio::steady_timer cv_;
        std::list<asio::cancellation_signal> css_;

    public:
        task_group(asio::any_io_executor exec);

        task_group(task_group const &) = delete;
        task_group(task_group &&) = delete;

        template<typename CompletionToken>
        auto adapt(CompletionToken &&completion_token);

        void emit(asio::cancellation_type type);

        template<typename CompletionToken = asio::default_completion_token_t<asio::any_io_executor>>
        auto async_wait(CompletionToken &&completion_token = asio::default_completion_token_t<asio::any_io_executor>{});
    };

    class Server {
    public:
        enum class Protocol { HTTP, HTTPS, WebSocket, WebSockets };

        Server(
            const asio::address &address,
            unsigned short port,
            const std::string &doc_root,
            int threads,
            Protocol protocol,
            mysql::connection_pool &pool);

        void run();

        void add_route_rest_full(const std::string &method, const std::string &path, Router::ParamHandler handler);
        void add_route(const std::string &method, const std::string &path, Router::NoParamHandler handler);

    private:
        asio::net::tcp::endpoint endpoint_;
        std::string doc_root_;
        int threads_;
        Protocol protocol_;
        asio::io_context ioc_;
        asio::ssl::context ctx_;
        task_group task_group_;
        Router router_;
        std::unordered_map<std::string, SessionData> sessions_;
        std::unordered_map<std::string, std::shared_ptr<asio::steady_timer>> session_timers_;

        fast::mapper::Database database_;

        asio::awaitable<void> detect_session(
            beast::tcp_stream stream, asio::ssl::context &ctx, beast::string_view doc_root, Protocol protocol);

        template<typename Stream>
        static asio::awaitable<void> run_session(
            Stream &stream, beast::flat_buffer &buffer, beast::string_view doc_root, Server &server);

        template<typename Stream>
        static asio::awaitable<void> run_websocket_session(
            Stream &stream,
            beast::flat_buffer &buffer,
            beast::http::request<beast::http::string_body> req,
            beast::string_view doc_root,
            Server &server);

        asio::awaitable<void> listen(
            task_group &task_group,
            asio::ssl::context &ctx,
            asio::net::tcp::endpoint endpoint,
            beast::string_view doc_root,
            Protocol protocol);

        asio::awaitable<void> handle_signals(task_group &task_group, asio::io_context &ioc);

        SessionData &get_or_create_session(const std::string &ip);
        SessionData &create_session(const std::string &ip);
        asio::awaitable<void> remove_session(const std::string &ip);
    };


}

