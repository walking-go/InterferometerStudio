#pragma once 
#include <vector>
#include <complex>
#include <opencv2/core.hpp>

// --- 定义 float 版本的便捷类型别名 ---
using ComplexFloat = std::complex<float>;
using ComplexVectorFloat = std::vector<ComplexFloat>;

// --- 用于封装 czt2 函数返回值的结构体 ---
struct Czt2Results {
    cv::Mat output;            
    float cztIntervalR;
    float cztLengthR;
    float cztIntervalC;
    float cztLengthC;
};

// --- 主要的辅助类 ---
class SignalProcessingUtils {
public:
    // 构造函数
    SignalProcessingUtils() = default;

    Czt2Results czt2_opencv_float(const cv::Mat& estCosDelta, float windowRatio, float samplingRatio);

private:
    ComplexVectorFloat czt_opencv_float(const ComplexVectorFloat& x, int k, ComplexFloat w, ComplexFloat a);
};


