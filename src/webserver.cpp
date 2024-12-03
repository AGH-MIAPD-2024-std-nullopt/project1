#include "webserver.h"
#include "AHP.h"
#include "logging.h"
#include "json_handling.h"

#include <mutex>
#include <filesystem>
#include <fmt/core.h>


void createErrorResponse(auto& req, restinio::http_status_line_t status = restinio::status_internal_server_error()) {
  req->create_response(status)
    .append_header_date_field()
    .connection_close()
    .done();
}

void createOKResponse(auto& req) {
  req->create_response(restinio::status_ok())
    .append_header_date_field()
    .connection_close()
    .done();
}

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

std::string getMIMEType(const restinio::string_view_t& ext)
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
  static struct : public std::mutex { // just to make this object lockable
    std::vector<std::string> alternatives;
    std::vector<std::string> criteria;
    std::vector<json_handling::AgentInput> agentInputs;
  } currentState;

  auto staticContentHandler = [](auto req, auto params) {
    const auto path = params["path"];
    const auto ext = params["ext"];

    const auto filePath = fmt::format("static/{}.{}", path, ext);

    if(filePath.find("..") != std::string::npos) {
      createErrorResponse(req, restinio::status_forbidden());
      return restinio::request_rejected();
    }

    if(!std::filesystem::exists(filePath)) {
      createErrorResponse(req, restinio::status_not_found());
      return restinio::request_rejected();
    }

    try
    {
      //TODO: read file and send it as response
    }
    catch(const std::exception& e)
    {
      createErrorResponse(req);
      return restinio::request_rejected();
    }
  };

  auto submitSetupHandler = [](auto req, auto) {
    try {
      auto query = restinio::parse_query(req->header().query());
      std::string jsonStr(query["data"]);

      auto [alternatives, criteria] = json_handling::parseSetup(jsonStr);

      logger::debug(fmt::format("Recieved valid setup.\n\t criteria: [{}] \n\t alternatives: [{}]", 
                    fmt::join(criteria, ","), fmt::join(alternatives, ",")));

      std::lock_guard lock(currentState);
      currentState.alternatives = alternatives;
      currentState.criteria = criteria;

      createOKResponse(req);
    }
    catch(const std::exception& e) {
      logger::error(fmt::format("Error while parsing setup json: {}\n\tquery: {}", e.what(), req->header().query()));
      createErrorResponse(req);
      return restinio::request_rejected();
    }
    return restinio::request_accepted();
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
      "/submitSetup",
      submitSetupHandler
    );
    
    router->http_get(
      R"(/static/:path(.*)\.:ext(.*))",
      staticContentHandler
    );

    return router;
  }

} // namespace webserver
