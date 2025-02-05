module;
#include "common/boost_head.h"
import std;
export module response_utils;

export std::string create_json_ok_response(boost::json::value to_json,
                                    int code = 200) {
  boost::json::object obj;
  obj["status"] = "ok";
  obj["code"] = code;
  obj["message"] = to_json;
  return boost::json::serialize(obj);
}

// 封装一个函数来创建错误的 JSON 响应
export std::string create_json_error_response(const std::string &message,
                                       int code = 500) {
  boost::json::object obj;
  obj["status"] = "error";
  obj["code"] = code;
  obj["message"] = message;
  return boost::json::serialize(obj);
}