#include "APDAProcessor.h"
#include "qglobal.h"
#include "utils.h"
#include "APDA.h"
#include <cmath>
#include <iostream>
#include <QDebug>
APDAProcessor::APDAProcessor()
{
}

cv::Mat APDAProcessor::run(
    const std::string& capturedPath,
    const std::string& fileType,
    int nCapture)
{
    std::cout<<"_______________________________________________________________";
    // === 图像读取 ===
    std::vector<cv::Mat> capturedIs = loadFiles(capturedPath, fileType, nCapture);

    // === 尺寸预处理 ===
    int nr = std::ceil((y2 - y1 + 1) * resizeFactor);
    int nc = std::ceil((x2 - x1 + 1) * resizeFactor);

    cv::Rect roi(x1, y1, x2 - x1 + 1, y2 - y1 + 1);

    std::vector<cv::Mat> noisedIs;
    for (int n = 0; n < nCapture; ++n)
    {
        cv::Mat image_roi;

        if (roi.x + roi.width > capturedIs[n].cols ||
            roi.y + roi.height > capturedIs[n].rows)
        {
            std::cerr << "ROI out of bounds!" << std::endl;
            continue;
        }

        capturedIs[n](roi).copyTo(image_roi);
        image_roi.convertTo(image_roi, CV_32F, 1 / 255.0);

        cv::Mat resizedImage;
        cv::resize(image_roi, resizedImage, cv::Size(nc, nr), 0, 0, cv::INTER_CUBIC);
        noisedIs.push_back(resizedImage);
    }

    // === mask ===
    cv::Mat mask = createMask(nr, nc);

    // === APDA 核心算法 ===
    cv::Mat finalPhase = APDA(noisedIs, mask, maxTiltFactor);
    cv::imshow("img",finalPhase);
    return finalPhase;
}

cv::Mat APDAProcessor::run(const std::vector<cv::Mat>& capturedImages)
{

    if (capturedImages.empty()) {
        std::cerr << "No images provided!" << std::endl;
        return cv::Mat();
    }

    int nCapture = static_cast<int>(capturedImages.size());

    // === 尺寸预处理 ===
    int nr = capturedImages[0].rows;
    int nc = capturedImages[0].rows;

    std::vector<cv::Mat> noisedIs;
    for (int n = 0; n < nCapture; ++n)
    {
        cv::Mat image_roi;

        capturedImages[n].copyTo(image_roi);
        image_roi.convertTo(image_roi, CV_32F, 1 / 255.0);

        cv::Mat resizedImage;
        cv::resize(image_roi, resizedImage, cv::Size(nc, nr), 0, 0, cv::INTER_CUBIC);
        noisedIs.push_back(resizedImage);
    }
    // === mask ===
    cv::Mat mask = createMask(nr, nc);

    // === APDA 核心算法 ===
    cv::Mat finalPhase = APDA(noisedIs, mask, maxTiltFactor);

    return finalPhase;
}
