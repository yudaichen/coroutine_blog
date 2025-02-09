// mysql_connect.ixx
module;

export module mysql_connect;

import std;
import boost;
import blog_mapper;

export namespace fast::mapper
{

struct OptionParams
{
    std::string host = "localhost";
    unsigned short port = 3306;
    std::string username;
    std::string password;
    std::string database;
    mysql::ssl_mode ssl{mysql::ssl_mode::disable};
    bool multi_queries{true};
    std::size_t initial_size{1};
    std::size_t max_size{2};
    std::chrono::seconds connect_timeout{1};
    std::chrono::seconds retry_interval{1};
    std::chrono::hours ping_interval{1};
    std::chrono::seconds ping_timeout{1};
    bool thread_safe{true};
};

mysql::pool_params CreateMysqlOption(const fast::mapper::OptionParams &params)
{
    mysql::pool_params mysql_params;
    mysql_params.server_address.emplace_host_and_port(params.host, params.port);
    mysql_params.username = params.username;
    mysql_params.password = params.password;
    mysql_params.database = params.database;
    mysql_params.ssl = params.ssl;
    mysql_params.multi_queries = params.multi_queries;
    mysql_params.initial_size = params.initial_size;
    mysql_params.max_size = params.max_size;
    mysql_params.connect_timeout = params.connect_timeout;
    mysql_params.retry_interval = params.retry_interval;
    mysql_params.ping_interval = params.ping_interval;
    mysql_params.ping_timeout = params.ping_timeout;
    mysql_params.thread_safe = params.thread_safe;
    return mysql_params;
}

class Database
{
  public:
    Database(mysql::connection_pool &pool)
    {
        pool_ = &pool;
        pool_->async_run(asio::detached);
        InitMapper();
    }

    void InitMapper()
    {
        blog_mapper = new mapper::BlogMapper(*pool_);
    }

    mapper::BlogMapper *blog_mapper;

  private:
    mysql::connection_pool *pool_;
};

} // namespace fast::mapper