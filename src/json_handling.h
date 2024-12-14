#pragma once

#include "AHP.h"

#include <vector>
#include <string>
#include <tuple>

namespace json_handling
{
  using AHP::ComparisonValues;
  using AHP::Comparisons;
  using AHP::AgentInput;
  
  using SetupData = std::tuple<std::vector<std::string>, std::vector<std::string>>;

  SetupData parseSetup(const std::string& jsonStr);
  AgentInput parseAgentInput(const std::string& jsonStr);

} // namespace json_handling