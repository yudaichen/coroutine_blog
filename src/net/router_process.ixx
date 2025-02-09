module;

import std;
import boost;
import session_data;
import net_common;

export module router_process;

export namespace fast::net
{

class Router
{
  public:
    using ParamHandler = std::function<asio::awaitable<void>(
        beast::http::request<beast::http::string_body> &, beast::http::response<beast::http::string_body> &,
        SessionData &, std::variant<std::map<std::string, std::string>> &)>;

    using NoParamHandler =
        std::function<asio::awaitable<void>(beast::http::request<beast::http::string_body> &,
                                            beast::http::response<beast::http::string_body> &, SessionData &)>;

    void add_route_rest_full(const std::string &method, const std::string &path, ParamHandler handler);
    void add_route(const std::string &method, const std::string &path, NoParamHandler handler);

    asio::awaitable<void> handle_request(beast::http::request<beast::http::string_body> &req,
                                         beast::http::response<beast::http::string_body> &res, SessionData &session);

    std::unordered_map<std::string, std::string> parse_query_params(beast::string_view query);

    template <typename Stream>
        requires(std::is_same_v<Stream, beast::tcp_stream> ||
                 std::is_same_v<Stream, asio::ssl::stream<beast::tcp_stream>>)
    asio::awaitable<void> handle_static_request(Stream &stream, beast::flat_buffer &buffer, beast::string_view doc_root,
                                                beast::http::request<beast::http::string_body> req);

  private:
    template <typename H> void add_route_internal(const std::string &method, const std::string &path, H handler);

    struct TrieNode
    {
        std::map<std::string, std::unique_ptr<TrieNode>> children;
        std::map<std::string, std::variant<ParamHandler, NoParamHandler>> handlers;
        std::vector<std::string> param_names;
    };

    std::unique_ptr<TrieNode> root_ = std::make_unique<TrieNode>();
};
} // namespace fast::net