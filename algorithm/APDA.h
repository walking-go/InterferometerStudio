#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include "deltaEstmationNPDA.h"
#include "fringePatternNormalization.h"
#include "NPDA.h"
#include "PhaseProcessor.h"
#include "utils.h" 
cv::Mat APDA(
    const std::vector<cv::Mat>& noisedIs,
    const cv::Mat& mask,
    double maxTiltFactor
);
