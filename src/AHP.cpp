#include "AHP.h"
#include <numeric>
#include <cmath>

/*    AHP HELPERS     */

AHP::Matrix2D AHP::buildMatrix(const AHP::Comparisons& comparisons, const std::vector<std::string>& alternatives) {
  const size_t n = alternatives.size();
  AHP::Matrix2D matrix = AHP::Matrix2D::Ones(n, n);

  std::map<std::string, int> alt_idx;
  int i = 0;
  for(auto& alt : alternatives) {
    alt_idx[alt] = i++;
  }

  for(auto [alt1, values] : comparisons) {
    for(auto [alt2, value] : values) {
      if (alt1 == alt2) 
        continue;
      matrix(alt_idx[alt1], alt_idx[alt2]) = value;
      matrix(alt_idx[alt2], alt_idx[alt1]) = 1.0 / value;
    }
  }

  return matrix;
}

AHP::AHPMeanCalculator::AHPMeanCalculator(const std::vector<std::string>& criteria) : criteria_(criteria) {};

void AHP::AHPMeanCalculator::addAltMatrices(std::map<std::string, AHP::Matrix2D>&& matrices) {
  std::vector<AHP::Matrix2D> alt_matrices;
  for (auto& criterion : criteria_) {
    alt_matrices.emplace_back(matrices.at(criterion));
  }
  altMatrices_.emplace_back(alt_matrices);
}

void AHP::AHPMeanCalculator::addCritMatrix(AHP::Matrix2D&& matrix) {
  critMatrices_.emplace_back(matrix);
}

std::vector<std::string> AHP::AHPMeanCalculator::getCriteria() {
  return criteria_;
}

AHP::Matrix2D AHP::AHPMeanCalculator::getMeanCritMatrix() {
  const size_t agentCount = critMatrices_.size();
  const size_t critCount = critMatrices_[0].rows();

  AHP::Matrix2D mean_matrix = AHP::Matrix2D::Zero(critCount, critCount);

  for (size_t row = 0; row < critCount; row++) {
    for (size_t col = 0; col < critCount; col++) {
      double product = 1.0;
      for (size_t agentIdx = 0; agentIdx < agentCount; agentIdx++) {
        product *= critMatrices_[agentIdx](row, col);
      }
      mean_matrix(row, col) = std::pow(product, 1.0/agentCount);
    }
  }

  return mean_matrix;
}

std::vector<AHP::Matrix2D> AHP::AHPMeanCalculator::getMeanAltMatrices() {
  const size_t agentCount = altMatrices_.size();
  const size_t critCount = altMatrices_[0].size();
  const size_t altCount = altMatrices_[0][0].rows();
  std::vector<AHP::Matrix2D> mean_matrices;

  for (size_t critIdx = 0; critIdx < critCount; critIdx++) {
    AHP::Matrix2D mean_matrix = AHP::Matrix2D::Zero(altCount, altCount);

    for (size_t row = 0; row < altCount; row++) {
      for (size_t col = 0; col < altCount; col++) {
        double product = 1.0;
        for (size_t agentIdx = 0; agentIdx < agentCount; agentIdx++) {
          product *= altMatrices_[agentIdx][critIdx](row, col);
        }
        mean_matrix(row, col) = std::pow(product, 1.0/agentCount);
      }
    }

    mean_matrices.push_back(mean_matrix);
  }

  return mean_matrices;
}

/*    AHP RANKER    */

AHP::WeightsAndIR AHP::AHPRanker::calculateWeightsAndIR(AHP::Matrix2D matrix)
{
  const size_t n = matrix.rows();
  Eigen::VectorXd weights = Eigen::VectorXd::Zero(n);

  for (size_t i = 0; i < n; i++) {
    double product = 1.0;
    for (size_t j = 0; j < n; j++) {
      product *= matrix(i, j);
    }
    weights(i) = std::pow(product, 1.0/n);
  }

  weights /= weights.sum();

  double lambda_max = 0.0;
  for (size_t i = 0; i < n; i++) {
    double sum_row = 0.0;
    for (size_t j = 0; j < n; j++) {
      sum_row += matrix(i, j) * weights(j);
    }
    lambda_max += sum_row / weights(i);
  }
  lambda_max /= n;

  double CI = (lambda_max - n) / (n - 1);

  const std::vector<double> RI = {0.0, 0.0, 0.58, 0.9, 1.12, 1.24, 1.32, 1.41, 1.45, 1.49};

  double CR = 0.0;
  if (n > 1 && n <= 10) {
    CR = CI / RI[n-1];
  }

  std::vector<double> weights_std(weights.data(), weights.data() + weights.size());

  return {weights_std, CR};
}

std::vector<double> AHP::AHPRanker::calculateRankingVector(AHP::Matrix2D matrix) {
  const int alternatives = matrix.rows();
  const int criteria = matrix.cols();

  Eigen::VectorXd criteriaWeights = matrix.row(0);

  std::vector<double> ranking(alternatives - 1, 0.0);

  for(int i = 1; i < alternatives; i++) {
    double sum = 0.0;
    for(int j = 0; j < criteria; j++) {
      sum += matrix(i, j) * criteriaWeights(j);
    }
    ranking[i-1] = sum;
  }

  return ranking;
}

AHP::AHPResult AHP::AHPRanker::calculateRanking(AHP::Matrix2D criteria_comparison, std::vector<Matrix2D> alternatives_comparisons) {
  auto [criteria_weights, criteria_ir] = calculateWeightsAndIR(criteria_comparison);

  const size_t n_criteria = criteria_weights.size();
  const size_t n_alternatives = alternatives_comparisons[0].rows();
  Matrix2D ranking_matrix(n_alternatives + 1, n_criteria);

  for (size_t i = 0; i < n_criteria; i++) {
    ranking_matrix(0, i) = criteria_weights[i];
  }

  std::vector<double> inconsistency_ratios;
  for (size_t i = 0; i < n_criteria; i++) {
    auto [weights, ir] = calculateWeightsAndIR(alternatives_comparisons[i]);
    inconsistency_ratios.push_back(ir);

    for (size_t j = 0; j < n_alternatives; j++) {
      ranking_matrix(j + 1, i) = weights[j];
    }
  }

  std::vector<double> final_ranking = calculateRankingVector(ranking_matrix);

  AHPResult result;
  result.ranking = final_ranking;
  result.criteriaIRatio = criteria_ir;
  result.alternativesIRatios = inconsistency_ratios;

  return result;
}