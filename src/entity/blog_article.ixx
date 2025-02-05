module;

#include <boost/json.hpp>
#include <boost/mysql/datetime.hpp>

import std;
export module blog_article_entity;

export namespace fast::entity {
using namespace std::__1;
struct BlogArticle {
  uint64_t article_id;
  std::string title;
  std::string content_preview;
  std::string markdown;
  boost::mysql::datetime create_time;
  boost::mysql::datetime update_time;
};

chrono::system_clock::time_point
mysql_datetime_to_time_point(const boost::mysql::datetime &dt) {
  //using namespace std::chrono;

  auto tp = chrono::system_clock::from_time_t(0);
  tp += chrono::hours(dt.hour()) + chrono::minutes(dt.minute()) + chrono::seconds(dt.second());
  tp += chrono::days(dt.day() - 1) + chrono::months(dt.month() - 1) + chrono::years(dt.year() - 1970);

  return tp;
}

std::string to_iso_string(const chrono::system_clock::time_point &tp) {
  std::time_t tt = chrono::system_clock::to_time_t(tp);
  std::tm tm = *std::localtime(&tt);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

boost::json::object article_to_json(const fast::entity::BlogArticle &article) {
  boost::json::object jobj;
  jobj["article_id"] = article.article_id;
  jobj["title"] = article.title;
  jobj["content_preview"] = article.content_preview;
  jobj["markdown"] = article.markdown;
  jobj["create_time"] =
      to_iso_string(mysql_datetime_to_time_point(article.create_time));
  jobj["update_time"] =
      to_iso_string(mysql_datetime_to_time_point(article.update_time));
  return jobj;
}

boost::json::value
vec_article_to_json(const std::vector<fast::entity::BlogArticle> &article) {
  boost::json::array j_columns;
  for (auto &&blog_article : article) {
    j_columns.push_back(article_to_json(blog_article));
  }
  return j_columns;
}

} // namespace fast::entity
