#pragma once
#include <vector>
#include <opencv2/opencv.hpp>

struct DeltaEstimationResult {
    std::vector<double> estKxs;
    std::vector<double> estKys;
    std::vector<double> estDs;
    std::vector<double> resErrs;
    std::vector<bool> reliable;
};

DeltaEstimationResult initCoeffEst_NLS(
    const std::vector<cv::Mat>& normalizedIs,
    double maxTiltFactor,
    const cv::Mat& mask
);
