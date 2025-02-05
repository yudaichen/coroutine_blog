module;

import std;
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


}