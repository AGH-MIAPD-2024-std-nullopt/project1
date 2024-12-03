#include "json_handling.h"
#include "logging.h"

#include <json.h>

namespace json_handling
{
  SetupData parseSetup(const std::string& jsonStr)
  {
    std::vector<std::string> alternatives;
    std::vector<std::string> criteria;

    json::jobject json = json::jobject::parse(jsonStr.c_str());

    json::jobject altArr = json["alternatives"];
    json::jobject critArr = json["criteria"];

    for(int i = 0; i < altArr.size(); i++) {
      alternatives.push_back(altArr.array(i).as_string());
    }

    for(int i = 0; i < critArr.size(); i++) {
      criteria.push_back(critArr.array(i).as_string());
    }

    return { alternatives, criteria };
  };

  AHP::ComparisonValues parseSingleAltComparisons(json::jobject& singleAltComparisons)
  {
    //TODO
  }

  AgentInput parseAgentInput(const std::string& jsonStr)
  {
    AgentInput agentInput;

    json::jobject json = json::jobject::parse(jsonStr.c_str());

    json::jobject altComparisons;
    json::jobject critComparisons;
    
    //TODO

    return agentInput;
  };

} // namespace json_handling