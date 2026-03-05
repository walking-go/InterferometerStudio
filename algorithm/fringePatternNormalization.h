#ifndef FRINGE_NORMALIZATION_H
#define FRINGE_NORMALIZATION_H

#include <opencv2/opencv.hpp>
#include <vector>

// 用于封装多个返回值的结构体
struct NormalizationResult {
    std::vector<cv::Mat> normalizedIs;
    double ampFactor;
    cv::Mat estA;
    cv::Mat estB;
    cv::Mat outputMask;
};

NormalizationResult fringePatternNormalization(const std::vector<cv::Mat>& Is, const cv::Mat& mask);
NormalizationResult fringePatternNormalization(const std::vector<cv::Mat>& Is);
#endif 