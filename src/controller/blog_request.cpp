// beast_net_server.cpp
module;

/*#include <boost/asio/awaitable.hpp>

#include <boost/mysql/connection_pool.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/field.hpp>*/

#include <boost/variant.hpp>
#include <boost/json.hpp>

import asio;
import std;
import mysql_connect;
import beast_net_server;
import small_utils;
import response_utils;
import net_common;
import session_data;
import blog_article_entity;
import blog_column_entity;

module blog_request;


asio::awaitable<void> get_blog_columns(beast::http::request<beast::http::string_body>& req,
                                              beast::http::response<beast::http::string_body>& res,
                                              fast::net::SessionData& session)
{
    fast::mapper::Database* database = session.get_database();
    std::vector<fast::entity::BlogColumn> columns;

    co_await database->blog_mapper->get_all_blog_columns_awaitable(columns);

    std::vector<fast::entity::BlogColumn> out_columns;
    fast::entity::build_column_tree_recursive(columns, out_columns,0);
    boost::json::value to_json = columns_to_json(out_columns);
    res.body() = create_json_ok_response(to_json);
    res.set(beast::http::field::content_type, "application/json; charset=utf-8");
    res.prepare_payload();
    co_return;
}

asio::awaitable<void> get_all_blog(beast::http::request<beast::http::string_body>& req,
                                          beast::http::response<beast::http::string_body>& res,
                                          fast::net::SessionData& session,
                                          boost::variant<std::map<std::string, std::string>>& params)
{
    auto& path_params = boost::get<std::map<std::string, std::string>>(params);
    std::string id = path_params["id"];
    std::string page = path_params["page"];
    std::string page_size = path_params["page_size"];
    fast::mapper::Database* database = session.get_database();
    std::vector<fast::entity::BlogArticle> blogs;

    co_await database->blog_mapper->get_articles_by_column_id_awaitable(std::stoull(id), std::stoi(page), std::stoi(page_size),
                                                           blogs);

    boost::json::value to_json = fast::entity::vec_article_to_json(blogs);
    res.body() = create_json_ok_response(to_json);
    res.set(beast::http::field::content_type, "application/json; charset=utf-8");
    res.prepare_payload();
    co_return;
}

asio::awaitable<void> add_route_rest_full(beast::http::request<beast::http::string_body>& req,
                                                 beast::http::response<beast::http::string_body>& res,
                                                 fast::net::SessionData& session,
                                                 boost::variant<std::map<std::string, std::string>>& params)
{
    auto& path_params = boost::get<std::map<std::string, std::string>>(params);

    std::string basic_string = req.body();
    boost::json::value json_value = boost::json::parse(basic_string);
    if (!json_value.is_object())
    {
        std::cerr << "blog_full_text Error: JSON is not an object." << std::endl;
        co_return;
    }
    boost::json::object json_obj = json_value.as_object();
    std::string blog_full_text = json_obj.at("blog_full_text").as_string().c_str();
    if (blog_full_text.size() == 0)
    {
        std::cerr << "blog_full_text Error: JSON is not an empty string." << std::endl;
        co_return;
    }

    std::string page = path_params["page"];
    std::string page_size = path_params["page_size"];
    fast::mapper::Database* database = session.get_database();

    std::vector<fast::entity::BlogArticle> blogs;
    co_await database->blog_mapper->get_articles_full_text_awaitable(blog_full_text, std::stoi(page), std::stoi(page_size), blogs);

    boost::json::value to_json = fast::entity::vec_article_to_json(blogs);
    res.body() = create_json_ok_response(to_json);
    res.set(beast::http::field::content_type, "application/json; charset=utf-8");
    res.prepare_payload();
    co_return;
}

asio::awaitable<void> get_one_blog(beast::http::request<beast::http::string_body>& req,
                                          beast::http::response<beast::http::string_body>& res,
                                          fast::net::SessionData& session,
                                          boost::variant<std::map<std::string, std::string>>& params)
{
    auto& path_params = boost::get<std::map<std::string, std::string>>(params);
    std::string id = path_params["id"];

    fast::mapper::Database* database = session.get_database();

    fast::entity::BlogArticle article;
    co_await database->blog_mapper->get_article_by_article_id_awaitable(std::stoull(id), article);

    // 观看自增1
    if (article.article_id != 0)
    {
        co_await database->blog_mapper->watch_number_added_awaitable(article.article_id);
    }

    boost::json::value to_json = fast::entity::article_to_json(article);
    res.body() = create_json_ok_response(to_json);
    res.set(beast::http::field::content_type, "application/json; charset=utf-8");
    res.prepare_payload();
    co_return;
}

namespace fast::blog
{
    void set_routines(fast::net::Server& server)
    {

        server.add_route("GET", "/get_blog_columns", get_blog_columns);

        server.add_route_rest_full("GET", "/get_all_blog/{id}/{page}/{page_size}", get_all_blog);

        server.add_route_rest_full("POST", "/blog_full_text/{page}/{page_size}", add_route_rest_full);

        server.add_route_rest_full("GET", "/get_one_blog/{id}", get_one_blog);
    }
};