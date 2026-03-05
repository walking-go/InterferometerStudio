#include "PhaseProcessor.h"
#include "PhaseUnwrapper.h"
#include <iostream>
#include "utils.h"
#include <chrono>
#include <cmath> 


PhaseProcessor::PhaseProcessor() { }

cv::Mat PhaseProcessor::postProcess(
    const cv::Mat& wrapped_phase,
    const cv::Mat& refPhase,
    const cv::Mat& mask,
    char type)
{
    CV_Assert(wrapped_phase.type() == CV_64FC1);
    CV_Assert(wrapped_phase.dims == 2 && !wrapped_phase.empty());
    cv::Mat effective_mask;
    if (mask.empty()) {
        effective_mask = cv::Mat(wrapped_phase.size(), CV_8UC1, cv::Scalar(255));
    }
    else {
        CV_Assert(mask.type() == CV_8UC1);
        CV_Assert(mask.size() == wrapped_phase.size());
        effective_mask = mask;
    }

    // 解包裹
    cv::Mat unwrapped_phase = m_unwrapper.unwrapPathMask(wrapped_phase, effective_mask);
    cv::Mat processed_phase = unwrapped_phase;
  
    // 移除倾斜项
    if (type == 'P') {
        processed_phase = removeTilt(processed_phase, effective_mask);
    }
    else {
        processed_phase = removeTiltSphere(processed_phase, effective_mask);
    }

    //二义性调整
    if (!refPhase.empty()) {
        CV_Assert(refPhase.size() == processed_phase.size() && refPhase.type() == processed_phase.type());
        double l1_norm_minus = cv::norm(processed_phase - refPhase, cv::NORM_L1);
        double l1_norm_plus = cv::norm(processed_phase + refPhase, cv::NORM_L1);

        if (l1_norm_minus > l1_norm_plus) {
            processed_phase *= -1.0;
        }
    }
    return processed_phase;
    
}

// 移除平移项和倾斜项 (Z1, Z2, Z3)
cv::Mat PhaseProcessor::removeTilt(const cv::Mat& phase, const cv::Mat& mask) {
    ZernikeCoreResult coreData = _zernikeFitCore(phase, mask, 4);
    cv::Mat tiltTerm = cv::Mat::zeros(phase.size(), phase.type());
    if (coreData.coeffs.size() >= 3) { 
        tiltTerm = coreData.zernikeTerms[0] * coreData.coeffs(0) +coreData.zernikeTerms[1] * coreData.coeffs(1) +
            coreData.zernikeTerms[2] * coreData.coeffs(2);
    }

// 计算校正相位
    cv::Mat correctedPhase = phase - tiltTerm;
    cv::Mat finalPhase = cv::Mat::zeros(phase.size(), phase.type());
    correctedPhase.copyTo(finalPhase, coreData.finalMask);

    return finalPhase;
}

//移除平移、倾斜和离焦项 (Z1, Z2, Z3, Z4)
cv::Mat PhaseProcessor::removeTiltSphere(const cv::Mat& phase, const cv::Mat& mask) {
    ZernikeCoreResult coreData = _zernikeFitCore(phase, mask, 4);

    cv::Mat correctionTerm = cv::Mat::zeros(phase.size(), phase.type());
    if (coreData.coeffs.size() >= 4) { 
        cv::Mat tiltTerm = coreData.zernikeTerms[0] * coreData.coeffs(0) +coreData.zernikeTerms[1] * coreData.coeffs(1) +
            coreData.zernikeTerms[2] * coreData.coeffs(2);
        cv::Mat sphericalTerm = coreData.zernikeTerms[3] * coreData.coeffs(3);
        correctionTerm = tiltTerm + sphericalTerm;
    }

    //计算校正相位
    cv::Mat correctedPhase = phase - correctionTerm;
    cv::Mat finalPhase = cv::Mat::zeros(phase.size(), phase.type());
    correctedPhase.copyTo(finalPhase, coreData.finalMask);

    return finalPhase;
}



//Zernike 辅助函数的实现
long long PhaseProcessor::factorial(int n) {
    if (n < 0) return -1; 
    if (n == 0) return 1;
    long long res = 1;
    for (int i = 2; i <= n; ++i) {
        res *= i;
    }
    return res;
}

//计算Zernike径向多项式
cv::Mat PhaseProcessor::zernikeRadial(int n, int m, const cv::Mat& R) {
    int abs_m = std::abs(m);
    cv::Mat ZR = cv::Mat::zeros(R.size(), CV_64FC1);

    if ((n - abs_m) % 2 != 0) {
        return ZR;
    }

    for (int s = 0; s <= (n - abs_m) / 2; ++s) {
        long double c = std::pow(-1.0, s) * static_cast<long double>(factorial(n - s));
        c /= (static_cast<long double>(factorial(s)) * static_cast<long double>(factorial((n + abs_m) / 2 - s)) * static_cast<long double>(factorial((n - abs_m) / 2 - s)));
        cv::Mat R_pow;
        cv::pow(R, n - 2 * s, R_pow);
        cv::add(ZR, R_pow * static_cast<double>(c), ZR);
    }
    return ZR;
}

 //计算Zernike基函数 
std::vector<cv::Mat> PhaseProcessor::zernikeBasis(int maxOrder, const cv::Mat& R, const cv::Mat& Theta) {
    int numTerms = (maxOrder + 1) * (maxOrder + 2) / 2;
    std::vector<cv::Mat> Z_terms;
    Z_terms.reserve(numTerms);

    int n = 0;
    int m = 0;

    while (n <= maxOrder) {
        if ((n - m) % 2 == 0) {
            cv::Mat ZR = zernikeRadial(n, m, R); 

            if (m == 0) {
                Z_terms.push_back(ZR);
            }
            else {
                cv::Mat cos_term, sin_term;
                cv::Mat mTheta = Theta * static_cast<double>(m);
                cv::Mat cos_mTheta(Theta.size(), CV_64FC1);
                cv::Mat sin_mTheta(Theta.size(), CV_64FC1);
                for (int r = 0; r < Theta.rows; ++r) {
                    for (int c = 0; c < Theta.cols; ++c) {
                        cos_mTheta.at<double>(r, c) = std::cos(mTheta.at<double>(r, c));
                        sin_mTheta.at<double>(r, c) = std::sin(mTheta.at<double>(r, c));
                    }
                }
                cv::multiply(ZR, cos_mTheta, cos_term);
                Z_terms.push_back(cos_term);
                cv::multiply(ZR, sin_mTheta, sin_term);
                Z_terms.push_back(sin_term);
            }
        }

        if (m < n) {
            m++;
        }
        else {
            n++;
            m = 0;
        }
    }
    CV_Assert(Z_terms.size() == numTerms);
    return Z_terms;
}


//执行Zernike拟合
PhaseProcessor::ZernikeCoreResult PhaseProcessor::_zernikeFitCore(const cv::Mat& estPhase, cv::Mat mask, int maxOrder) {
    const int rows = estPhase.rows;
    const int cols = estPhase.cols;
    const int numTerms = (maxOrder + 1) * (maxOrder + 2) / 2;
    CV_Assert(mask.size() == estPhase.size() && mask.type() == CV_8UC1);
    CV_Assert(estPhase.type() == CV_64FC1);
    cv::Mat X(rows, cols, CV_64FC1);
    cv::Mat Y(rows, cols, CV_64FC1);

    double x_scale = 1.0, y_scale = 1.0;
    if (rows > cols) { 
        x_scale = static_cast<double>(cols) / rows;
    }
    else if (cols > rows) { 
        y_scale = static_cast<double>(rows) / cols;
    }
    cv::Mat x_vec(1, cols, CV_64FC1);
    cv::Mat y_vec_trans(rows, 1, CV_64FC1);
    double x_denom = (cols > 1) ? static_cast<double>(cols - 1) : 1.0;
    double y_denom = (rows > 1) ? static_cast<double>(rows - 1) : 1.0;

    double* p_x_vec = x_vec.ptr<double>(0);
    for (int c = 0; c < cols; ++c) {
        p_x_vec[c] = (static_cast<double>(c) / x_denom * 2.0 - 1.0) * y_scale; 
    }
    double* p_y_vec = y_vec_trans.ptr<double>();
    for (int r = 0; r < rows; ++r) {
        p_y_vec[r] = (static_cast<double>(r) / y_denom * 2.0 - 1.0) * x_scale;
    }
    if (rows >= cols) {
        x_scale = 1.0;
        y_scale = static_cast<double>(rows) / cols;
    }
    else {
        x_scale = static_cast<double>(cols) / rows;
        y_scale = 1.0;
    }

    p_x_vec = x_vec.ptr<double>(0);
    for (int c = 0; c < cols; ++c) {
        p_x_vec[c] = (cols == 1) ? 0.0 : (static_cast<double>(c) / (cols - 1) * 2.0 - 1.0) * x_scale;
    }

    p_y_vec = y_vec_trans.ptr<double>();
    for (int r = 0; r < rows; ++r) {
        p_y_vec[r] = (rows == 1) ? 0.0 : (static_cast<double>(r) / (rows - 1) * 2.0 - 1.0) * y_scale;
    }
    X = cv::repeat(x_vec, rows, 1);
    Y = cv::repeat(y_vec_trans, 1, cols);
    cv::Mat R, Theta;
    cv::cartToPolar(X, Y, R, Theta, false); 
    std::vector<cv::Mat> zernikeTerms = zernikeBasis(maxOrder, R, Theta);
    cv::Mat R_mask;
    cv::compare(R, 1.0, R_mask, cv::CMP_LE); 
    cv::Mat finalMask;
    cv::bitwise_and(R_mask, mask, finalMask);

    //准备最小二乘拟合 (Eigen)
    const int N = cv::countNonZero(finalMask);
    if (N < numTerms) {
        std::cerr << "Warning: Not enough valid points (" << N << ") to fit "
            << numTerms << " Zernike terms. Returning zero correction." << std::endl;
        return { Eigen::VectorXd::Zero(numTerms), zernikeTerms, finalMask };
    }

    Eigen::MatrixXd Z_fit(numTerms, N);
    Eigen::VectorXd phase_vector(N);

    int valid_idx = 0;
    for (int r = 0; r < rows; ++r) {
        const uchar* pMask = finalMask.ptr<uchar>(r);
        const double* pPhase = estPhase.ptr<double>(r);

        for (int c = 0; c < cols; ++c) {
            if (pMask[c] > 0) {
                for (int term = 0; term < numTerms; ++term) {
                    Z_fit(term, valid_idx) = zernikeTerms[term].at<double>(r, c);
                }
                phase_vector(valid_idx) = pPhase[c];
                valid_idx++;
            }
        }
    }
    // 求解 (Eigen)
    Eigen::MatrixXd A = Z_fit * Z_fit.transpose();
    Eigen::VectorXd b = Z_fit * phase_vector;
    Eigen::VectorXd coeffs = A.ldlt().solve(b); 
    return { coeffs, zernikeTerms, finalMask };
}