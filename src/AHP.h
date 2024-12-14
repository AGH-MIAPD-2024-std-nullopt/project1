#pragma once

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <Eigen/Dense>

namespace AHP {
  using ComparisonValues = std::map<std::string, double>;       // alt2 -> value
  using Comparisons = std::map<std::string, ComparisonValues>;  // alt1 -> map of comaprison values for each alt2
  using Matrix2D = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
  using WeightsAndIR = std::pair<std::vector<double>, double>;  // weights and inconsistancy ratio for a single matrix

  struct AHPResult {
    std::vector<double> ranking;              // Final ranking of alternatives
    double criteriaIRatio;                    // Inconsistency ratio for criteria comparison matrix
    std::vector<double> alternativesIRatios;  // Inconsistency ratios for comparison of alternatives by single criterium 
  };

  struct AgentInput {
    std::map<std::string, Comparisons> altComparisons;  // criteria -> comparisons of alternatives in this criteria
    Comparisons critComparisons;
  };


  Matrix2D buildMatrix(const Comparisons& comparisons, const std::vector<std::string>& alternatives);

  class AHPMeanCalculator {
  public:
    AHPMeanCalculator(const std::vector<std::string>& criteria);

    void addAltMatrices(std::map<std::string, Matrix2D>&& matrices);
    void addCritMatrix(Matrix2D&& matrix);

    std::vector<std::string> getCriteria();   // returns criteria names

    Matrix2D getMeanCritMatrix();                // returns geometric mean matrix for criteria comparison
    std::vector<Matrix2D> getMeanAltMatrices();  // returns geometric mean matrices for each criteria

  private:
    std::vector<std::vector<Matrix2D>> altMatrices_;    // altMatrices_[agentIdx][criteriaIdx]
    std::vector<Matrix2D> critMatrices_;                // vector of criteria comparison matrices
    std::vector<std::string> criteria_;                 // criteria names
  };

  class AHPRanker {
  public:
    AHPResult calculateRanking(Matrix2D criteria_comparison, std::vector<Matrix2D> alternatives_comparisons);
      
  private:
    WeightsAndIR calculateWeightsAndIR(Matrix2D matrix);
    std::vector<double> calculateRankingVector(Matrix2D matrix);
  };
}