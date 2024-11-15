#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <Eigen/Dense>

class AHPRanker {
public:
    using Matrix2D = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;

    AHPRanker() = default;
    AHPRanker(const AHPRanker&) = default;
    AHPRanker(AHPRanker&&) = default;
    AHPRanker& operator=(const AHPRanker&) = default;
    AHPRanker& operator=(AHPRanker&&) = default;

    void addComparison(const std::string& alternative1,
                       const std::string& alternative2,
                       const std::string& criteria,
                       double value);

    void readSingleAgentCsv(const std::string& filename);
    void readMultiAgentsCsv(const std::vector<std::string>& filenames);

    void calculate();

    std::vector<double> getRanking();
    std::vector<std::string> getCriteria();
    double getConsistencyRatio();

private:
    Matrix2D buildAvgMatrix(const std::vector<Matrix2D>& matrices);
    
    std::vector<double> calculateRanking();
    double calculateConsistencyRatio();
    double getRandomConsistencyIndex(size_t n);

    using ComparisonMap = std::map<std::pair<std::string, std::string>, double>;
    using ComparisonsByCriteriaMap = std::map<std::string, ComparisonMap>;
    ComparisonsByCriteriaMap comparisons;

    std::optional<std::vector<std::string>> criteria;
    std::optional<Matrix2D> comparison_matrix;
    std::optional<std::vector<double>> calculatedRanking;
    std::optional<double> calculatedConsistencyRatio;
};