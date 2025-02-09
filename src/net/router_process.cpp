module;

import boost;
module router_process;
import std;
import session_data;
import net_common;

namespace fast::net
{

std::unordered_map<std::string, std::string> Router::parse_query_params(beast::string_view query)
{
    std::unordered_map<std::string, std::string> params;
    std::string key, value;
    bool is_key = true;
    for (char c : query)
    {
        if (c == '=')
        {
            is_key = false;
        }
        else if (c == '&')
        {
            params[key] = value;
            key.clear();
            value.clear();
            is_key = true;
        }
        else
        {
            if (is_key)
            {
                key.push_back(c);
            }
            else
            {
                value.push_back(c);
            }
        }
    }
    if (!key.empty())
    {
        params[key] = value;
    }
    return params;
}

// 显式实例化声明
template asio::awaitable<void> Router::handle_static_request<beast::tcp_stream>(
    beast::tcp_stream &stream, beast::flat_buffer &buffer, beast::string_view doc_root,
    beast::http::request<beast::http::string_body> req);

template asio::awaitable<void> Router::handle_static_request<asio::ssl::stream<beast::tcp_stream>>(
    asio::ssl::stream<beast::tcp_stream> &stream, beast::flat_buffer &buffer, beast::string_view doc_root,
    beast::http::request<beast::http::string_body> req);

template <typename Stream>
    requires(std::is_same_v<Stream, beast::tcp_stream> || std::is_same_v<Stream, asio::ssl::stream<beast::tcp_stream>>)
asio::awaitable<void> Router::handle_static_request(Stream &stream, beast::flat_buffer &buffer,
                                                    beast::string_view doc_root,
                                                    beast::http::request<beast::http::string_body> req)
{
    auto target = req.target();
    auto query_pos = target.find('?');
    beast::string_view path = target.substr(0, query_pos);
    beast::string_view query = query_pos == beast::string_view::npos ? "" : target.substr(query_pos + 1);

    std::string full_path = path_cat(doc_root, path);
    if (path.back() == '/')
        full_path.append("index.html");

    beast::error_code ec;
    beast::http::file_body::value_type body;
    body.open(full_path.c_str(), beast::file_mode::scan, ec);

    if (ec == std::errc::no_such_file_or_directory)
    {
        auto res = not_found(std::move(req));
        co_await beast::http::async_write(stream, res);
        co_return;
    }

    if (ec)
    {
        auto res = server_error(std::move(req), ec.message());
        co_await beast::http::async_write(stream, res);
        co_return;
    }

    auto const size = body.size();

    std::unordered_map<std::string, std::string> query_params = parse_query_params(query);

    if (req.method() == beast::http::verb::head)
    {
        beast::http::response<beast::http::empty_body> res{beast::http::status::ok, req.version()};
        res.set(beast::http::field::server, "bander v1.0");
        res.set(beast::http::field::content_type, mime_type(full_path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());

        for (const auto &param : query_params)
        {
            res.set(param.first, param.second);
        }

        co_await beast::http::async_write(stream, res);
        co_return;
    }

    // 处理 Range 请求
    auto range_header = req.find(beast::http::field::range);
    if (range_header != req.end())
    {
        std::size_t start, end;
        if (parse_range_header(range_header->value(), size, start, end))
        {
            // 调整文件指针
            body.seek(start, ec);
            if (ec)
            {
                auto res_error = server_error(std::move(req), ec.message());
                co_await beast::http::async_write(stream, res_error);
                co_return;
            }

            // 返回 206 Partial Content
            auto res = partial_content(std::move(req), std::move(body), start, end, size);
            co_await beast::http::async_write(stream, res);
            co_return;
        }
        else
        {
            // 返回 416 Range Not Satisfiable
            auto res_error = range_not_satisfiable(std::move(req), size);
            co_await beast::http::async_write(stream, res_error);
            co_return;
        }
    }

    // 普通 GET 请求
    beast::http::response<beast::http::file_body> res{std::piecewise_construct, std::make_tuple(std::move(body)),
                                                      std::make_tuple(beast::http::status::ok, req.version())};
    res.set(beast::http::field::server, "bander v1.0");
    res.set(beast::http::field::content_type, mime_type(full_path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());

    for (const auto &param : query_params)
    {
        res.set(param.first, param.second);
    }

    co_await beast::http::async_write(stream, res);
}

asio::awaitable<void> Router::handle_request(beast::http::request<beast::http::string_body> &req,
                                             beast::http::response<beast::http::string_body> &res, SessionData &session)
{
    std::vector<std::string> path_segments;
    std::string segment;
    std::string target = std::string(req.target());
    if (!target.empty() && target[0] == '/')
        target = target.substr(1);

    for (char c : target)
    {
        if (c == '/')
        {
            if (!segment.empty())
            {
                path_segments.push_back(segment);
                segment.clear();
            }
        }
        else
        {
            segment += c;
        }
    }
    if (!segment.empty())
        path_segments.push_back(segment);

    const TrieNode *current = root_.get();
    std::map<std::string, std::string> path_params;

    for (const auto &seg : path_segments)
    {
        bool found = false;
        for (const auto &child : current->children)
        {
            if (child.first == seg)
            {
                current = child.second.get();
                found = true;
                break;
            }
            else if (!child.first.empty() && child.first[0] == '{' && child.first.back() == '}')
            {
                current = child.second.get();
                std::string param_name = child.first.substr(1, child.first.size() - 2);
                path_params[param_name] = seg;
                found = true;
                break;
            }
        }
        if (!found)
        {
            res = not_found(std::move(req));
            co_return;
        }
    }

    auto method = req.method_string();
    if (current && current->handlers.find(method) != current->handlers.end())
    {
        try
        {
            auto &handler = current->handlers.at(method);
            if (std::holds_alternative<ParamHandler>(handler))
            {
                std::variant<std::map<std::string, std::string>> params(path_params);
                co_await std::get<ParamHandler>(handler)(req, res, session, params);
            }
            else if (std::holds_alternative<NoParamHandler>(handler))
            {
                co_await std::get<NoParamHandler>(handler)(req, res, session);
            }
        }
        catch (std::bad_variant_access &e)
        {
            std::cerr << "bad get: " << e.what() << std::endl;
            res = server_error(std::move(req), "bad get");
        }
        co_return;
    }

    res = not_found(std::move(req));
    co_return;
}

template <typename H> void Router::add_route_internal(const std::string &method, const std::string &path, H handler)
{
    std::vector<std::string> path_segments;
    std::vector<std::string> param_names;
    std::string segment;
    for (char c : path)
    {
        if (c == '/')
        {
            if (!segment.empty())
            {
                path_segments.push_back(segment);
                if (segment.size() > 2 && segment[0] == '{' && segment.back() == '}')
                    param_names.push_back(segment.substr(1, segment.size() - 2));
                segment.clear();
            }
        }
        else
        {
            segment += c;
        }
    }
    if (!segment.empty())
    {
        path_segments.push_back(segment);
        if (segment.size() > 2 && segment[0] == '{' && segment.back() == '}')
            param_names.push_back(segment.substr(1, segment.size() - 2));
    }

    TrieNode *current = root_.get();
    for (const auto &seg : path_segments)
    {
        if (current->children.find(seg) == current->children.end())
        {
            current->children[seg] = std::make_unique<TrieNode>();
        }
        current = current->children[seg].get();
    }
    current->handlers[method] = std::move(handler);
    current->param_names = std::move(param_names);
}

void Router::add_route_rest_full(const std::string &method, const std::string &path, ParamHandler handler)
{
    add_route_internal(method, path, std::move(handler));
}

void Router::add_route(const std::string &method, const std::string &path, NoParamHandler handler)
{
    add_route_internal(method, path, std::move(handler));
}
} // namespace fast::net