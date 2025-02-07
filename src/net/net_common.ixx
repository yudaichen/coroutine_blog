module;

#include <boost/container/flat_map.hpp>
import boost;
import std;
export module net_common;


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

// 辅助函数：解析http分片的 Range 头部
bool parse_range_header(const std::string& range_header, size_t file_size, size_t& start, size_t& end) {
  const std::string prefix = "bytes=";
  if (range_header.compare(0, prefix.size(), prefix) != 0) {
    return false;
  }
  std::string range_str = range_header.substr(prefix.size());
  size_t dash_pos = range_str.find('-');
  if (dash_pos == std::string::npos) {
    return false;
  }

  std::string start_str = range_str.substr(0, dash_pos);
  std::string end_str = range_str.substr(dash_pos + 1);

  if (start_str.empty() && end_str.empty()) {
    return false;
  }

  try {
    if (!start_str.empty() && !end_str.empty()) {
      start = std::stoull(start_str);
      end = std::stoull(end_str);
    } else if (end_str.empty()) {
      start = std::stoull(start_str);
      end = file_size - 1;
    } else {
      size_t suffix_len = std::stoull(end_str);
      if (suffix_len > file_size) {
        suffix_len = file_size;
      }
      start = file_size - suffix_len;
      end = file_size - 1;
    }
  } catch (...) {
    return false;
  }

  if (start >= file_size || end >= file_size || start > end) {
    return false;
  }

  return true;
}

// 生成 416 Range Not Satisfiable 响应
template <class Body, class Allocator>
beast::http::response<beast::http::string_body> range_not_satisfiable(beast::http::request<Body, beast::http::basic_fields<Allocator>>&& req, size_t size) {
  beast::http::response<beast::http::string_body> res{beast::http::status::range_not_satisfiable, req.version()};
  res.set(beast::http::field::server, "bander v1.0");
  res.set(beast::http::field::content_type, "text/html");
  res.content_length(0);
  res.keep_alive(req.keep_alive());
  res.set(beast::http::field::content_range, "bytes */" + std::to_string(size));
  return res;
}

// 生成 206 Partial Content 响应
template <class Body, class Allocator>
beast::http::response<beast::http::file_body> partial_content(beast::http::request<Body, beast::http::basic_fields<Allocator>>&& req,
                                                beast::http::file_body::value_type&& body,
                                                size_t start, size_t end, size_t file_size) {
  beast::http::response<beast::http::file_body> res{beast::http::status::partial_content, req.version()};
  res.set(beast::http::field::server, "bander v1.0");
  res.content_length(end - start + 1);
  res.keep_alive(req.keep_alive());
  res.set(beast::http::field::content_range,
          "bytes " + std::to_string(start) + "-" + std::to_string(end) + "/" + std::to_string(file_size));
  res.body() = std::move(body);
  return res;
}


}





