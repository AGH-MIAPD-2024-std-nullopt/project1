#pragma once

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <Eigen/Dense>

namespace AHP {
    using Matrix2D = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
    using ComparisonValues = std::map<std::string, double>; // alt2 -> value
    using Comparisons = std::map<std::string, ComparisonValues>; // alt1 -> map of comaprison values for each alt2
    using WeightsAndIR = std::pair<std::vector<double>, double>; // weights and inconsistancy ratio for a single matrix

    struct AHPResult {
        std::vector<double> ranking;             // Final ranking of alternatives
        double criteriaIRatio;           // Inconsistency ratio for criteria comparison matrix
        std::vector<double> alternativesIRatios;  // Inconsistency ratios for comparison of alternatives by single criterium 
    };
    
    struct AgentInput {
        std::map<std::string, Comparisons> altComparisons;  // criteria -> comparisons of alternatives in this criteria
        Comparisons critComparisons;
    };

    class AHPMatrixBuilder {
    public:
        AHPMatrixBuilder(const std::vector<std::string>& alternatives, const std::vector<std::string>& criteria);

        void addComparison(const std::string& alt1, const std::string& alt2, double val);

        Matrix2D getMatrix();

    private:
        const std::vector<std::string>& alternatives_;
        const std::vector<std::string>& criteria_;
        Comparisons comps_;
        Matrix2D mat_;
    };

    class AHPRanker {
    public:
        AHPResult calculateRanking(Matrix2D criteria_comparison, std::vector<Matrix2D> alternatives_comparisons);
        
    private:
        WeightsAndIR calculateWeightsAndIR(Matrix2D matrix);
        std::vector<double> calculateRankingVector(Matrix2D matrix);
    };

}