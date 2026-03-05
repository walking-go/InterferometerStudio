#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#ifndef DELTA_ESTIMATION_H
#define DELTA_ESTIMATION_H

 //用于存储倾斜量和相位估计结果的数据结构。
struct EstimationResult {
    double estKx;       
    double estKy;       
    double estD;        
    cv::Mat estDelta;   
    double resErr;      
    cv::Mat estCosDelta; 
};

 //  用于两幅图像之间估计倾斜量及相位。
EstimationResult deltaEstmationNPDA_I1I2(const cv::Mat& I1, const cv::Mat& I2, double maxTiltFactor, const cv::Mat& ROI);


// 定义用于存储函数返回值的结构体
struct DeltaEstimationResult
{
    std::vector<double> estKxs;
    std::vector<double> estKys;
    std::vector<double> estDs;
    std::vector<double> resErrs;
    std::vector<cv::Mat> estDeltas;
    std::vector<cv::Mat> estPhase;
};
// 函数声明 (带 ROI 的版本)
DeltaEstimationResult deltaEstmationNPDA(
    const std::vector<cv::Mat>& Is,
    double maxTiltFactor,
    const cv::Mat& roi_input
);
// 函数声明 (不带 ROI 的重载版本)
DeltaEstimationResult deltaEstmationNPDA(
    const std::vector<cv::Mat>& Is,
    double maxTiltFactor);
// 这段代码应该放在头文件中
struct CoeffsMDPD {
    std::vector<double> kxs;
    std::vector<double> kys;
    std::vector<double> ds;
};
#endif 


