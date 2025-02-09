module;

// #include <sio/awaitable.hpp>
// #include <ysql/connection_pool.hpp>
// #include <ariant.hpp>


export module blog_mapper;
import std;
import boost;
import blog_column_entity;
import blog_article_entity;

export namespace fast::mapper
{

class BlogMapper
{
  public:
    BlogMapper(mysql::connection_pool &pool);

    asio::awaitable<void> get_articles_full_text_awaitable(std::string &full_text, int page, int page_size,
                                                           std::vector<fast::entity::BlogArticle> &articles);
    asio::awaitable<void> get_articles_by_column_id_awaitable(std::uint64_t column_id, int page, int page_size,
                                                              std::vector<fast::entity::BlogArticle> &articles);

    asio::awaitable<void> get_all_blog_columns_awaitable(std::vector<fast::entity::BlogColumn> &columns);

    asio::awaitable<void> get_article_by_article_id_awaitable(std::uint64_t article_id,
                                                              fast::entity::BlogArticle &article);

    asio::awaitable<void> watch_number_added_awaitable(std::uint64_t article_id);

  private:
    mysql::connection_pool *pool_;
};
} // namespace fast::mapper