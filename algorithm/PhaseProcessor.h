#pragma once

#include <opencv2/opencv.hpp>
#include "PhaseUnwrapper.h"
#include <Eigen/Dense>
#include <vector> 


// 封装相位后处理的完整流程
class PhaseProcessor {
public:
    PhaseProcessor();
    cv::Mat postProcess(
        const cv::Mat& wrapped_phase,
        const cv::Mat& refPhase,
        const cv::Mat& mask,
        char type = 'P'
    );

private:
    // 包含一个解包裹器实例，以调用 unwrapPathMask
    PhaseUnwrapper m_unwrapper;
    cv::Mat removeTilt(const cv::Mat& phase, const cv::Mat& mask);
    cv::Mat removeTiltSphere(const cv::Mat& phase, const cv::Mat& mask);


    // --- Zernike 拟合的私有辅助工具 ---
    struct ZernikeCoreResult {
        Eigen::VectorXd coeffs;
        std::vector<cv::Mat> zernikeTerms;
        cv::Mat finalMask;
    };

    // 内部核心函数：执行Zernike拟合
    ZernikeCoreResult _zernikeFitCore(const cv::Mat& estPhase, cv::Mat mask, int maxOrder);

    // 计算Zernike基函数
    std::vector<cv::Mat> zernikeBasis(int maxOrder, const cv::Mat& R, const cv::Mat& Theta);

    // 计算Zernike径向多项式
    cv::Mat zernikeRadial(int n, int m, const cv::Mat& R);

    // 计算阶乘
    long long factorial(int n);
};
