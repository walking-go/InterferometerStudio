#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include "deltaEstmationNPDA.h"

// 输出结果结构体
struct NPDAResult {
    cv::Mat estPhase;
    cv::Mat estA;
    cv::Mat estB;
    std::vector<double> estKxs;
    std::vector<double> estKys;
    std::vector<double> estDs;
    std::vector<double> historyEK;
    std::vector<double> historyEIt;
    int nIters;
};

/**
 * @brief 使用NPDA算法估计干涉图的相位、背景、调制度和倾斜移相系数
 * @param inputIs 输入的干涉图序列 (vector of CV_32FC1 Mat)
 * @param initCoeffs 初始估计系数
 * @param ROI感兴趣区域 (CV_8UC1 Mat)
 * @return 包含所有估计结果的 NPDAResult 结构体
 */
NPDAResult NPDA(
    const std::vector<cv::Mat>& inputIs,
    const CoeffsMDPD& initCoeffs,
    const cv::Mat& ROI);