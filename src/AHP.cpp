#include "AHP.h"

#include <fmt/core.h>


void AHPRanker::addComparison(const std::string& alternative1, 
                              const std::string& alternative2, 
                              const std::string& criteria, 
                              double value) 
{
  auto& thisCriteriaComparisons = comparisons[criteria];

  if(thisCriteriaComparisons.contains({alternative1, alternative2})) {
    std::string errMsg = fmt::format("Comparison between {} and {} for criteria {} already exists", 
      alternative1, 
      alternative2, 
      criteria);
    throw std::runtime_error(errMsg);
  }

  thisCriteriaComparisons[{alternative1, alternative2}] = value;
}