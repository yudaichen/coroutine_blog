module;

import boost;
import std;
import mysql_connect;
import BS.thread_pool;
import beast_net_server;
import small_utils;
import session_data;

export module blog_request;

asio::awaitable<void> get_blog_columns(beast::http::request<beast::http::string_body> &req,
                                       beast::http::response<beast::http::string_body> &res,
                                       fast::net::SessionData &session);

asio::awaitable<void> get_all_blog(beast::http::request<beast::http::string_body> &req,
                                   beast::http::response<beast::http::string_body> &res,
                                   fast::net::SessionData &session,
                                   std::variant<std::map<std::string, std::string>> &params);

asio::awaitable<void> add_route_rest_full(beast::http::request<beast::http::string_body> &req,
                                          beast::http::response<beast::http::string_body> &res,
                                          fast::net::SessionData &session,
                                          std::variant<std::map<std::string, std::string>> &params);

asio::awaitable<void> get_one_blog(beast::http::request<beast::http::string_body> &req,
                                   beast::http::response<beast::http::string_body> &res,
                                   fast::net::SessionData &session,
                                   std::variant<std::map<std::string, std::string>> &params);

export namespace fast::blog
{
void set_routines(fast::net::Server &server);
}
