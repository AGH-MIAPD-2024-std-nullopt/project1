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

    std::vector<std::string> altArr = json["alternatives"].as_array();
    std::vector<std::string> critArr = json["criteria"].as_array();

    for(auto i : altArr) {
      alternatives.push_back(i);
    }

    for(auto i : critArr) {
      criteria.push_back(i);
    }

    return { alternatives, criteria };
  };

  AHP::ComparisonValues parseSingleAltComparisons(json::jobject& singleAltComparisons)
  {
    AHP::ComparisonValues compValues;

    auto keys = singleAltComparisons.list_keys();
    for(auto& key : keys) {
      std::string val = singleAltComparisons[key].as_string();
      compValues[key] = std::stod(val);
    }
    
    return compValues;
  }

  AgentInput parseAgentInput(const std::string& jsonStr)
  {
    AgentInput agentInput;

    json::jobject json = json::jobject::parse(jsonStr.c_str());

    json::jobject altComparisons = json["alternativeMatrices"];
    json::jobject critComparisons = json["criteriaMatrix"];

    for(std::string criteria : altComparisons.list_keys()) {
      auto altCompByCriteria = altComparisons[criteria].as_object();

      for(std::string alt1 : altCompByCriteria.list_keys()) {
        auto altValues = altCompByCriteria[alt1].as_object();
        agentInput.altComparisons[criteria][alt1] = parseSingleAltComparisons(altValues);
      }
    }

    for(std::string criteria : critComparisons.list_keys()) {
      auto critValues = critComparisons[criteria].as_object();
      agentInput.critComparisons[criteria] = parseSingleAltComparisons(critValues);
    }

    return agentInput;
  };

} // namespace json_handling