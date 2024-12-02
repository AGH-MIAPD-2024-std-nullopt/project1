#include "AHP.h"

#include <fmt/core.h>
#include <fstream>
#include <numeric>

void AHPRanker::addComparison(const std::string& alternative1,
                              const std::string& alternative2,
                              const std::string& criteria,
                              double value) {
    auto& thisCriteriaComparisons = comparisons[criteria];

    if (thisCriteriaComparisons.contains(std::make_pair(alternative1, alternative2))) {
        throw std::runtime_error(fmt::format("Comparison between {} and {} for criteria {} already exists",
                                             alternative1, alternative2, criteria));
    }

    thisCriteriaComparisons[std::make_pair(alternative1, alternative2)] = value;
}

void AHPRanker::readSingleAgentCsv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::vector<std::string> criteria_names;
    std::string line;

    if (std::getline(file, line)) {
        std::stringstream lineStream(line);
        std::string cell;
        while (std::getline(lineStream, cell, ',')) {
            criteria_names.emplace_back(std::move(cell));
        }
    }

    criteria_names[criteria_names.size() - 1].pop_back(); 
    
    size_t n = criteria_names.size();

    Matrix2D tmp_matrix(n, n);
    size_t i = 0;
    while (std::getline(file, line)) {
        std::stringstream lineStream(line);
        std::string cell;
        size_t j = 0;
        while (std::getline(lineStream, cell, ',')) {
            tmp_matrix(i, j) = std::stod(cell);
            ++j;
        }
        ++i;
    }

    file.close();

    this->criteria = std::move(criteria_names);
    this->comparison_matrix = std::move(tmp_matrix);
}

void AHPRanker::readMultiAgentsCsv(const std::vector<std::string>& filenames) {
    std::vector<Matrix2D> matrices;

    for (const auto& filename : filenames) {
        readSingleAgentCsv(filename);
        matrices.emplace_back(comparison_matrix.value());
    }

    comparison_matrix = buildAvgMatrix(matrices);
}

void AHPRanker::calculate() {
    calculatedRanking = calculateRanking();
    calculatedConsistencyRatio = calculateConsistencyRatio();
}

std::vector<double> AHPRanker::getRanking() {
    if (!calculatedRanking.has_value()) {
        throw std::runtime_error("Ranking is not calculated yet");
    }
    return calculatedRanking.value();
}

double AHPRanker::getConsistencyRatio() {
    if (!calculatedConsistencyRatio.has_value()) {
        throw std::runtime_error("Consistency ratio is not calculated yet");
    }
    return calculatedConsistencyRatio.value();
}

std::vector<std::string> AHPRanker::getCriteria() {
    if (!criteria.has_value()) {
        throw std::runtime_error("Criteria are not read yet");
    }
    return criteria.value();
}

AHPRanker::Matrix2D AHPRanker::buildAvgMatrix(const std::vector<Matrix2D>& matrices) {
    size_t n = matrices.at(0).rows();

    for (const auto& matrix : matrices) {
        if (matrix.rows() != n || matrix.cols() != n) {
            throw std::invalid_argument("All matrices must have the same dimensions!");
        }
    }

    Matrix2D avgMatrix = Matrix2D::Zero(n, n);

    for (const auto& matrix : matrices) {
        avgMatrix += matrix;
    }

    avgMatrix /= matrices.size();

    return avgMatrix;
}
std::vector<double> AHPRanker::calculateRanking() {
    size_t n = comparison_matrix.value().rows();
    auto double_n = static_cast<double>(n);
    
    // Calculate weights
    std::vector<double> weights(n);
    for (size_t i = 0; i < n; ++i) {
        weights[i] = comparison_matrix.value().col(i).sum();
    }
    
    // Normalize matrix
    Matrix2D normMatrix(n, n);
    for (size_t i = 0; i < n; ++i) {
        normMatrix.col(i) = comparison_matrix.value().col(i) / weights[i];
    }
    
    // Criteria weights
    std::vector<double> criteriaWeights(n);
    for (size_t i = 0; i < n; ++i) {
        criteriaWeights[i] = normMatrix.row(i).sum() / double_n;
    }

    assert(std::accumulate(criteriaWeights.begin(), criteriaWeights.end(), 0.0) == 1.0);
    
    return criteriaWeights;
}
double AHPRanker::calculateConsistencyRatio() {
    if (!comparison_matrix.has_value()) {
        throw std::runtime_error("Comparison matrix not initialized");
    }
    
    size_t n = comparison_matrix.value().rows();

    // Calculate the principal eigenvalue
    Eigen::EigenSolver<Matrix2D> eigensolver(comparison_matrix.value());
    std::complex<double> principalEigenvalue = eigensolver.eigenvalues()[0];

    // Calculate the consistency index
    double consistencyIndex = (principalEigenvalue.real() - n) / (n - 1);

    // Calculate the consistency ratio
    double randomConsistencyIndex = getRandomConsistencyIndex(n);
    double consistencyRatio = consistencyIndex / randomConsistencyIndex;

    return consistencyRatio;
}

double AHPRanker::getRandomConsistencyIndex(size_t n) {
    // This table maps the size of the matrix to the corresponding random consistency index
    // The values are taken from the Saaty's scale
    static const std::map<size_t, double> randomConsistencyIndices = {
            {1, 0.0}, {2, 0.0}, {3, 0.58}, {4, 0.9}, {5, 1.12}, {6, 1.24},
            {7, 1.32}, {8, 1.41}, {9, 1.45}, {10, 1.49}
    };

    auto it = randomConsistencyIndices.find(n);
    if (it != randomConsistencyIndices.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("Matrix size is not supported");
    }
}