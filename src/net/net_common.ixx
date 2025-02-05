module;

/*#include <boost/beast/core.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http.hpp>*/
#include <boost/container/flat_map.hpp>
import asio;
import std;
export module net_common;

/*namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;*/

export namespace fast::net {

beast::string_view mime_type(beast::string_view path) {
  const boost::container::flat_map<std::string, std::string> mime_types = {
    {".htm", "text/html"},
    {".html", "text/html"},
    {".php", "text/html"}, // Placeholder, actual type may vary
    {".css", "text/css"},
    {".txt", "text/plain"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".swf", "application/x-shockwave-flash"},
    {".flv", "video/x-flv"},
    {".png", "image/png"},
    {".jpe", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"},
    {".bmp", "image/bmp"},
    {".ico", "image/vnd.microsoft.icon"},
    {".tiff", "image/tiff"},
    {".tif", "image/tiff"},
    {".svg", "image/svg+xml"},
    {".svgz", "image/svg+xml"},
};
  auto const ext = path.substr(path.rfind("."));

  // Use flat_map beast::string_view to avoid unnecessary copies
  return mime_types.find(ext) != mime_types.end() ? mime_types.at(ext)
                                                  : "application/text";
}


std::string path_cat(beast::string_view base, beast::string_view path) {
  if (base.empty())
    return std::string(path);
  std::string result(base);
#ifdef BOOST_MSVC
  char constexpr path_separator = '\\';
  if (result.back() == path_separator)
    result.resize(result.size() - 1);
  result.append(path.data(), path.size());
  for (auto &c : result)
    if (c == '/')
      c = path_separator;
#else
  char constexpr path_separator = '/';
  if (result.back() == path_separator)
    result.resize(result.size() - 1);
  result.append(path.data(), path.size());
#endif
  return result;
}


beast::http::response<beast::http::string_body>
not_found(beast::http::request<beast::http::string_body> &&req) {
  beast::http::response<beast::http::string_body> res{beast::http::status::not_found, req.version()};
  res.set(beast::http::field::server, "bander v1.0.0");
  res.set(beast::http::field::content_type, "text/html");
  res.keep_alive(req.keep_alive());
  res.body() =
      "The resource '" + std::string(req.target()) + "' was not found.";
  res.prepare_payload();
  return res;
}


beast::http::response<beast::http::string_body>
server_error(beast::http::request<beast::http::string_body> &&req, beast::string_view what) {
  beast::http::response<beast::http::string_body> res{beast::http::status::internal_server_error,
                                        req.version()};
  res.set(beast::http::field::server, "bander v1.0.0");
  res.set(beast::http::field::content_type, "text/html");
  res.keep_alive(req.keep_alive());
  res.body() = "An error occurred: '" + std::string(what) + "'";
  res.prepare_payload();
  return res;
}


}





