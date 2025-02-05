module;

/*#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <boost/mysql/with_params.hpp>
#include <boost/mysql/connection_pool.hpp>*/
#include <boost/system/system_error.hpp>

import asio;
import blog_article_entity;
import blog_column_entity;
import std;
import small_utils;

module blog_mapper;


namespace fast::mapper {


BlogMapper::BlogMapper(mysql::connection_pool &pool) { pool_ = &pool; }

asio::awaitable<void> BlogMapper::get_articles_full_text_awaitable(
    std::string &full_text, int page, int page_size,
    std::vector<fast::entity::BlogArticle> &articles) {
  try {
    mysql::pooled_connection conn =
        co_await pool_->async_get_connection(asio::use_awaitable);

    int offset = (page > 0) ? (page - 1) * page_size : 0;
    mysql::results result;
    mysql::diagnostics diag;

    std::ostringstream query;
    query << "SELECT article_id, title, content FROM "
          << "blog_article WHERE MATCH (title, markdown) AGAINST ('"
          << full_text << "' IN NATURAL LANGUAGE MODE) "
          << "LIMIT " << offset << ", " << page_size;
    std::string sql_str = query.str();

    co_await conn->async_execute(sql_str, result, diag, asio::use_awaitable);

    for (auto &&row : result.rows()) {
      fast::entity::BlogArticle article;
      article.article_id = row.at(0).as_int64();
      article.title = row.at(1).as_string();
      article.content_preview =
          fast::util::remove_html_tags(row.at(2).as_string());
      if (article.content_preview.length() > 30) {
        article.content_preview = fast::util::split_first_n_unicode_chars(
                                      article.content_preview, 40) +
                                  "...";
      }
      articles.emplace_back(article);
    }
  } catch (const std::exception &e) {
    std::cerr << "General Exception in get_articles_full_text_awaitable: "
              << e.what() << std::endl;
  }
  co_return;
}

asio::awaitable<void> BlogMapper::get_articles_by_column_id_awaitable(
    std::uint64_t column_id, int page, int page_size,
    std::vector<fast::entity::BlogArticle> &articles) {
  try {
    mysql::pooled_connection conn =
        co_await pool_->async_get_connection(asio::use_awaitable);

    int offset = (page > 0) ? (page - 1) * page_size : 0;
    mysql::results result;
    mysql::diagnostics diag;

    if (column_id == 0) {
      co_await conn->async_execute(
          mysql::with_params("SELECT article_id, title, content FROM "
                             "blog_article LIMIT {}, {}",
                             offset, page_size),
          result, diag, asio::use_awaitable);
    } else {
      co_await conn->async_execute(
          mysql::with_params("SELECT article_id, title, content FROM "
                             "blog_article WHERE column_id = {} LIMIT {}, {}",
                             column_id, offset, page_size),
          result, diag, asio::use_awaitable);
    }

    for (auto &&row : result.rows()) {
      fast::entity::BlogArticle article;
      article.article_id = row.at(0).as_int64();
      article.title = row.at(1).as_string();
      article.content_preview =
          fast::util::remove_html_tags(row.at(2).as_string());
      if (article.content_preview.length() > 30) {
        article.content_preview =
            util::split_first_n_unicode_chars(article.content_preview, 40) +
            "...";
      }
      articles.emplace_back(article);
    }
  } catch (const std::exception &e) {
    std::cerr << "General Exception in get_articles_by_column_id_awaitable: "
              << e.what() << std::endl;
  }
  co_return;
}

asio::awaitable<void> BlogMapper::get_all_blog_columns_awaitable(
    std::vector<fast::entity::BlogColumn> &columns) {
  try {
    mysql::pooled_connection conn =
        co_await pool_->async_get_connection(asio::use_awaitable);

    mysql::results result;
    mysql::diagnostics diag;
    co_await conn->async_execute("SELECT column_id, column_name, "
                                 "column_description, pid FROM blog_column",
                                 result, diag, asio::use_awaitable);

    if (!result.has_value()) {
      std::cerr << "not info";
    }
    for (auto &&row : result.rows()) {
      try {
        fast::entity::BlogColumn column;
        column.column_id = row.at(0).as_int64();
        column.column_name = row.at(1).as_string();
        column.column_description = row.at(2).as_string();
        column.pid = row.at(3).as_int64();
        columns.push_back(column);
      } catch (const std::invalid_argument &e) {
        std::cerr << "Invalid column_id: " << e.what() << std::endl;
        continue;
      } catch (const std::out_of_range &e) {
        std::cerr << "Out of range column_id: " << e.what() << std::endl;
        continue;
      } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
      }
    }
  } catch (const boost::system::system_error &ec) {
    std::cerr << "BlogMapper Error (system_error): " << ec.code().message()
              << " (" << ec.code().value() << ")" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "General Exception in get_all_blog_columns_awaitable: "
              << e.what() << std::endl;
  }
}

asio::awaitable<void> BlogMapper::get_article_by_article_id_awaitable(
    std::uint64_t article_id, fast::entity::BlogArticle &article) {
  try {
    auto conn = co_await pool_->async_get_connection(asio::use_awaitable);

    mysql::results result;
    mysql::diagnostics diag;
    co_await conn->async_execute(
        mysql::with_params("SELECT title, markdown, create_time, update_time "
                           "FROM blog_article "
                           "WHERE article_id = {}",
                           article_id),
        result, diag, asio::use_awaitable);

    if (!result.has_value()) {
      co_return;
    }
    mysql::row_view row = result.rows().at(0);
    article.title = row.at(0).as_string();
    article.markdown = row.at(1).as_string();
    article.create_time = row.at(2).as_datetime();
    article.update_time = row.at(3).as_datetime();
  } catch (const boost::system::system_error &ec) {
    std::cerr << "BlogMapper Error (system_error): " << ec.code().message()
              << " (" << ec.code().value() << ")" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "General Exception in get_all_blog_columns_awaitable: "
              << e.what() << std::endl;
  }
}

asio::awaitable<void>
BlogMapper::watch_number_added_awaitable(uint64_t article_id) {
  auto conn = co_await pool_->async_get_connection(asio::use_awaitable);
  mysql::results result;
  mysql::diagnostics diag;

  co_await conn->async_execute(
      mysql::with_params("UPDATE blog_article "
                         "SET watch_number = watch_number + 1 "
                         "WHERE article_id = {}",
                         article_id),
      result, diag, asio::use_awaitable);
}

}; // namespace fast::mapper