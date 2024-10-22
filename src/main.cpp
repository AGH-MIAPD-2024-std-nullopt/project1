#include "webserver.h"

#include <restinio/all.hpp>


int main()
{
  restinio::run(
    restinio::on_this_thread<webserver::serverTraits_t>()
      .port( 8080 )
      .address("localhost")
      .request_handler(webserver::createRequestHandler()) 
  );
}