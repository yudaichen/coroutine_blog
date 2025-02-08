module;

import boost;
module router_process;
import std;
import session_data;
import net_common;

namespace fast::net
{

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