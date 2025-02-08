module;

import std;
import mysql_connect;
import small_utils;

module session_data;

namespace fast::net
{

SessionData::SessionData(const std::string &ip, fast::mapper::Database &database) : ip_address(ip), database_(&database)
{
    uuid_ = fast::util::UUID::generate();
    last_access_time = std::chrono::steady_clock::now();
}

fast::mapper::Database *SessionData::get_database() const
{
    return database_;
}
const std::string &SessionData::get_uuid() const
{
    return uuid_;
}
const std::string &SessionData::get_ip_address() const
{
    return ip_address;
}
void SessionData::update_last_access_time()
{
    last_access_time = std::chrono::steady_clock::now();
}
std::chrono::steady_clock::time_point SessionData::get_last_access_time() const
{
    return last_access_time;
}

} // namespace fast::net