module;

#include <boost/variant.hpp>

import std;
import boost;
import session_data;
import net_common;

export module router_process;


export namespace fast::net {
class Router {
public:
  using ParamHandler = std::function<asio::awaitable<void>(
       beast::http::request<beast::http::string_body> &,
      beast::http::response<beast::http::string_body> &, SessionData &,
      boost::variant<std::map<std::string, std::string>> &)>;

  using NoParamHandler = std::function<asio::awaitable<void>(
      beast::http::request<beast::http::string_body> &,
      beast::http::response<beast::http::string_body> &, SessionData &)>;

  void add_route_rest_full(const std::string &method, const std::string &path,
                           ParamHandler handler);
  void add_route(const std::string &method, const std::string &path,
                 NoParamHandler handler);

  asio::awaitable<void>
  handle_request(beast::http::request<beast::http::string_body> &req,
                 beast::http::response<beast::http::string_body> &res,
                 SessionData &session);

  std::unordered_map<std::string, std::string>
  parse_query_params(beast::string_view query);

  template <typename Stream>
  asio::awaitable<void>
  handle_static_request(Stream &stream, beast::flat_buffer &buffer,
                        beast::string_view doc_root,
                        beast::http::request<beast::http::string_body> req) {
    static_assert(
        std::is_same_v<Stream, beast::tcp_stream> ||
            std::is_same_v<Stream, asio::ssl::stream<beast::tcp_stream>>,
        "Unsupported stream type");

    auto target = req.target();
    auto query_pos = target.find('?');
    beast::string_view path = target.substr(0, query_pos);
    beast::string_view query = query_pos == beast::string_view::npos
                                   ? ""
                                   : target.substr(query_pos + 1);

    std::string full_path = path_cat(doc_root, path);
    if (path.back() == '/')
      full_path.append("index.html");

    beast::error_code ec;
    beast::http::file_body::value_type body;
    body.open(full_path.c_str(), beast::file_mode::scan, ec);

    if (ec == std::errc::no_such_file_or_directory) {
      auto res = not_found(std::move(req));
      co_await beast::http::async_write(stream, res);
      co_return;
    }

    if (ec) {
      auto res = server_error(std::move(req), ec.message());
      co_await beast::http::async_write(stream, res);
      co_return;
    }

    auto const size = body.size();

    std::unordered_map<std::string, std::string> query_params =
        parse_query_params(query);

    if (req.method() == beast::http::verb::head) {
      beast::http::response<beast::http::empty_body> res{
          beast::http::status::ok, req.version()};
      res.set(beast::http::field::server, "bander v1.0");
      res.set(beast::http::field::content_type, mime_type(full_path));
      res.content_length(size);
      res.keep_alive(req.keep_alive());

      for (const auto &param : query_params) {
        res.set(param.first, param.second);
      }

      co_await beast::http::async_write(stream, res);
      co_return;
    }

    beast::http::response<beast::http::file_body> res{
        std::piecewise_construct, std::make_tuple(std::move(body)),
        std::make_tuple(beast::http::status::ok, req.version())};
    res.set(beast::http::field::server, "bander v1.0");
    res.set(beast::http::field::content_type, mime_type(full_path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());

    for (const auto &param : query_params) {
      res.set(param.first, param.second);
    }

    co_await beast::http::async_write(stream, res);
  }

private:
  template <typename H>
  void add_route_internal(const std::string &method, const std::string &path,
                          H handler);

  struct TrieNode {
    std::map<std::string, std::unique_ptr<TrieNode>> children;
    std::map<std::string, boost::variant<ParamHandler, NoParamHandler>>
        handlers;
    std::vector<std::string> param_names;
  };

  std::unique_ptr<TrieNode> root_ = std::make_unique<TrieNode>();
};
} // namespace fast::net