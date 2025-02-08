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

class Session {
public:
  Session(asio::io_context &ioc,fast::mapper::Database &pool)
      : ioc_(ioc), database_pool_(pool) {}

  SessionData &get_or_create_session(const std::string &ip) {
    auto it = sessions_.find(ip);
    if (it == sessions_.end()) {
      it = sessions_.emplace(ip, SessionData(ip, database_pool_))
               .first; // 获取数据库连接
      create_session_timer(ip);
    }
    it->second.update_last_access_time();
    return it->second;
  }

  SessionData &create_session(const std::string &ip) {
    auto it = sessions_.emplace(ip, SessionData(ip, database_pool_))
                  .first; // 获取数据库连接
    create_session_timer(ip);
    return it->second;
  }

  asio::awaitable<void> remove_session(const std::string &ip) {
    auto it = sessions_.find(ip);
    if (it != sessions_.end()) {
      auto now = std::chrono::steady_clock::now();
      if (now - it->second.get_last_access_time() >= std::chrono::minutes(30)) {
        sessions_.erase(it);
        session_timers_.erase(ip);
      } else {
        reset_session_timer(ip);
      }
    }
    co_return;
  }

private:
  void create_session_timer(const std::string &ip) {
    auto timer =
        std::make_shared<asio::steady_timer>(ioc_, std::chrono::seconds(60));
    session_timers_[ip] = timer;

    timer->async_wait([this, ip](const boost::system::error_code &ec) {
      if (!ec) {
        asio::co_spawn(ioc_, remove_session(ip), asio::detached);
      }
    });
  }

  void reset_session_timer(const std::string &ip) {
    auto it = sessions_.find(ip);
    if (it != sessions_.end()) {
      auto now = std::chrono::steady_clock::now();
      auto timer = session_timers_[ip];
      timer->expires_after(std::chrono::minutes(30) -
                           (now - it->second.get_last_access_time()));
      timer->async_wait([this, ip](const boost::system::error_code &ec) {
        if (!ec) {
          asio::co_spawn(ioc_, remove_session(ip), asio::detached);
        }
      });
    }
  }

private:
  asio::io_context &ioc_;
  fast::mapper::Database &database_pool_;
  std::map<std::string, SessionData> sessions_;
  std::map<std::string, std::shared_ptr<asio::steady_timer>> session_timers_;
};

} // namespace fast::net