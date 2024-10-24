#include "webserver.h"

#include <restinio/all.hpp>


int main()
{
  restinio::run(
    restinio::on_this_thread<webserver::serverTraits_t>()
      .port( 8080 )
      .address("0.0.0.0")
      .request_handler(webserver::createRequestHandler()) 
  );
}