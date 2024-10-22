#pragma once
#include <restinio/all.hpp>


namespace webserver 
{
  using router_t = restinio::router::express_router_t<>;

  // compile-time constants defining the server
  using serverTraits_t = restinio::traits_t<
    restinio::asio_timer_manager_t,
    restinio::single_threaded_ostream_logger_t,
    router_t
  >;

  std::unique_ptr<router_t> createRequestHandler();
}
