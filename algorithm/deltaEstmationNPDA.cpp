#include "deltaEstmationNPDA.h"
#include <iostream>
#include <vector>
#include "czt2.h"
#include <complex>
#include <opencv2/opencv.hpp>
#include"utils.h"
#include <cmath>
#include <iomanip>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <string>
#include <algorithm>
#include <sstream>

#include <omp.h>

// 辅助函数：实现 MATLAB 的 sign() 功能
cv::Mat sign(const cv::Mat& src) {
    cv::Mat dst = cv::Mat::zeros(src.size(), src.type());
    cv::Mat greater_mask, lesser_mask;
    cv::compare(src, 0, greater_mask, cv::CMP_GT);
    cv::compare(src, 0, lesser_mask, cv::CMP_LT);

    dst.setTo(1.0f, greater_mask);
    dst.setTo(-1.0f, lesser_mask);

    return dst;
}

void meshgrid(int NC, int NR, cv::Mat& x_grid, cv::Mat& y_grid)
{
    const int grid_w = NC + 2;
    const int grid_h = NR + 2;

    cv::Mat x_row(1, grid_w, CV_32F);
    for (int i = 0; i < grid_w; ++i) {
        x_row.at<float>(0, i) = static_cast<float>(i + 1);
    }
    cv::repeat(x_row, grid_h, 1, x_grid);
    cv::Mat y_col(grid_h, 1, CV_32F);
    for (int i = 0; i < grid_h; ++i) {
        y_col.at<float>(i, 0) = static_cast<float>(i + 1);
    }
    cv::repeat(y_col, 1, grid_w, y_grid);
}

cv::Mat linspace(float start, float end, int count) {
    if (count <= 0) {
        return cv::Mat();
    }
    if (count == 1) {
        return cv::Mat(1, 1, CV_32F, end);
    }
    cv::Mat result(1, count, CV_32F);
    float step = (end - start) / (count - 1);
    for (int i = 0; i < count; ++i) {
        result.at<float>(0, i) = start + i * step;
    }
    return result;
}


cv::Mat diff(const cv::Mat& mat, int order, int dim) {
    if (order != 1) {
        throw std::invalid_argument("此 diff 实现仅支持一阶差分。");
    }
    if (dim == 1) {
        return mat.rowRange(1, mat.rows) - mat.rowRange(0, mat.rows - 1);
    }
    else if (dim == 2) {
        return mat.colRange(1, mat.cols) - mat.colRange(0, mat.cols - 1);
    }
    else {
        throw std::invalid_argument("维度 dim 必须是 1 或 2。");
    }
}


void meshgrid(const cv::Mat& xvec, const cv::Mat& yvec, cv::Mat& X, cv::Mat& Y)
{
    cv::Mat x_row = xvec.reshape(1, 1);
    cv::Mat y_col = yvec.reshape(1, yvec.total());

    cv::repeat(x_row, y_col.rows, 1, X);
    cv::repeat(y_col, 1, x_row.cols, Y);
}
using ComplexFloat = std::complex<float>;

// 用于封装 centroid1Spectrum 函数返回值的结构体
struct CentroidResult {
    cv::Point2f p;
    ComplexFloat v;
};


void fftshift(cv::Mat& mat)
{
    if (mat.empty()) {
        return;
    }
    int cx = mat.cols / 2;
    int cy = mat.rows / 2;
    cv::Mat q0(mat, cv::Rect(0, 0, cx, cy));
    cv::Mat q1(mat, cv::Rect(cx, 0, cx, cy));
    cv::Mat q2(mat, cv::Rect(0, cy, cx, cy));
    cv::Mat q3(mat, cv::Rect(cx, cy, cx, cy));

    cv::Mat tmp;
    q0.copyTo(tmp);
    q3.copyTo(q0);
    tmp.copyTo(q3);
    q1.copyTo(tmp);
    q2.copyTo(q1);
    tmp.copyTo(q2);
}


float calculateMedian(const cv::Mat& mat)
{
    if (mat.empty() || mat.channels() != 1) {
        return 0.0f;
    }
    if (mat.type() == CV_8UC1) {
        int histSize = 256;
        float range[] = { 0, 256 };
        const float* histRange = { range };
        cv::Mat hist;
        cv::calcHist(&mat, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
        long total = mat.total();
        long median_threshold = total / 2;
        long sum = 0;
        for (int i = 0; i < histSize; i++) {
            sum += static_cast<long>(hist.at<float>(i));
            if (sum >= median_threshold) {
                return static_cast<float>(i);
            }
        }
        return 255.0f;
    }
    else {
        cv::Mat flat;
        mat.reshape(1, mat.total()).convertTo(flat, CV_32F);
        std::vector<float> vec;
        flat.copyTo(vec);
        size_t n = vec.size();
        if (n == 0) {
            return 0.0f;
        }
        if (n % 2 == 0) {
            auto mid1_it = vec.begin() + n / 2 - 1;
            auto mid2_it = vec.begin() + n / 2;
            std::nth_element(vec.begin(), mid1_it, vec.end());
            float mid1_val = *mid1_it;
            std::nth_element(vec.begin(), mid2_it, vec.end());
            return (mid1_val + *mid2_it) / 2.0f;
        }
        else {
            auto mid_it = vec.begin() + n / 2;
            std::nth_element(vec.begin(), mid_it, vec.end());
            return *mid_it;
        }
    }
}



CentroidResult centroid1Spectrum(const cv::Mat& b) {
    CentroidResult result;

    if (b.empty()) {
        std::cerr << "警告: centroid1Spectrum 输入矩阵为空!" << std::endl;
        result.p = cv::Point2f(-1, -1);
        result.v = ComplexFloat(0, 0);
        return result;
    }

    cv::Mat magnitude;
    std::vector<cv::Mat> channels(2);
    cv::split(b, channels);
    cv::magnitude(channels[0], channels[1], magnitude);

    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(magnitude, &minVal, &maxVal, &minLoc, &maxLoc);
    result.p = cv::Point2f(static_cast<float>(maxLoc.x), static_cast<float>(maxLoc.y));
    cv::Vec2f vec_v = b.at<cv::Vec2f>(maxLoc.y, maxLoc.x);
    result.v = ComplexFloat(vec_v[0], vec_v[1]);

    return result;
}

using Complex = std::complex<double>;

float findHistogramPeak(const cv::Mat& data, int num_bins) {
    if (data.empty() || data.channels() != 1) {
        std::cerr << "警告: findHistogramPeak 输入数据为空或不是单通道矩阵!" << std::endl;
        return 0.0f;
    }
    double minVal, maxVal;
    cv::minMaxLoc(data, &minVal, &maxVal);

    cv::Mat hist_counts;
    int histSize[] = { num_bins };
    float range[] = { static_cast<float>(minVal), static_cast<float>(maxVal) };
    const float* histRange[] = { range };
    int channels[] = { 0 };
    cv::calcHist(&data, 1, channels, cv::Mat(), hist_counts, 1, histSize, histRange, true, false);

    double maxHistVal;
    cv::Point maxIdx;
    cv::minMaxLoc(hist_counts, nullptr, &maxHistVal, nullptr, &maxIdx);

    int peak_bin_index = maxIdx.y;
    float bin_width = (maxVal - minVal) / num_bins;
    float peak_center = static_cast<float>(minVal + peak_bin_index * bin_width + bin_width / 2.0);
    return peak_center;
}

EstimationResult deltaEstmationNPDA_I1I2(const cv::Mat& I1, const cv::Mat& I2, double maxTiltFactor, const cv::Mat& ROI, size_t loop_index)
{
    if (I1.size() != I2.size() || I2.type() != I1.type()) {
        throw std::invalid_argument("输入图像 I1 和 I2 必须具有相同的尺寸和类型。");
    }

    int H = I1.rows;
    int W = I1.cols;

    if (H <= 2 || W <= 2) {
        throw std::invalid_argument("输入图像的尺寸必须大于 2x2。");
    }

    EstimationResult result;

    // 倾斜量估计
    cv::Mat P, Q, R;
    cv::pow(I1, 2, P);
    cv::pow(I2, 2, Q);
    R = I1.mul(I2);
    //saveMatToBin(R, "R");
    // 创建差分矩阵
    cv::Rect center_rect(1, 1, W - 2, H - 2);
    cv::Mat Pc = P(center_rect);
    cv::Mat Qc = Q(center_rect);
    cv::Mat Rc = R(center_rect);

    // 定义邻域位置的 Rect
    cv::Rect lu_rect(0, 0, W - 2, H - 2);
    cv::Rect u_rect(1, 0, W - 2, H - 2);
    cv::Rect l_rect(0, 1, W - 2, H - 2);
    cv::Rect r_rect(2, 1, W - 2, H - 2);
    cv::Rect b_rect(1, 2, W - 2, H - 2);
    cv::Rect rb_rect(2, 2, W - 2, H - 2);


    // 创建一个包含所有差分方向的向量
    std::vector<cv::Mat> PStackedDiff, QStackedDiff, RStackedDiff;
    PStackedDiff.push_back(P(r_rect) - Pc);
    PStackedDiff.push_back(P(r_rect) - P(l_rect));
    PStackedDiff.push_back(P(b_rect) - Pc);
    PStackedDiff.push_back(P(b_rect) - P(u_rect));
    PStackedDiff.push_back(P(rb_rect) - Pc);
    PStackedDiff.push_back(P(rb_rect) - P(lu_rect));

    QStackedDiff.push_back(Q(r_rect) - Qc);
    QStackedDiff.push_back(Q(r_rect) - Q(l_rect));
    QStackedDiff.push_back(Q(b_rect) - Qc);
    QStackedDiff.push_back(Q(b_rect) - Q(u_rect));
    QStackedDiff.push_back(Q(rb_rect) - Qc);
    QStackedDiff.push_back(Q(rb_rect) - Q(lu_rect));

    RStackedDiff.push_back(R(r_rect) - Rc);
    RStackedDiff.push_back(R(r_rect) - R(l_rect));
    RStackedDiff.push_back(R(b_rect) - Rc);
    RStackedDiff.push_back(R(b_rect) - R(u_rect));
    RStackedDiff.push_back(R(rb_rect) - Rc);
    RStackedDiff.push_back(R(rb_rect) - R(lu_rect));

    //从6组中选取最优质的一组
    int innerH = H - 2;
    int innerW = W - 2;
    cv::Mat PDiff = cv::Mat::zeros(innerH, innerW, P.type());
    cv::Mat QDiff = cv::Mat::zeros(innerH, innerW, Q.type());
    cv::Mat RDiff = cv::Mat::zeros(innerH, innerW, R.type());

    for (int r = 0; r < innerH; ++r) {
        for (int c = 0; c < innerW; ++c) {
            double maxVal = -1.0;
            int maxIdx = 0;
            for (int i = 0; i < 6; ++i) {
                double val = std::abs(RStackedDiff[i].at<float>(r, c));
                if (val > maxVal) {
                    maxVal = val;
                    maxIdx = i;
                }
            }
            PDiff.at<float>(r, c) = PStackedDiff[maxIdx].at<float>(r, c);
            QDiff.at<float>(r, c) = QStackedDiff[maxIdx].at<float>(r, c);
            RDiff.at<float>(r, c) = RStackedDiff[maxIdx].at<float>(r, c);
        }
    }


    //计算 estCosDelta
    cv::Mat estCosDelta = (PDiff + QDiff) / (2.0 * RDiff);
    cv::patchNaNs(estCosDelta, 0.0);
    cv::Mat ROICropped = ROI(center_rect);
    cv::Mat mask;
    cv::compare(ROICropped, 0, mask, cv::CMP_EQ);
    estCosDelta.setTo(0, mask);

    cv::min(estCosDelta, 1.0, estCosDelta);
    cv::max(estCosDelta, -1.0, estCosDelta);
    result.estCosDelta = estCosDelta;
    //saveMatToBin(estCosDelta,"estCosDelta");
    //showPseudoColorWithColorBar(estCosDelta,"estCosDelta");
    //cv::waitKey(0);

    // --- 基于CZT的Kx Ky估计 ---
    double windowRatio = maxTiltFactor / 250.0;
    double samplingRatio = (250.0 / maxTiltFactor) * 512.0 / H;
    SignalProcessingUtils processor;
    //start_time = clock();   //获取开始执行时间

    Czt2Results czt2_results = processor.czt2_opencv_float(estCosDelta, static_cast<float>(windowRatio), static_cast<float>(samplingRatio));
    //end_time = clock();     //获取结束时间
    //double Times = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    //printf("czt2: %f seconds\n", Times);
    //// 从结构体中提取各个输出值
    //Czt2Results czt2_results = SignalProcessingUtils::czt2_opencv_float(estCosDelta,static_cast<float>(windowRatio),static_cast<float>(samplingRatio));

    cv::Mat cztI = czt2_results.output;
    float cztIntervalR = czt2_results.cztIntervalR;
    float cztLengthR = czt2_results.cztLengthR;
    float cztIntervalC = czt2_results.cztIntervalC;
    float cztLengthC = czt2_results.cztLengthC;

    // //步骤 1: 将复数矩阵分离成实部和虚部
    //std::vector<cv::Mat> planes;
    //cv::split(cztI, planes); // planes[0] 是实部, planes[1] 是虚部

    //// 步骤 2: 计算幅度
    //cv::Mat magnitudeImage;
    //cv::magnitude(planes[0], planes[1], magnitudeImage); // magnitudeImage 是 CV_32F 类型

    //// 步骤 3: 将浮点类型的幅度值归一化到 [0, 255] 的8位灰度图
    //cv::Mat displayImage;
    //cv::normalize(magnitudeImage, displayImage, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    //// 步骤 4: 显示处理后的图像，这不会再报错
    //cv::imshow("CZT Magnitude", displayImage); // 使用处理后的 displayImage
    //cv::waitKey(0);


    CentroidResult p1_result = centroid1Spectrum(cztI);
    cv::Point2f p1 = p1_result.p; // p1.x 是列坐标, p1.y 是行坐标

    // MATLAB: estKx = (p1(1) - cztLengthR/2) * cztIntervalR;
    // p1(1) 在 MATLAB 中是 x 坐标 (列)
    double estKx1 = ((float)p1.x + 1 - cztLengthR / 2.0f) * cztIntervalR;
    double estKy1 = -((float)p1.y + 1 - cztLengthC / 2.0f) * cztIntervalC;

    //我们可以打印一下结果来验证
    /*std::cout << "cztLengthR:" << cztLengthR << std::endl;
   std::cout << "cztLengthC:" << cztLengthC << std::endl;
   std::cout << "cztIntervalR:" << cztIntervalR << std::endl;
   std::cout << "cztIntervalC:" << cztIntervalC << std::endl;*/
    result.estKx = estKx1;
    result.estKy = estKy1;
    //std::cout << "estKx1" << estKx1 << std::endl;
    //std::cout << "estKy1" << estKy1 << std::endl;


    if ((std::abs(estKx1) * 256 < 2 * CV_PI) && (std::abs(estKy1) * 256 <2 * CV_PI))
    {
        // --- 低通滤波 ---
        cv::Mat estCosDelta_LPF = estCosDelta;
        int H_inner = estCosDelta_LPF.rows;
        int W_inner = estCosDelta_LPF.cols;

        cv::Mat F;
        cv::dft(estCosDelta_LPF, F, cv::DFT_COMPLEX_OUTPUT);
        fftshift(F);
        //cv32fc2Imshow("123", F);
        //showPseudoColorWithColorBar(estCosDelta_LPF,"estCosDelta_LPF");
        //cv::waitKey(0);
        /* cv::Vec2f F_pixel = F.at<cv::Vec2f>(debug_row, debug_col);
         std::cout << "F (频域矩阵) at (" << debug_row << "," << debug_col << "): (" << F_pixel[0] << ", " << F_pixel[1] << "i)" << std::endl;*/

        // MATLAB: cutoff_frequency = 0.05 * 256/W;
        float cutoff_frequency = 0.05f * 256.0f / static_cast<float>(W);
        cv::Mat x_vec = linspace(-1.0f, 1.0f, W_inner);
        cv::Mat y_vec = linspace(-1.0f, 1.0f, H_inner);
        cv::Mat x_norm, y_norm;
        meshgrid(x_vec, y_vec, x_norm, y_norm);

        cv::Mat D2, D;
        cv::pow(x_norm, 2, x_norm);
        cv::pow(y_norm, 2, y_norm);
        cv::add(x_norm, y_norm, D2);
        cv::sqrt(D2, D);
        //showPseudoColorWithColorBar(D, "D");
        //cv::waitKey(0);

        // MATLAB: M = exp(-(D.^2) / (2 * cutoff_frequency^2));
        cv::Mat M;
        cv::pow(D, 2, D2);
        cv::exp(-D2 / (2 * cutoff_frequency * cutoff_frequency), M);
        //showPseudoColorWithColorBar(M, "M");
        //cv::waitKey(0);

        // MATLAB: estCosDelta = real(ifft2(ifftshift(F .* M)));
        std::vector<cv::Mat> F_channels;
        cv::split(F, F_channels); // F is CV_32FC2
        cv::multiply(F_channels[0], M, F_channels[0]); // real part * M
        cv::multiply(F_channels[1], M, F_channels[1]); // imag part * M
        cv::merge(F_channels, F);

        fftshift(F);
        cv::idft(F, estCosDelta_LPF, cv::DFT_REAL_OUTPUT | cv::DFT_SCALE);
        //showPseudoColorWithColorBar(estCosDelta_LPF, "estCosDelta_LPF");
        //cv::waitKey(0);

        // --- 估计 kx^2 ---
        cv::Mat numer_kx, estKx2;
        cv::pow(diff(estCosDelta_LPF, 1, 2), 2, numer_kx);
        cv::Mat denom_kx_base;
        cv::pow(estCosDelta_LPF.colRange(0, W_inner - 1), 2, denom_kx_base);
        cv::Mat denom_kx = 1 - denom_kx_base + DBL_EPSILON;
        cv::divide(numer_kx, denom_kx, estKx2);
        //showPseudoColorWithColorBar(estKx2, "estKx2");
        //cv::waitKey(0);

        cv::Mat sqrt_estKx2;
        cv::sqrt(estKx2, sqrt_estKx2);
        float estKx_grad = std::sqrt(calculateMedian(estKx2)) / 2.0f + findHistogramPeak(sqrt_estKx2, 1000) / 2.0f;

        // --- 估计 ky^2 ---
        cv::Mat numer_ky, estKy2;
        cv::pow(diff(estCosDelta_LPF, 1, 1), 2, numer_ky);
        cv::Mat denom_ky_base;
        cv::pow(estCosDelta_LPF.rowRange(0, H_inner - 1), 2, denom_ky_base);
        cv::Mat denom_ky = 1 - denom_ky_base + DBL_EPSILON;
        cv::divide(numer_ky, denom_ky, estKy2);
        cv::Mat sqrt_estKy2;
        cv::sqrt(estKy2, sqrt_estKy2);
        float estKy_grad = std::sqrt(calculateMedian(estKy2)) / 2.0f + findHistogramPeak(sqrt_estKy2, 1000) / 2.0f;


        // --- 估计 kx*ky ---
        cv::Mat estCosDelta_sub1 = estCosDelta_LPF(cv::Rect(0, 0, W_inner - 1, H_inner - 1));
        cv::Mat estCosDelta_sub2 = estCosDelta_LPF(cv::Rect(1, 1, W_inner - 1, H_inner - 1));
        cv::Mat numer_kxky, estKxpKy2;
        cv::pow(estCosDelta_sub2 - estCosDelta_sub1, 2, numer_kxky);
        cv::Mat estCosDelta_pow;
        cv::pow(estCosDelta_sub1, 2.0, estCosDelta_pow);
        cv::Mat denom_kxky = 1 - estCosDelta_pow + DBL_EPSILON;
        //showPseudoColorWithColorBar(denom_ky, "Pseudo Color Image with Color Bar");
        //cv::waitKey(0);
        cv::divide(numer_kxky, denom_kxky, estKxpKy2);
        cv::Mat estKx2_sub = estKx2(cv::Rect(0, 0, W_inner - 1, H_inner - 1));
        cv::Mat estKy2_sub = estKy2(cv::Rect(0, 0, W_inner - 1, H_inner - 1));
        cv::Mat estKxKy = (estKxpKy2 - estKx2_sub - estKy2_sub) / 2.0;
        //saveMatToBin(estKxKy,"estKxKy");
        //showPseudoColorWithColorBar(estKxKy, "estKxKy");
        //cv::waitKey(0);

        // --- ky的符号问题解决 ---
        if (calculateMedian(estKxKy) / 2.0 + findHistogramPeak(estKxKy, 1000) / 2.0 < 0) {
            estKy_grad = -estKy_grad;
        }
        result.estKx = estKx_grad;
        result.estKy = estKy_grad;
    }
    //std::cout << "--- 循环索引 [" << loop_index << "] 的估算结果 ---" << std::endl;

    //// 设置打印格式：使用定点表示法，并保留8位小数
    //std::cout << std::fixed << std::setprecision(5);

    //// 打印结果
    //std::cout << "  estKx: " << result.estKx << std::endl;
    //std::cout << "  estKy: " << result.estKy << std::endl;
    //std::cout << "-------------------------------------------\n" << std::endl;




    cv::Mat X, Y;
    cv::Mat x_range = cv::Mat(1, innerW, CV_32F);
    cv::Mat y_range = cv::Mat(1, innerH, CV_32F);

    for (int i = 0; i < innerW; ++i) { x_range.at<float>(0, i) = static_cast<float>(i); }
    for (int i = 0; i < innerH; ++i) { y_range.at<float>(0, i) = static_cast<float>(i); }
    meshgrid(x_range, y_range, X, Y);

    cv::Mat q_phase_map = result.estKx * X + result.estKy * Y;
    int num_pixels = innerW * innerH;
    cv::Mat Q_mat(num_pixels, 2, CV_32F);
    cv::Mat transposed_map = q_phase_map.t();
    cv::Mat q_flat = transposed_map.reshape(1, num_pixels);

    for (int i = 0; i < num_pixels; ++i) {
        float q_val = q_flat.at<float>(i, 0);
        Q_mat.at<float>(i, 0) = -std::sin(q_val);
        Q_mat.at<float>(i, 1) = std::cos(q_val);
    }

    cv::Mat transposed_estCosDelta = estCosDelta.t();
    cv::Mat P_vec = transposed_estCosDelta.reshape(1, num_pixels);


    cv::Mat R_solution;
    cv::solve(Q_mat, P_vec, R_solution, cv::DECOMP_QR);
    float R1 = R_solution.at<float>(0, 0);
    float R2 = R_solution.at<float>(1, 0);
    double estD = std::atan2(R1, R2);


    if (estD < 0) {
        estD += 2 * CV_PI;
    }

    result.estD = estD;
    cv::Mat  RDiff_8U, binaryRDiff, validMask;
    cv::normalize(RDiff, RDiff_8U, 0, 255, cv::NORM_MINMAX, CV_8U);
    cv::threshold(RDiff_8U, binaryRDiff, 0,255, cv::THRESH_BINARY + cv::THRESH_OTSU);
    //adaptiveImshow("123", binaryRDiff);
    //cv::Mat ROI1 = ROICropped / 255.0;
    cv::bitwise_and(binaryRDiff, ROICropped, validMask);
    //saveMatToBin(validMask,"validMask10");
    //showPseudoColorWithColorBar(binaryRDiff," binaryRDiff");
    //cv::waitKey(0);
    const int NR = estCosDelta.rows;
    const int NC = estCosDelta.cols;
    cv::Mat x_grid, y_grid;
    meshgrid(NC, NR, x_grid, y_grid);
    cv::Mat estDelta = result.estKx * (x_grid +1) + result.estKy* (y_grid + 1) + estD;
    result.estDelta = estDelta;
    //std::cout << "estDelta类型：" << getMatType(estDelta) << std::endl;
    //adaptiveImshow("123", estDelta);
    cv::Rect roi_rect(1, 1, NC, NR);
    cv::Mat cropEstDelta = estDelta(roi_rect);
    //showPseudoColorWithColorBar(cropEstDelta, "cropEstDelta");
    //cv::waitKey(0);
    std::vector<float> resErr1;
    int valid_pixel_count = cv::countNonZero(validMask);
    resErr1.reserve(valid_pixel_count);
    for (int c = 0; c < NR; ++c) {
        for (int r = 0; r < NC; ++r) {
            if (validMask.at<uchar>(r, c)) {
                float est_val = cropEstDelta.at<float>(r, c);
                float cos_est_val = std::cos(est_val);
                float actual_val = estCosDelta.at<float>(r, c);
                resErr1.push_back(cos_est_val- actual_val);
            }
        }
    }

    float sum_sq_err = 0.0f;
    for (float error_value : resErr1) {
        sum_sq_err += error_value * error_value;
    }

    float resErr = 0.0f;
    if (valid_pixel_count > 0) {
        resErr = sum_sq_err / valid_pixel_count;
    }
    result.resErr = resErr;
    return result;
}


// 主函数: deltaEstmationNPDA 的实现
// 实现不带 ROI 的重载版
DeltaEstimationResult deltaEstmationNPDA(const std::vector<cv::Mat>&Is, double maxTiltFactor)
{
    // 检查输入是否为空
    if (Is.empty()) {
        throw std::invalid_argument("输入图像序列 'Is' 不能为空。");
    }
    // 创建一个默认的、覆盖整个图像的 ROI
    // MATLAB 的 ones(size) -> OpenCV 创建一个全1的Mat
    // 掩码通常是 CV_8U 类型，值为 255
    cv::Mat defaultROI = cv::Mat(Is[0].size(), CV_8U, cv::Scalar(255));
    // 调用带 ROI 的版本
    return deltaEstmationNPDA(Is, maxTiltFactor, defaultROI);
}


//实现核心功能的、带 ROI 的版本
DeltaEstimationResult deltaEstmationNPDA(const std::vector<cv::Mat>& Is, double maxTiltFactor, const cv::Mat& roi_input)
{
    std::cout << "开始执行：NPDA初值估计" << std::endl;
    if (Is.empty()) {
        throw std::invalid_argument("输入图像序列 'Is' 不能为空。");
    }

    cv::Mat ROI = roi_input.clone();

    if (ROI.type() != CV_8U) {
        ROI.convertTo(ROI, CV_8U, 255.0);
    }

    cv::compare(ROI, 0, ROI, cv::CMP_NE);
    DeltaEstimationResult result;
    const size_t numImages = Is.size();
    const int height = Is[0].rows;
    const int width = Is[0].cols;

    result.estKxs.resize(numImages, 0.0);
    result.estKys.resize(numImages, 0.0);
    result.estDs.resize(numImages, 0.0);
    result.resErrs.resize(numImages, 0.0);
    result.estDeltas.resize(numImages);
    result.estPhase.resize(numImages);

    for (size_t i = 1; i < numImages; ++i) {
        result.estDeltas[i] = cv::Mat::zeros(height, width, CV_32F);
        result.estPhase[i] = cv::Mat::zeros(height, width, CV_32F);
    }



    //循环计算与结果存储
    const cv::Mat& I1 = Is[0];

    const int loopNum = static_cast<int>(numImages);
#pragma omp parallel for
    for (int n = 1; n < loopNum; ++n) {
        const cv::Mat& I2 = Is[n];
        EstimationResult frame_result = deltaEstmationNPDA_I1I2(I1, I2, maxTiltFactor, ROI, n);
        result.estKxs[n] = frame_result.estKx;
        result.estKys[n] = frame_result.estKy;
        result.estDs[n] = frame_result.estD;
        result.resErrs[n] = frame_result.resErr;
        result.estDeltas[n] = frame_result.estDelta;


        // --- 计算并存储相位图 (estPhase) ---
        {
            const cv::Mat& deltaMat = frame_result.estDelta;
            cv::Mat cosDelta(deltaMat.size(), deltaMat.type());
            cv::Mat sinDelta(deltaMat.size(), deltaMat.type());

            CV_Assert(deltaMat.type() == CV_32F && I1.type() == CV_32F && I2.type() == CV_32F);

            for (int y = 0; y < deltaMat.rows; ++y) {
                const float* delta_ptr = deltaMat.ptr<float>(y);
                float* cos_ptr = cosDelta.ptr<float>(y);
                float* sin_ptr = sinDelta.ptr<float>(y);
                for (int x = 0; x < deltaMat.cols; ++x) {
                    const float pixel_value = delta_ptr[x];
                    cos_ptr[x] = std::cos(pixel_value);
                    sin_ptr[x] = std::sin(pixel_value);
                }
            }

            cv::Mat sign_sin_delta = sign(sinDelta);
            cv::Mat y_arg = (I1.mul(cosDelta) - I2).mul(sign_sin_delta);
            cv::Mat x_arg = (I1.mul(sinDelta)).mul(sign_sin_delta);
            cv::Mat& phaseMat = result.estPhase[n];
            CV_Assert(phaseMat.size() == y_arg.size() && phaseMat.type() == y_arg.type());

            for (int y = 0; y < y_arg.rows; ++y) {
                const float* y_ptr = y_arg.ptr<float>(y);
                const float* x_ptr = x_arg.ptr<float>(y);
                float* phase_ptr = phaseMat.ptr<float>(y);

                for (int x = 0; x < y_arg.cols; ++x) {
                    phase_ptr[x] = std::atan2(y_ptr[x], x_ptr[x]);
                }
            }

        }
        //saveMatToBin(result.estPhase[1], "estPhase[1]");
        cv::patchNaNs(result.estPhase[n], 0.0);

    }


    // 二义性修正
    if (numImages >= 3)
    {
        const cv::Mat& refPhase = result.estPhase[1];
        for (size_t n = 2; n < numImages; ++n)
        {
            const cv::Mat& currentPhase = result.estPhase[n];
            cv::Mat diff = refPhase - currentPhase;
            cv::Mat sum_ = refPhase + currentPhase;
            double ssd_diff = cv::sum(diff.mul(diff))[0];
            double ssd_sum = cv::sum(sum_.mul(sum_))[0];

            if (ssd_diff > ssd_sum)
            {
                result.estKxs[n] *= -1.0;
                result.estKys[n] *= -1.0;
                double val = std::fmod(-result.estDs[n], 2.0 * CV_PI);
                if (val < 0) {
                    val += 2.0 * CV_PI;
                }
                result.estDs[n] = val;
                result.estDeltas[n] = -result.estDeltas[n];
            }
        }

        //end_time = clock();     //获取结束时间
        //double Times = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        //printf("for: %f seconds\n", Times);

    }
    std::cout << "执行完成：NPDA初值估计" << std::endl;



    return result;
}

