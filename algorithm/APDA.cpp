#include "APDA.h"
#include <iostream>
#include "deltaEstmationNPDA.h"
#include "fringePatternNormalization.h"
#include "NPDA.h"
#include "PhaseProcessor.h"
#include "utils.h"
#include <QElapsedTimer> // 用于计时
#include <QDebug>

cv::Mat APDA(
    const std::vector<cv::Mat>& noisedIs,
    const cv::Mat& mask,
    double maxTiltFactor
    )
{
    const double resThresh = 1;
    const int estSize = 256;
    const size_t nCapture = noisedIs.size();

    // 初值估计的尺寸预处理
    std::vector <cv::Mat> noisedIs256;
    double resizeFactor256 = static_cast<double>(estSize) / noisedIs[0].rows;

    for (size_t n = 0; n < nCapture; ++n)
    {
        cv::Mat resizedImage;
        cv::resize(noisedIs[n], resizedImage, cv::Size(estSize, estSize), 0, 0, cv::INTER_CUBIC);
        noisedIs256.push_back(resizedImage);
    }


    cv::Mat mask1 = createMask(estSize, estSize);
    for (auto& image : noisedIs256)
    {
        image = image.mul(mask1);
    }

    NormalizationResult result = fringePatternNormalization(noisedIs256, mask1);

    cv::Mat combinedMask;
    cv::Mat mask_8u;
    cv::Mat outputMask_8u;

    mask1.convertTo(mask_8u, CV_8U, 255.0);
    result.outputMask.convertTo(outputMask_8u, CV_8U);
    cv::bitwise_and(mask_8u, outputMask_8u, combinedMask);

    DeltaEstimationResult npdaResult = deltaEstmationNPDA(noisedIs256, maxTiltFactor, combinedMask);


    CoeffsMDPD initCoeffsMDPD;
    initCoeffsMDPD.kxs = npdaResult.estKxs;
    initCoeffsMDPD.kys = npdaResult.estKys;
    initCoeffsMDPD.ds = npdaResult.estDs;

    for (double& kx : initCoeffsMDPD.kxs) {
        kx *= resizeFactor256;
    }
    for (double& ky : initCoeffsMDPD.kys) {
        ky *= resizeFactor256;
    }

    // 图像取舍
    std::vector<cv::Mat> retainedIs;
    std::vector<double> retainedKxs;
    std::vector<double> retainedKys;
    std::vector<double> retainedDs;

    for (size_t i = 0; i < npdaResult.resErrs.size(); ++i) {
        if (npdaResult.resErrs[i] < resThresh) {
            retainedIs.push_back(noisedIs[i].clone());
            retainedKxs.push_back(initCoeffsMDPD.kxs[i]);
            retainedKys.push_back(initCoeffsMDPD.kys[i]);
            retainedDs.push_back(initCoeffsMDPD.ds[i]);
        }
    }
    qDebug()<<"__________________________有效图像数量："<<retainedDs.size();

    initCoeffsMDPD.kxs = std::move(retainedKxs);
    initCoeffsMDPD.kys = std::move(retainedKys);
    initCoeffsMDPD.ds = std::move(retainedDs);


    //  NPDA 迭代
    cv::Mat maskForNPDA;
    mask.convertTo(maskForNPDA, CV_8UC1, 255);


    NPDAResult temp = NPDA(retainedIs, initCoeffsMDPD, maskForNPDA);



    //后处理
    PhaseProcessor processor;
    cv::Mat finalPhase = processor.postProcess(temp.estPhase, temp.estPhase, maskForNPDA);




    return finalPhase;
}
