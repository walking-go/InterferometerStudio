#include "fringePatternNormalization.h"
#include <limits>
// #include "utils.h"
#include<QDebug>

// 主函数实现
NormalizationResult fringePatternNormalization(const std::vector<cv::Mat>& Is, const cv::Mat& mask) {
    if (Is.empty()) {
        throw std::invalid_argument("Input image sequence 'Is' cannot be empty.");
    }

    const int height = Is[0].rows;
    const int width = Is[0].cols;
    const size_t num_images = Is.size();

    cv::Mat erodedMask1;
    cv::Mat structuringElement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(11, 11));
    cv::Mat mask_8u;
    if (mask.type() != CV_8U) {
        cv::compare(mask, 0, mask_8u, cv::CMP_NE);
    }
    else {
        mask_8u = mask;
    }
    cv::erode(mask_8u, erodedMask1, structuringElement, cv::Point(-1, -1), 1, cv::BorderTypes::BORDER_CONSTANT, 0);
    cv::Mat erodedMask = erodedMask1 / 255.0;
    double max_val = -std::numeric_limits<double>::infinity();
    for (const auto& img : Is) {
        double current_max;
        cv::minMaxLoc(img, nullptr, &current_max);
        if (current_max > max_val) {
            max_val = current_max;
        }
    }
    double ampFactor = (max_val == 0) ? 1.0 : max_val;

    //浅拷贝
    std::vector<cv::Mat> normalizedIs = Is;
    for (int n = 0; n < Is.size(); n++)
    {
        Is[n].copyTo(normalizedIs[n]);
    }

    // // 创建深拷贝（避免修改原始数据），会寄
    // std::vector<cv::Mat> normalizedIs2;
    // normalizedIs2.reserve(Is.size());
    // for (const auto& img : Is) {
    //     normalizedIs2.push_back(img.clone());
    // }


    for (auto& img : normalizedIs)
    {
        img /= ampFactor;
    }


    // 计算 Imaxs 和 Imins (沿第一维的最大最小值)
    cv::Mat Imaxs = cv::Mat(height, width, CV_32F, -std::numeric_limits<double>::infinity());
    cv::Mat Imins = cv::Mat(height, width, CV_32F, std::numeric_limits<double>::infinity());

    for (const auto& img : normalizedIs)
    {
        cv::max(img, Imaxs, Imaxs);
        cv::min(img, Imins, Imins);
    }


    //中值滤波
    cv::Mat ImaxsMedFiltered, IminsMedFiltered;
    int kernel_size = 7;
    cv::Mat Imaxs_8U;
    Imaxs.convertTo(Imaxs_8U, CV_8U, 255.0);
    cv::Mat ImaxsMedFiltered_8U;
    cv::medianBlur(Imaxs_8U, ImaxsMedFiltered_8U, kernel_size);
    ImaxsMedFiltered_8U.convertTo(ImaxsMedFiltered, Imaxs.type(), 1.0 / 255.0);
    cv::Mat Imins_8U;
    Imins.convertTo(Imins_8U, CV_8U, 255.0);
    cv::Mat IminsMedFiltered_8U;
    cv::medianBlur(Imins_8U, IminsMedFiltered_8U, kernel_size);
    IminsMedFiltered_8U.convertTo(IminsMedFiltered, Imins.type(), 1.0 / 255.0);


    // 根据腐蚀后的掩码更新
    ImaxsMedFiltered.copyTo(Imaxs, erodedMask);
    IminsMedFiltered.copyTo(Imins, erodedMask);
    cv::Mat estA, estB;
    estA = 0.5 * (Imins + Imaxs);
    estB = 0.5 * (Imaxs - Imins);

    //归一化干涉图
    for (auto& img : normalizedIs)
    {
        cv::subtract(img, estA, img);
        cv::divide(img, estB, img);
    }

    //对每一幅归一化后的图像进行高斯滤波
    for (auto& img : normalizedIs) {
        cv::GaussianBlur(img, img, cv::Size(21,21),5,5,cv::BORDER_REPLICATE);
    }
    cv::Mat sumIs = cv::Mat::zeros(height, width, CV_32F);
    for (const auto& img : normalizedIs)
    {
        cv::add(sumIs, img, sumIs);
    }
    cv::Mat nanMask = cv::Mat(sumIs.size(), CV_8U);
    for (int r = 0; r < sumIs.rows; ++r) {
        for (int c = 0; c < sumIs.cols; ++c)
        {
            nanMask.at<uchar>(r, c) = std::isnan(sumIs.at<float>(r, c)) ? 255 : 0;
        }
    }
    cv::Mat notNanMask;
    cv::bitwise_not(nanMask, notNanMask);
    cv::Mat outputMask;
    cv::bitwise_and(mask_8u, notNanMask, outputMask);

    //将Is中的NaN值设为0
    for (auto& img : normalizedIs)
    {
        cv::patchNaNs(img, 0.0);
    }

    NormalizationResult result;
    result.normalizedIs = normalizedIs;
    result.ampFactor = ampFactor;
    result.estA = estA;
    result.estB = estB;
    result.outputMask = outputMask;
    return result;
}


// 不带mask的重载版本
NormalizationResult fringePatternNormalization(const std::vector<cv::Mat>& Is)
{
    if (Is.empty())
    {
        throw std::invalid_argument("Input image sequence 'Is' cannot be empty.");
    }
    cv::Mat defaultMask = cv::Mat::ones(Is[0].size(), CV_8U) * 255;
    return fringePatternNormalization(Is, defaultMask);
}
