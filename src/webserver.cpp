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
    .append_header( restinio::http_field::content_type, "application/json" )
    .append_header_date_field()
    .set_body("{ \"status\": \"Success\" }")
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

    const std::string filePath = fmt::format("src_html/static/{}.{}", path, ext);

    if(filePath.find("..") != std::string::npos) {
      createErrorResponse(req, restinio::status_forbidden());
      return restinio::request_rejected();
    }

    const std::string absPath = std::filesystem::absolute(filePath).string();

    if(!std::filesystem::exists(absPath)) {
      createErrorResponse(req, restinio::status_not_found());
      return restinio::request_rejected();
    }

    try
    {
      req->create_response()
        .append_header( restinio::http_field::content_type, getMIMEType(ext) )
        .append_header_date_field()
        .set_body(loadFile(absPath))
        .done();
      return restinio::request_accepted();
    }
    catch(const std::exception& e)
    {
      createErrorResponse(req);
      return restinio::request_rejected();
    }
    return restinio::request_rejected();
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
      currentState.agentInputs.clear();
    }
    catch(const std::exception& e) {
      logger::error(fmt::format("Error while parsing setup json: {}\n\tquery: {}", e.what(), req->header().query()));
      createErrorResponse(req);
      return restinio::request_rejected();
    }

    createOKResponse(req);
    return restinio::request_accepted();
  };

  auto submitHandler = [](auto req, auto) {
    try {
      auto query = restinio::parse_query(req->header().query());
      std::string jsonStr(query["data"]);

      json_handling::AgentInput agi = json_handling::parseAgentInput(jsonStr);

      logger::debug(fmt::format("Recieved valid agent input."));

      std::lock_guard lock(currentState);
      currentState.agentInputs.push_back(agi);
    }
    catch(const std::exception& e) {
      logger::error(fmt::format("Error while parsing setup json: {}\n\tquery: {}", e.what(), req->header().query()));
      createErrorResponse(req);
      return restinio::request_rejected();
    }

    createOKResponse(req);
    return restinio::request_accepted();
  };

  auto resultsHandler = [](auto req, auto) {
    try {
      std::lock_guard lock(currentState);

      if(currentState.alternatives.empty() || currentState.criteria.empty() || currentState.agentInputs.empty()) {
        req->create_response()
          .append_header( restinio::http_field::content_type, "application/json" )
          .append_header_date_field()
          .set_body("{ \"status\": \"Error\", \"message\": \"No data has been submitted\" }")
          .done();
        return restinio::request_rejected();
      }

      AHP::AHPMeanCalculator meanCalc(currentState.criteria);

      for(auto& agentInput : currentState.agentInputs) {
        AHP::Matrix2D criteriaMatrix = AHP::buildMatrix(agentInput.critComparisons, currentState.criteria);
        meanCalc.addCritMatrix(std::move(criteriaMatrix));

        std::map<std::string, AHP::Matrix2D> altMatrices;
        for(auto& [criteria, comparisons] : agentInput.altComparisons) {
          altMatrices[criteria] = AHP::buildMatrix(comparisons, currentState.alternatives);
        }
        meanCalc.addAltMatrices(std::move(altMatrices));
      }

      auto critMatrix = meanCalc.getMeanCritMatrix();
      auto altMatrices = meanCalc.getMeanAltMatrices();

      AHP::AHPRanker ranker;
      AHP::AHPResult result = ranker.calculateRanking(critMatrix, altMatrices);

      // Prepare response
      std::string resp = loadFile("src_html/templates/results.html");

      // Ranking of alternatives
      std::vector<std::pair<std::string, double>> rankedAlternatives;
      for(size_t i = 0; i < currentState.alternatives.size(); i++) {
        rankedAlternatives.push_back({currentState.alternatives[i], result.ranking[i]});
      }
      std::sort(rankedAlternatives.begin(), rankedAlternatives.end(), 
                [](auto& a, auto& b) { return a.second > b.second; });

      std::string ranking = "";
      int pos = 1;
      for(auto& [alt, val] : rankedAlternatives) {
        ranking += fmt::format("<tr><td class=\"ranking-poscol\">{}.</td><td>{}</td> <td class=\"ranking-valcol\">{:.3}</td></tr>", 
          pos++, alt, val);
      }
      resp.replace(resp.find("{{RANKING}}"), 11, ranking);

      // Inconsistency ratios for alternatives
      std::string ir_alts = "";
      for(size_t i = 0; i < currentState.criteria.size(); i++) {
        ir_alts += fmt::format("<tr><td class=\"ir-alt-data\">{}:</td> <td class=\"data-valcol\">{:.3}</td></tr>", 
          currentState.criteria[i], result.alternativesIRatios[i]);
      }
      resp.replace(resp.find("{{IR_ALTS}}"), 11, ir_alts);

      // Inconsistency ratio for criteria
      std::string ir_crit = std::isnan(result.criteriaIRatio) ? "N/A" : fmt::format("{:.3}", result.criteriaIRatio);
      
      resp.replace(resp.find("{{IR_CRIT}}"), 11, ir_crit);

      req->create_response()
        .append_header( restinio::http_field::content_type, "text/html; charset=utf-8" )
        .append_header_date_field()
        .set_body(resp)
        .done();
    }
    catch(const std::exception& e) {
      logger::error(fmt::format("Error while calculating results: {}", e.what()));
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
        .append_header_date_field()
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
      "/submit",
      submitHandler
    );
    router->http_get(
      "/results",
      resultsHandler
    );
    router->http_get(
      R"(/static/:path(.*)\.:ext(.*))",
      staticContentHandler
    );

    return router;
  }

} // namespace webserver
