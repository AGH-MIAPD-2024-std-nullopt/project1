#pragma once
#include <vector>
#include <string>
#include <optional>
#include <map>

#include <Eigen/Dense>

// This is only a stub for the actual implementation
// Details are yet to be determined later

class AHPRanker 
{
  using Matrix2D = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
public:
  AHPRanker();
  AHPRanker(AHPRanker&&) = default;
  AHPRanker(const AHPRanker&) = default;

  void addComparison(const std::string& alternative1, 
                      const std::string& alternative2, 
                      const std::string& criteria, 
                      double value);
  void calculate();

  std::vector<double> getRanking();
  double getConsistencyRatio();

protected:
  void buildMatrix();

  using ComparisonMap = std::map<std::pair<std::string, std::string>, double>;    // key: (alternative1, alternative2), value: value of this comparison
  using ComparisonsByCriteriaMap = std::map<std::string, ComparisonMap>;          // key: criteria name, value: ComparisonMap for this criteria
  ComparisonsByCriteriaMap comparisons = {{}};

  std::optional<Matrix2D> matrix                          = std::nullopt;
  std::optional<std::vector<size_t>> calculatedRanking    = std::nullopt;
  std::optional<double> calculatedConsistencyRatio        = std::nullopt;
};