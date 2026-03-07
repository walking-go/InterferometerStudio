#include "NPDA.h"
#include <iostream>
#include <numeric>
#include <algorithm>
#include <random>

#include <opencv2/opencv.hpp>

// 为了 `solvePsiCoeffs` 中高效的线性代数运算，引入 Eigen
#include <Eigen/Dense>
#include <chrono>
#include <QDebug>

// 内部辅助函数，实现 solvePsiCoeffs
// 使用 Eigen 进行核心计算
namespace { // Use an anonymous namespace for internal linkage

    Eigen::Vector3d solvePsiCoeffs(
        const Eigen::Vector3d& C0,
        const std::vector<double>& estPhase_roi,
        const std::vector<double>& estA_roi,
        const std::vector<double>& estB_roi,
        const std::vector<float>& I_roi,
        const Eigen::MatrixXd& J1xy_roi,
        double& out_resnorm)
    {
        // LM 算法参数
        const int maxIters = 100;
        double lambda = 0.01;
        const double niu = 10.0;
        const double lambda_max = 1e6;
        const double tolFun = 1e-6;
        const double tolX = 1e-6;

        // MATLAB代码中对像素进行了随机采样，这里我们也实现它
        long totalPixels = estPhase_roi.size();
        long numSamples = 10000;
        if (numSamples > totalPixels) {
            numSamples = totalPixels;
        }

        std::vector<int> idx(totalPixels);
        std::iota(idx.begin(), idx.end(), 0);
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(idx.begin(), idx.end(), g);

        Eigen::VectorXd estA(numSamples);
        Eigen::VectorXd estB(numSamples);
        Eigen::VectorXd estPhase(numSamples);
        Eigen::VectorXd I(numSamples);
        Eigen::MatrixXd J1xy(numSamples, 3);

        for (long i = 0; i < numSamples; ++i) {
            int sample_idx = idx[i];
            estA(i) = estA_roi[sample_idx];
            estB(i) = estB_roi[sample_idx];
            estPhase(i) = estPhase_roi[sample_idx];
            I(i) = I_roi[sample_idx];
            J1xy.row(i) = J1xy_roi.row(sample_idx);
        }

        Eigen::Vector3d C = C0;
        Eigen::VectorXd psi = J1xy * C;
        Eigen::VectorXd r = I.array() - (estA.array() + estB.array() * (estPhase.array() + psi.array()).cos());
        
        for (int iter = 0; iter < maxIters; ++iter) {
            Eigen::VectorXd Q = estB.array() * (estPhase.array() + psi.array()).sin();
            Eigen::MatrixXd J = (Q.asDiagonal() * J1xy).eval();

            Eigen::Matrix3d H = J.transpose() * J;
            Eigen::Vector3d g = J.transpose() * r;

            Eigen::Vector3d dC;
            bool update_found = false;

            int maxInnerIters = 50;
            for (int inner_iter = 0; inner_iter < maxInnerIters; ++inner_iter) {
                Eigen::Matrix3d H_lm = H + lambda * Eigen::Matrix3d::Identity();
                dC = H_lm.ldlt().solve(g);

                Eigen::Vector3d C_new = C - dC;
                Eigen::VectorXd psi_new = J1xy * C_new;
                Eigen::VectorXd r_new = I.array() - (estA.array() + estB.array() * (estPhase.array() + psi_new.array()).cos());

                if (r_new.norm() < r.norm() * (1.0 - 1e-6)) {
                    lambda /= niu;
                    C = C_new;
                    psi = psi_new;
                    r = r_new;
                    update_found = true;
                    break;
                }
                else {
                    lambda *= niu;
                    if (lambda > lambda_max) {
                        break;
                    }
                }
            }

            if (!update_found) {
                break;
            }

            if (r.norm() < tolFun || dC.norm() < tolX) {
                break;
            }
        }

        out_resnorm = r.norm();
        return C;
    }

} 

NPDAResult NPDA(
    const std::vector<cv::Mat>& inputIs,
    const CoeffsMDPD& initCoeffs,
    const cv::Mat& ROI)
{
    qDebug() << "开始执行：NPDA迭代优化";
    auto npdaStartTime = std::chrono::high_resolution_clock::now();

    // 初始化和参数设置
    const int N = static_cast<int>(inputIs.size());
    if (N == 0) throw std::runtime_error("输入图像序列为空!");
    const int NR = inputIs[0].rows;
    const int NC = inputIs[0].cols;

    NPDAResult result;
    result.estA = cv::Mat::zeros(NR, NC, CV_64FC1);
    result.estB = cv::Mat::zeros(NR, NC, CV_64FC1);
    result.estPhase = cv::Mat::zeros(NR, NC, CV_64FC1);

    result.estKxs = initCoeffs.kxs;
    result.estKys = initCoeffs.kys;
    result.estDs = initCoeffs.ds;

    const double epsilon = 1e-3;
    const int maxIters = 5;
    const double omega = 1.5; 

    result.nIters = 0;
    result.historyEK.push_back(std::numeric_limits<double>::infinity());
    result.historyEIt.push_back(std::numeric_limits<double>::infinity());

    // 预计算 J1xy (仅针对ROI区域)
    std::vector<cv::Point> roi_pixels;
    cv::findNonZero(ROI, roi_pixels);
    const long num_roi_pixels = roi_pixels.size();

    // J1xy 使用 Eigen::MatrixXd 以便与 solvePsiCoeffs 无缝衔接
    Eigen::MatrixXd J1xy(num_roi_pixels, 3);
    for (long i = 0; i < num_roi_pixels; ++i) {
        J1xy(i, 0) = 1.0;
        J1xy(i, 1) = static_cast<double>(roi_pixels[i].x);
        J1xy(i, 2) = static_cast<double>(roi_pixels[i].y);
    }

    // 主迭代循环
    while (true) {
        std::vector<double> estKxsLast = result.estKxs;
        std::vector<double> estKysLast = result.estKys;
        std::vector<double> estDsLast = result.estDs;

        Eigen::Map<const Eigen::VectorXd> estKxs_vec(result.estKxs.data(), N);
        Eigen::Map<const Eigen::VectorXd> estKys_vec(result.estKys.data(), N);
        Eigen::Map<const Eigen::VectorXd> estDs_vec(result.estDs.data(), N);

        Eigen::RowVectorXd x_coords = Eigen::RowVectorXd::LinSpaced(NC, 0, NC - 1);
        Eigen::RowVectorXd y_coords = Eigen::RowVectorXd::LinSpaced(NR, 0, NR - 1);

        Eigen::MatrixXd precalX = estKxs_vec * x_coords;
        Eigen::MatrixXd precalYd = estKys_vec * y_coords + estDs_vec.replicate(1, NR);

        cv::parallel_for_(cv::Range(0, NR * NC), [&](const cv::Range& range) {
            for (int i = range.start; i < range.end; ++i) {
                int r = i / NC;
                int c = i % NC;

                if (ROI.at<uchar>(r, c) == 0) continue;

                double sCosD = 0, sSinD = 0, sCossinD = 0, sCos2D = 0, sSin2D = 0;
                double sumI = 0, sumICosD = 0, sumISinD = 0;

                for (int n = 0; n < N; ++n) {
                    double delta_n = precalX(n, c) + precalYd(n, r);
                    double cosD = std::cos(delta_n);
                    double sinD = std::sin(delta_n);

                    sCosD += cosD;
                    sSinD += sinD;
                    sCossinD += cosD * sinD;
                    sCos2D += cosD * cosD;
                    sSin2D += sinD * sinD;

                    float I_nc = inputIs[n].at<float>(r, c);
                    sumI += I_nc;
                    sumICosD += I_nc * cosD;
                    sumISinD += I_nc * sinD;
                }

                cv::Matx33d Q(N, sCosD, sSinD,
                    sCosD, sCos2D, sCossinD,
                    sSinD, sCossinD, sSin2D);
                cv::Vec3d P(sumI, sumICosD, sumISinD);

                cv::Vec3d R;
                cv::solve(Q, P, R, cv::DECOMP_LU);

                result.estA.at<double>(r, c) = R[0];
                double val_b_cos_phi = R[1];
                double val_b_sin_phi = R[2];

                result.estB.at<double>(r, c) = std::sqrt(val_b_cos_phi * val_b_cos_phi + val_b_sin_phi * val_b_sin_phi);
                result.estPhase.at<double>(r, c) = std::atan2(-val_b_sin_phi, val_b_cos_phi);
            }
            });

        std::vector<double> estPhase_roi, estA_roi, estB_roi;
        estPhase_roi.reserve(num_roi_pixels);
        estA_roi.reserve(num_roi_pixels);
        estB_roi.reserve(num_roi_pixels);
        for (const auto& p : roi_pixels) {
            estPhase_roi.push_back(result.estPhase.at<double>(p));
            estA_roi.push_back(result.estA.at<double>(p));
            estB_roi.push_back(result.estB.at<double>(p));
        }

        double EIt = 0.0;
        for (int n = 1; n < N; ++n) { 
            Eigen::Vector3d C0;
            C0 << result.estDs[n], result.estKxs[n], result.estKys[n];

            std::vector<float> I_n_roi;
            I_n_roi.reserve(num_roi_pixels);
            for (const auto& p : roi_pixels) {
                I_n_roi.push_back(inputIs[n].at<float>(p));
            }

            double resnorm;
            Eigen::Vector3d C = solvePsiCoeffs(C0, estPhase_roi, estA_roi, estB_roi, I_n_roi, J1xy, resnorm);

            double deltaD = C(0) - result.estDs[n];
            double deltaKx = C(1) - result.estKxs[n];
            double deltaKy = C(2) - result.estKys[n];

            // 使用松弛因子更新
            result.estDs[n] += omega * deltaD;
            result.estKxs[n] += omega * deltaKx;
            result.estKys[n] += omega * deltaKy;

            EIt += resnorm;
        }

        //  检查收敛
        result.nIters++;
        double EK = 0.0;
        for (int i = 0; i < N; ++i) {
            EK += std::pow(estKxsLast[i] - result.estKxs[i], 2);
            EK += std::pow(estKysLast[i] - result.estKys[i], 2);
            EK += std::pow(estDsLast[i] - result.estDs[i], 2);
        }
        EK = std::sqrt(EK);

        if (result.nIters >= maxIters ||
            (std::abs(result.historyEK.back() - EK) / EK < epsilon) ||
            (std::abs(result.historyEIt.back() - EIt) / EIt < epsilon))
        {
            break;
        }

        result.historyEK.push_back(EK);
        result.historyEIt.push_back(EIt);
    }

    auto npdaEndTime = std::chrono::high_resolution_clock::now();
    double npdaElapsed = std::chrono::duration<double>(npdaEndTime - npdaStartTime).count();
    qDebug() << QString("执行完成：NPDA迭代优化，迭代次数: %1，耗时 %2 秒").arg(result.nIters).arg(npdaElapsed, 0, 'f', 3);
    return result;
}


