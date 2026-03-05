#ifndef APDA_PROCESSOR_H
#define APDA_PROCESSOR_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

class APDAProcessor
{
public:
    APDAProcessor();

    // 主处理流程函数（等价于原来的 main 逻辑）
    cv::Mat run(
        const std::string& capturedPath,
        const std::string& fileType,
        int nCapture
        );

    // 从内存中的图像列表运行APDA算法
    cv::Mat run(const std::vector<cv::Mat>& capturedImages);

private:
    int x1 = 1, y1 = 1;
    int x2 = x1 + 511, y2 = y1 + 511;
    double resizeFactor = 0.5;

    double maxTiltFactor = 10;
};

#endif // APDA_PROCESSOR_H
