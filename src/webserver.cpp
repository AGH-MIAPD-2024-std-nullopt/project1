#include "webserver.h"

#include <filesystem>
#include <fmt/core.h>


std::string loadFile(const std::string& path)
{
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file: " + path);
  }

  std::ostringstream contents;
  contents << file.rdbuf();
  file.close();

  return contents.str();
}

std::string getMIMEType(const restinio::string_view_t & ext)
{
  if(ext == "css")    return "text/css";
  if(ext == "csv")    return "text/csv";
  if(ext == "html")   return "text/html";
  if(ext == "js")     return "application/javascript";
  if(ext == "json")   return "application/json";
  if(ext == "xhtml")  return "application/xhtml+xml";

  if(ext == "jpeg")   return "image/jpeg";
  if(ext == "jpg")    return "image/jpeg";
  if(ext == "png")    return "image/png";
  if(ext == "svg")    return "image/svg+xml";
  if(ext == "webp")   return "image/webp";

  return "application/text";
}

namespace webserver 
{
  auto staticContentHandler = [](auto req, auto params) {
    const auto path = params["path"];
    const auto ext = params["ext"];

    const auto filePath = fmt::format("static/{}.{}", path, ext);

    if(filePath.find("..") != std::string::npos) {
      return req->create_response(restinio::status_forbidden())
        .append_header_date_field()
        .connection_close()
        .done();
    }

    if(!std::filesystem::exists(filePath)) {
      return req->create_response(restinio::status_not_found())
        .append_header_date_field()
        .connection_close()
        .done();
    }

    try
    {
      //TODO: read file and send it as response
    }
    catch(const std::exception& e)
    {
      return req->create_response(restinio::status_internal_server_error())
        .append_header_date_field()
        .connection_close()
        .done();
    }
  };

  std::unique_ptr<router_t> createRequestHandler()
  {
    auto router = std::make_unique<router_t>();
    router->http_get(
      "/",
      [](auto req, auto) {
        req->create_response()
        .append_header( restinio::http_field::content_type, "text/html; charset=utf-8" )
        .set_body(loadFile("src_html/templates/index.html"))
        .done();

        return restinio::request_accepted();
      }
    );
    
    router->http_get(
      R"(/static/:path(.*)\.:ext(.*))",
      staticContentHandler
    );

    return router;
  }

} // namespace webserver
