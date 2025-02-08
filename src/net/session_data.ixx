module;

#include <boost/system/error_code.hpp>

import std;
import boost;
import mysql_connect;
export module session_data;

export namespace fast::net {

class SessionData {
public:
  SessionData(const std::string &ip, fast::mapper::Database &database);

  fast::mapper::Database *get_database() const;
  const std::string &get_uuid() const;
  const std::string &get_ip_address() const;
  void update_last_access_time();
  std::chrono::steady_clock::time_point get_last_access_time() const;

private:
  std::string uuid_;
  std::string ip_address;
  std::chrono::steady_clock::time_point last_access_time;
  fast::mapper::Database *database_;
};

class SessionManager {
public:
    SessionManager(asio::io_context& ioc, fast::mapper::Database& pool)
        : ioc_(ioc), database_pool_(pool), strand_(ioc.get_executor()) {}

    // 获取或创建会话
    asio::awaitable<SessionData*> get_or_create_session(const std::string& ip) {
        co_await asio::post(strand_, asio::use_awaitable);
        auto& session = sessions_[ip]; // 自动创建或获取现有会话
        if (!session) {
            session = std::make_unique<SessionData>(ip, database_pool_);
            ++active_sessions_;
            ++total_sessions_created_;
        }
        session->update_last_access_time();
        co_return session.get(); // 返回裸指针
    }

    // 移除会话
    asio::awaitable<void> remove_session(const std::string& ip) {
        co_await asio::post(strand_, asio::use_awaitable);
        auto it = sessions_.find(ip);
        if (it != sessions_.end()) {
            sessions_.erase(it);
            --active_sessions_;
            ++total_sessions_expired_;
        }
        co_return;
    }

    // 获取当前活跃会话数
    std::size_t get_active_sessions() const noexcept {
        return active_sessions_.load();
    }

    // 获取总创建的会话数
    std::size_t get_total_sessions_created() const noexcept {
        return total_sessions_created_.load();
    }

    // 获取总超时的会话数
    std::size_t get_total_sessions_expired() const noexcept {
        return total_sessions_expired_.load();
    }

    // 获取会话的平均生命周期（秒）
    std::chrono::seconds get_average_session_lifetime() const noexcept {
        auto total_lifetime = total_session_lifetime_.load();
        auto total_expired = total_sessions_expired_.load();
        if (total_expired == 0) return std::chrono::seconds(0);
        return std::chrono::duration_cast<std::chrono::seconds>(total_lifetime / total_expired);
    }

private:
    asio::io_context& ioc_;
    fast::mapper::Database& database_pool_;
    asio::strand<asio::io_context::executor_type> strand_;
    std::unordered_map<std::string, std::unique_ptr<SessionData>> sessions_;

    // 统计指标
    std::atomic<std::size_t> active_sessions_{0};          // 当前活跃会话数
    std::atomic<std::size_t> total_sessions_created_{0};   // 总创建的会话数
    std::atomic<std::size_t> total_sessions_expired_{0};   // 总超时的会话数
    std::atomic<std::chrono::steady_clock::duration> total_session_lifetime_; // 总会话生命周期
};

} // namespace fast::net