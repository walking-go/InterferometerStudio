#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

bool saveMatToBin(const cv::Mat& mat, const std::string& filename);

// 函数声明
std::vector<cv::Mat> loadFiles(const std::string& filePath, const std::string& type, int nCapture);
cv::Mat createMask(int height, int width);
std::string getMatType(const cv::Mat& mat);
void adaptiveImshow(const std::string& window_name, const cv::Mat& mat);
void cv32fc2Imshow(const std::string& window_name, const cv::Mat& mat);


cv::Mat createColorBar(int height, double minVal, double maxVal, int colormap);

// cv::Mat pseudoColor(const cv::Mat mat_);

cv::Mat showPseudoColorWithColorBar(const cv::Mat& inputMat, const std::string& windowTitle);

#endif 
