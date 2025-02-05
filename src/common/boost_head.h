//
// Created by ydc on 25-1-14.
//

#ifndef BOOST_HEAD_H
#define BOOST_HEAD_H
#if LINUX
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#endif

// Boost.Container
#include <boost/container/flat_map.hpp>

// Boost.Variant
#include <boost/variant.hpp>

// Boost.Asio
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/as_tuple.hpp>

// Boost.Endian
#include <boost/endian/conversion.hpp>

// Boost.System
#include <boost/system/error_code.hpp>

// Boost.Beast
#include <boost/beast/core.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

// Boost.MySQL
#include <boost/mysql/any_address.hpp>
#include <boost/mysql/any_connection.hpp>
#include <boost/mysql/connect_params.hpp>
#include <boost/mysql/connection_pool.hpp>
#include <boost/mysql/error_with_diagnostics.hpp>
#include <boost/mysql/pool_params.hpp>
#include <boost/mysql/results.hpp>
#include <boost/mysql/row_view.hpp>
#include <boost/mysql/statement.hpp>
#include <boost/mysql/static_results.hpp>
#include <boost/mysql/with_params.hpp>

// Boost.JSON
#include <boost/json.hpp>
#include <boost/json/serialize.hpp>

// Boost.Cobalt
#include <boost/cobalt/task.hpp>



#endif //BOOST_HEAD_H
