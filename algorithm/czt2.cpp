#include "czt2.h"
#include <cmath>
#include <complex>
#include <iostream>
#include <algorithm>
#include <omp.h>
using namespace std::complex_literals;


// 辅助函数，用于预计算
void precompute_czt_params(int m, int k, ComplexFloat w,
                           std::vector<ComplexFloat>& ww,
                           cv::Mat& mat_fv_dft,
                           int& nfft, int& m_minus_1)
{
    nfft = cv::getOptimalDFTSize(m + k - 1);
    int kk_start = -m + 1;
    int kk_end = std::max(k - 1, m - 1);
    m_minus_1 = m - 1;

    ww.resize(kk_end - kk_start + 1);
    for (int i = 0; i <= kk_end - kk_start; ++i) {
        float kk_val = static_cast<float>(kk_start + i);
        ww[i] = std::pow(w, (kk_val * kk_val) / 2.0f);
    }

    ComplexVectorFloat fv(nfft, 0.0f);
    for (int i = 0; i < m + k - 1; ++i) {
        fv[i] = 1.0f / ww[i];
    }

    cv::Mat mat_fv(fv.size(), 1, CV_32FC2, fv.data());
    mat_fv_dft.create(fv.size(), 1, CV_32FC2);
    cv::dft(mat_fv, mat_fv_dft);
}

// 优化的 CZT 执行函数
ComplexVectorFloat czt_opencv_float_exec(
    const ComplexVectorFloat& x,
    int k, ComplexFloat a,
    const std::vector<ComplexFloat>& ww,
    const cv::Mat& mat_fv_dft,
    int nfft, int m_minus_1)
{
    int m = x.size();
    if (m == 0) return {};

    ComplexVectorFloat y(nfft, 0.0f);
    for (int n = 0; n < m; ++n) {
        ComplexFloat aa_term = std::pow(a, -static_cast<float>(n));
        ComplexFloat ww_term = ww[m_minus_1 + n];
        y[n] = x[n] * aa_term * ww_term;
    }

    cv::Mat mat_y(y.size(), 1, CV_32FC2, y.data());
    cv::dft(mat_y, mat_y);

    // 关键优化：跳过了 fv 的计算和 dft
    cv::mulSpectrums(mat_y, mat_fv_dft, mat_y, 0);

    cv::idft(mat_y, mat_y, cv::DFT_SCALE);

    ComplexVectorFloat g(k);
    for (int i = 0; i < k; ++i) {
        cv::Vec2f vec = mat_y.at<cv::Vec2f>(m_minus_1 + i);
        ComplexFloat conv_result(vec[0], vec[1]);
        g[i] = conv_result * ww[m_minus_1 + i];
    }
    return g;
}


// --- czt2_opencv_float 的实现 ---
Czt2Results SignalProcessingUtils::czt2_opencv_float(const cv::Mat& estCosDelta, float windowRatio, float samplingRatio) {
    CV_Assert(estCosDelta.type() == CV_32F);
    int NR = estCosDelta.rows;
    int NC = estCosDelta.cols;

    float cztIntervalR = 2.0f * CV_PI / (samplingRatio * NC);
    float cztLengthR = samplingRatio * NC * windowRatio;
    float cztIntervalC = 2.0f * CV_PI / (samplingRatio * NR);
    float cztLengthC = samplingRatio * NR * windowRatio;

    int mc = static_cast<int>(std::round(cztLengthC));
    ComplexFloat wc = std::exp(-1.0if* cztIntervalC);
    ComplexFloat ac = std::exp(-1.0if* cztIntervalC* cztLengthC / 2.0f);

    int mr = static_cast<int>(std::round(cztLengthR));
    ComplexFloat wr = std::exp(1.0if* cztIntervalR);
    ComplexFloat ar = std::exp(1.0if* cztIntervalR* cztLengthR / 2.0f);

    // --- 第一个循环 (列处理) ---
    cv::Mat mat_a(mc, NC, CV_32FC2);
    std::vector<ComplexFloat> ww_col;
    cv::Mat mat_fv_dft_col;
    int nfft_col, m_minus_1_col;
    precompute_czt_params(NR, mc, wc, ww_col, mat_fv_dft_col, nfft_col, m_minus_1_col);

#pragma omp parallel for
    for (int j = 0; j < NC; ++j) {
        ComplexVectorFloat col_vec(NR);
        for (int i = 0; i < NR; ++i) {
            col_vec[i] = ComplexFloat(estCosDelta.at<float>(i, j), 0.0f);
        }

        ComplexVectorFloat czt_result_col = czt_opencv_float_exec(
            col_vec, mc, ac, ww_col, mat_fv_dft_col, nfft_col, m_minus_1_col
            );

        for (int i = 0; i < mc; ++i) {
            mat_a.at<cv::Vec2f>(i, j) = { czt_result_col[i].real(), czt_result_col[i].imag() };
        }
    }

    // --- 第二个循环 (行处理) ---
    cv::Mat mat_a_T;
    cv::transpose(mat_a, mat_a_T);
    cv::Mat mat_b(mr, mc, CV_32FC2);
    std::vector<ComplexFloat> ww_row;
    cv::Mat mat_fv_dft_row;
    int nfft_row, m_minus_1_row;
    precompute_czt_params(NC, mr, wr, ww_row, mat_fv_dft_row, nfft_row, m_minus_1_row);

#pragma omp parallel for
    for (int j = 0; j < mc; ++j) {
        ComplexVectorFloat row_vec(NC);
        for (int i = 0; i < NC; ++i) {
            cv::Vec2f val = mat_a_T.at<cv::Vec2f>(i, j);
            row_vec[i] = ComplexFloat(val[0], val[1]);
        }

        ComplexVectorFloat czt_result_row = czt_opencv_float_exec(
            row_vec, mr, ar, ww_row, mat_fv_dft_row, nfft_row, m_minus_1_row
            );

        for (int i = 0; i < mr; ++i) {
            mat_b.at<cv::Vec2f>(i, j) = { czt_result_row[i].real(), czt_result_row[i].imag() };
        }
    }


    Czt2Results results;
    cv::transpose(mat_b, results.output);
    results.cztIntervalR = cztIntervalR;
    results.cztLengthR = cztLengthR;
    results.cztIntervalC = cztIntervalC;
    results.cztLengthC = cztLengthC;

    return results;
}


