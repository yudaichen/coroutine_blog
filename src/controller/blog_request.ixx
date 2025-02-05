module;


#include <boost/asio/awaitable.hpp>

#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>

#include <boost/variant.hpp>


import std;
import mysql_connect;
import BS.thread_pool;
import beast_net_server;
import small_utils;
import session_data;

export module blog_request;


boost::asio::awaitable<void> get_blog_columns(boost::beast::http::request<boost::beast::http::string_body> &req,
         boost::beast::http::response<boost::beast::http::string_body> &res,
         fast::net::SessionData &session);

boost::asio::awaitable<void> get_all_blog(boost::beast::http::request<boost::beast::http::string_body> &req,
         boost::beast::http::response<boost::beast::http::string_body> &res,
         fast::net::SessionData &session, boost::variant<std::map<std::string, std::string>> &params);

boost::asio::awaitable<void> add_route_rest_full(boost::beast::http::request<boost::beast::http::string_body> &req,
         boost::beast::http::response<boost::beast::http::string_body> &res,
         fast::net::SessionData &session, boost::variant<std::map<std::string, std::string>> &params);

boost::asio::awaitable<void> get_one_blog(boost::beast::http::request<boost::beast::http::string_body> &req,
         boost::beast::http::response<boost::beast::http::string_body> &res,
         fast::net::SessionData &session, boost::variant<std::map<std::string, std::string>> &params);


export namespace fast::blog
{
    void set_routines(fast::net::Server &server);
}


