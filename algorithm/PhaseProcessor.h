#pragma once

#include <opencv2/opencv.hpp>
#include "PhaseUnwrapper.h"
#include <Eigen/Dense>
#include <vector>
#include <cstdint>


// 封装相位后处理的完整流程
class PhaseProcessor {
public:
    PhaseProcessor();

    // Zernike term bit flags for removeZernikeTerms()
    static constexpr uint32_t ZERNIKE_PISTON      = 0x01;  // Z[0]
    static constexpr uint32_t ZERNIKE_TILT         = 0x02;  // Z[1], Z[2]
    static constexpr uint32_t ZERNIKE_POWER        = 0x04;  // Z[3]
    static constexpr uint32_t ZERNIKE_ASTIGMATISM  = 0x08;  // Z[4], Z[5]
    static constexpr uint32_t ZERNIKE_COMA         = 0x10;  // Z[6], Z[7]
    static constexpr uint32_t ZERNIKE_SPHERICAL    = 0x20;  // Z[10]

    cv::Mat postProcess(
        const cv::Mat& wrapped_phase,
        const cv::Mat& refPhase,
        const cv::Mat& mask,
        char type = 'P',
        cv::Mat* unwrapped_out = nullptr
    );

    // Selective Zernike term removal (operates on unwrapped phase)
    cv::Mat removeZernikeTerms(
        const cv::Mat& unwrapped_phase,
        const cv::Mat& mask,
        uint32_t removeMask
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
