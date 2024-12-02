#pragma once

#include <vector>
#include <string>
#include <tuple>

namespace json_handling
{
  struct SingleComparison {
    std::string alt2;
    double value;
  };

  struct SingleAlternativeComparisons {
    std::string alt1;
    std::vector<SingleComparison> comparisons; // alt2 -> value, where value is the comparison of alt1 to alt2
  };

  struct SingleCriteriaComparisons {
    std::string criteria;
    std::vector<SingleAlternativeComparisons> compData;  // comparisons of all alternatives in this criteria
  };

  using AgentInput = std::vector<SingleCriteriaComparisons>;
  using SetupData = std::tuple<std::vector<std::string>, std::vector<std::string>>;

  SetupData parseSetup(const std::string& jsonStr);
  AgentInput parseAgentInput(const std::string& jsonStr);

} // namespace json_handling