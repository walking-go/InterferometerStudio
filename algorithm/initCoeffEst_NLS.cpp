#include "initCoeffEst_NLS.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <omp.h>
#include <QDebug>

// ============================================================
// Constants
// ============================================================
static constexpr int    GRID_SIZE       = 31;
static constexpr int    DOWNSAMPLE_RATE = 4;
static constexpr double VALID_THRESH    = 0.05;
static constexpr double RELIABLE_THRESH = 0.17;
static constexpr double OUTLIER_THRESH  = 10.0;

// ============================================================
// Internal data structures
// ============================================================
struct FrameEstResult {
    double kx;
    double ky;
    double d;
    double resErr;
};

struct GridSearchResult {
    double bestKx;
    double bestKy;
    double bestD;
    double bestResidual;
};

// ============================================================
// computeCosDelta — Eq.9 with 6-direction neighbor differencing
// ============================================================
static void computeCosDelta(
    const cv::Mat& I1,
    const cv::Mat& I2,
    const cv::Mat& ROI,         // (H-2)x(W-2) mask, CV_8U
    double validThresh,
    cv::Mat& estCosDelta,       // output: (H-2)x(W-2), CV_32F
    cv::Mat& validMask)         // output: (H-2)x(W-2), CV_8U
{
    const int H = I1.rows;
    const int W = I1.cols;
    const int innerH = H - 2;
    const int innerW = W - 2;

    // P = I1^2, Q = I2^2, R = I1*I2
    cv::Mat P, Q, R;
    cv::pow(I1, 2, P);
    cv::pow(I2, 2, Q);
    R = I1.mul(I2);

    // Neighbor region rects
    cv::Rect center_rect(1, 1, innerW, innerH);
    cv::Rect lu_rect(0, 0, innerW, innerH);
    cv::Rect u_rect(1, 0, innerW, innerH);
    cv::Rect l_rect(0, 1, innerW, innerH);
    cv::Rect r_rect(2, 1, innerW, innerH);
    cv::Rect b_rect(1, 2, innerW, innerH);
    cv::Rect rb_rect(2, 2, innerW, innerH);

    cv::Mat Pc = P(center_rect), Qc = Q(center_rect), Rc = R(center_rect);

    // 6 direction diffs
    std::vector<cv::Mat> PD(6), QD(6), RD(6);
    PD[0] = P(r_rect)  - Pc;        QD[0] = Q(r_rect)  - Qc;        RD[0] = R(r_rect)  - Rc;
    PD[1] = P(r_rect)  - P(l_rect); QD[1] = Q(r_rect)  - Q(l_rect); RD[1] = R(r_rect)  - R(l_rect);
    PD[2] = P(b_rect)  - Pc;        QD[2] = Q(b_rect)  - Qc;        RD[2] = R(b_rect)  - Rc;
    PD[3] = P(b_rect)  - P(u_rect); QD[3] = Q(b_rect)  - Q(u_rect); RD[3] = R(b_rect)  - R(u_rect);
    PD[4] = P(rb_rect) - Pc;        QD[4] = Q(rb_rect) - Qc;        RD[4] = R(rb_rect) - Rc;
    PD[5] = P(rb_rect) - P(lu_rect);QD[5] = Q(rb_rect) - Q(lu_rect);RD[5] = R(rb_rect) - R(lu_rect);

    // Select best direction per pixel (max |RDiff|)
    cv::Mat PDiff = cv::Mat::zeros(innerH, innerW, CV_32F);
    cv::Mat QDiff = cv::Mat::zeros(innerH, innerW, CV_32F);
    cv::Mat RDiff = cv::Mat::zeros(innerH, innerW, CV_32F);

    for (int row = 0; row < innerH; ++row) {
        for (int col = 0; col < innerW; ++col) {
            float maxVal = -1.0f;
            int maxIdx = 0;
            for (int d = 0; d < 6; ++d) {
                float val = std::abs(RD[d].at<float>(row, col));
                if (val > maxVal) {
                    maxVal = val;
                    maxIdx = d;
                }
            }
            PDiff.at<float>(row, col) = PD[maxIdx].at<float>(row, col);
            QDiff.at<float>(row, col) = QD[maxIdx].at<float>(row, col);
            RDiff.at<float>(row, col) = RD[maxIdx].at<float>(row, col);
        }
    }

    // estCosDelta = (PDiff + QDiff) / (2 * RDiff), clamped to [-1, 1]
    estCosDelta = (PDiff + QDiff) / (2.0f * RDiff);
    cv::patchNaNs(estCosDelta, 0.0);

    // Apply ROI mask
    cv::Mat roiInv;
    cv::compare(ROI, 0, roiInv, cv::CMP_EQ);
    estCosDelta.setTo(0, roiInv);

    cv::min(estCosDelta, 1.0f, estCosDelta);
    cv::max(estCosDelta, -1.0f, estCosDelta);

    // Valid mask: |RDiff| > validThresh (within ROI)
    validMask = cv::Mat::zeros(innerH, innerW, CV_8U);
    for (int row = 0; row < innerH; ++row) {
        for (int col = 0; col < innerW; ++col) {
            if (ROI.at<uchar>(row, col) != 0 &&
                std::abs(RDiff.at<float>(row, col)) > static_cast<float>(validThresh)) {
                validMask.at<uchar>(row, col) = 255;
            }
        }
    }
}

// ============================================================
// gridSearchWithLinearD — Separable decomposition + Cramer's rule
// ============================================================
static GridSearchResult gridSearchWithLinearD(
    const std::vector<double>& kxRange,
    const std::vector<double>& kyRange,
    const std::vector<double>& xValid,
    const std::vector<double>& yValid,
    const std::vector<double>& alphaValid)
{
    const int nKx = static_cast<int>(kxRange.size());
    const int nKy = static_cast<int>(kyRange.size());
    const int N = static_cast<int>(xValid.size());

    // Precompute separable cos/sin tables
    // cosKx[i][p] = cos(kxRange[i] * xValid[p])
    std::vector<std::vector<double>> cosKx(nKx, std::vector<double>(N));
    std::vector<std::vector<double>> sinKx(nKx, std::vector<double>(N));
    std::vector<std::vector<double>> cosKy(nKy, std::vector<double>(N));
    std::vector<std::vector<double>> sinKy(nKy, std::vector<double>(N));

    for (int i = 0; i < nKx; ++i) {
        for (int p = 0; p < N; ++p) {
            double angle = kxRange[i] * xValid[p];
            cosKx[i][p] = std::cos(angle);
            sinKx[i][p] = std::sin(angle);
        }
    }
    for (int j = 0; j < nKy; ++j) {
        for (int p = 0; p < N; ++p) {
            double angle = kyRange[j] * yValid[p];
            cosKy[j][p] = std::cos(angle);
            sinKy[j][p] = std::sin(angle);
        }
    }

    double globalBestResidual = std::numeric_limits<double>::max();
    double globalBestKx = 0, globalBestKy = 0, globalBestD = 0;

    // Grid search using separable combination (no trig in inner loop)
    for (int i = 0; i < nKx; ++i) {
        for (int j = 0; j < nKy; ++j) {
            // Accumulate 6 sums for Cramer's rule (2x2 linear LS)
            // alpha = cos(q)*cos(d) - sin(q)*sin(d)
            // Design matrix columns: [cos(q), -sin(q)]
            // Normal equations: [Scc, -Scs; -Scs, Sss] [cosD; sinD] = [Sca; -Ssa]
            double Scc = 0, Sss = 0, Scs = 0;
            double Sca = 0, Ssa = 0, Saa = 0;

            for (int p = 0; p < N; ++p) {
                // cos(kx*x + ky*y) = cos(kx*x)*cos(ky*y) - sin(kx*x)*sin(ky*y)
                double cosQ = cosKx[i][p] * cosKy[j][p] - sinKx[i][p] * sinKy[j][p];
                // sin(kx*x + ky*y) = sin(kx*x)*cos(ky*y) + cos(kx*x)*sin(ky*y)
                double sinQ = sinKx[i][p] * cosKy[j][p] + cosKx[i][p] * sinKy[j][p];
                double a = alphaValid[p];

                Scc += cosQ * cosQ;
                Sss += sinQ * sinQ;
                Scs += cosQ * sinQ;
                Sca += cosQ * a;
                Ssa += sinQ * a;
                Saa += a * a;
            }

            // Cramer's rule for normal equations:
            // [Scc, -Scs] [cosD]   [Sca]
            // [-Scs, Sss] [sinD] = [-Ssa]
            double det = Scc * Sss - Scs * Scs;
            if (std::abs(det) < 1e-15) continue;

            double cosD = (Sca * Sss - Ssa * Scs) / det;
            double sinD = (Scs * Sca - Scc * Ssa) / det;

            double d = std::atan2(sinD, cosD);

            // sum((alpha - predicted)^2) where predicted = cosQ*cosD - sinQ*sinD
            double residual = Saa - 2.0 * cosD * Sca + 2.0 * sinD * Ssa
                            + cosD * cosD * Scc + sinD * sinD * Sss
                            - 2.0 * cosD * sinD * Scs;
            residual /= N;

            if (residual < globalBestResidual) {
                globalBestResidual = residual;
                globalBestKx = kxRange[i];
                globalBestKy = kyRange[j];
                globalBestD = d;
            }
        }
    }

    return {globalBestKx, globalBestKy, globalBestD, globalBestResidual};
}

// ============================================================
// localRefine — cv::DownhillSolver (Nelder-Mead)
// ============================================================
class NLSObjective : public cv::MinProblemSolver::Function {
public:
    NLSObjective(const std::vector<double>& xValid,
                 const std::vector<double>& yValid,
                 const std::vector<double>& alphaValid)
        : m_xValid(xValid), m_yValid(yValid), m_alphaValid(alphaValid)
        , m_N(static_cast<int>(xValid.size())) {}

    int getDims() const override { return 2; }

    double calc(const double* x) const override {
        double kx = x[0];
        double ky = x[1];

        double Scc = 0, Sss = 0, Scs = 0;
        double Sca = 0, Ssa = 0, Saa = 0;

        for (int p = 0; p < m_N; ++p) {
            double q = kx * m_xValid[p] + ky * m_yValid[p];
            double cosQ = std::cos(q);
            double sinQ = std::sin(q);
            double a = m_alphaValid[p];

            Scc += cosQ * cosQ;
            Sss += sinQ * sinQ;
            Scs += cosQ * sinQ;
            Sca += cosQ * a;
            Ssa += sinQ * a;
            Saa += a * a;
        }

        double det = Scc * Sss - Scs * Scs;
        if (std::abs(det) < 1e-15) return 1e10;

        double cosD = (Sca * Sss - Ssa * Scs) / det;
        double sinD = (Scs * Sca - Scc * Ssa) / det;

        double residual = Saa - 2.0 * cosD * Sca + 2.0 * sinD * Ssa
                        + cosD * cosD * Scc + sinD * sinD * Sss
                        - 2.0 * cosD * sinD * Scs;
        return residual / m_N;
    }

private:
    const std::vector<double>& m_xValid;
    const std::vector<double>& m_yValid;
    const std::vector<double>& m_alphaValid;
    int m_N;
};

static void localRefine(
    double& bestKx, double& bestKy, double& bestD, double& bestResidual,
    const std::vector<double>& kxRange,
    const std::vector<double>& kyRange,
    const std::vector<double>& xValid,
    const std::vector<double>& yValid,
    const std::vector<double>& alphaValid)
{
    // Step sizes based on grid spacing
    double stepKx = (kxRange.size() > 1) ? (kxRange[1] - kxRange[0]) : 0.01;
    double stepKy = (kyRange.size() > 1) ? (kyRange[1] - kyRange[0]) : 0.01;

    auto objective = cv::makePtr<NLSObjective>(xValid, yValid, alphaValid);

    cv::Mat initStep = (cv::Mat_<double>(1, 2) << stepKx, stepKy);
    cv::TermCriteria termCrit(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 200, 1e-10);

    auto solver = cv::DownhillSolver::create(objective, initStep, termCrit);

    cv::Mat x = (cv::Mat_<double>(1, 2) << bestKx, bestKy);
    double minVal = solver->minimize(x);

    // Only adopt if better (matching MATLAB behavior)
    if (minVal < bestResidual) {
        double refinedKx = x.at<double>(0, 0);
        double refinedKy = x.at<double>(0, 1);

        // Recompute d for the refined (kx, ky) via Cramer's rule
        int N = static_cast<int>(xValid.size());
        double Scc = 0, Sss = 0, Scs = 0, Sca = 0, Ssa = 0;
        for (int p = 0; p < N; ++p) {
            double q = refinedKx * xValid[p] + refinedKy * yValid[p];
            double cosQ = std::cos(q);
            double sinQ = std::sin(q);
            double a = alphaValid[p];
            Scc += cosQ * cosQ;
            Sss += sinQ * sinQ;
            Scs += cosQ * sinQ;
            Sca += cosQ * a;
            Ssa += sinQ * a;
        }
        double det = Scc * Sss - Scs * Scs;
        if (std::abs(det) > 1e-15) {
            double cosD = (Sca * Sss - Ssa * Scs) / det;
            double sinD = (Scs * Sca - Scc * Ssa) / det;
            bestD = std::atan2(sinD, cosD);
        }

        bestKx = refinedKx;
        bestKy = refinedKy;
        bestResidual = minVal;
    }
}

// ============================================================
// resolveSignAmbiguity — Phase consistency check
// ============================================================
static void resolveSignAmbiguity(
    const std::vector<cv::Mat>& normalizedIs,
    std::vector<double>& estKxs,
    std::vector<double>& estKys,
    std::vector<double>& estDs)
{
    const int N = static_cast<int>(normalizedIs.size());
    if (N < 3) return;

    const int NR = normalizedIs[0].rows;
    const int NC = normalizedIs[0].cols;

    const cv::Mat& I1 = normalizedIs[0];

    // Create coordinate grids (1-based, matching MATLAB meshgrid(1:NC, 1:NR))
    cv::Mat xGrid(NR, NC, CV_32F);
    cv::Mat yGrid(NR, NC, CV_32F);
    for (int r = 0; r < NR; ++r) {
        for (int c = 0; c < NC; ++c) {
            xGrid.at<float>(r, c) = static_cast<float>(c + 1);
            yGrid.at<float>(r, c) = static_cast<float>(r + 1);
        }
    }

    // Compute estPhase for each frame
    std::vector<cv::Mat> estPhase(N);
    for (int n = 1; n < N; ++n) {
        cv::Mat estDelta(NR, NC, CV_32F);
        float kx = static_cast<float>(estKxs[n]);
        float ky = static_cast<float>(estKys[n]);
        float d  = static_cast<float>(estDs[n]);

        for (int r = 0; r < NR; ++r) {
            for (int c = 0; c < NC; ++c) {
                estDelta.at<float>(r, c) = kx * xGrid.at<float>(r, c)
                                         + ky * yGrid.at<float>(r, c) + d;
            }
        }

        estPhase[n] = cv::Mat(NR, NC, CV_32F);
        for (int r = 0; r < NR; ++r) {
            for (int c = 0; c < NC; ++c) {
                float delta_val = estDelta.at<float>(r, c);
                float sinD_val = std::sin(delta_val);
                float signSinD = (sinD_val > 0) ? 1.0f : ((sinD_val < 0) ? -1.0f : 0.0f);

                float i1 = I1.at<float>(r, c);
                float in = normalizedIs[n].at<float>(r, c);

                float y_arg = (i1 * std::cos(delta_val) - in) * signSinD;
                float x_arg = (i1 * std::sin(delta_val)) * signSinD;
                estPhase[n].at<float>(r, c) = std::atan2(y_arg, x_arg);
            }
        }
        cv::patchNaNs(estPhase[n], 0.0);
    }

    // Compare phase[2..N-1] against phase[1] (the reference)
    const cv::Mat& refPhase = estPhase[1];
    for (int n = 2; n < N; ++n) {
        const cv::Mat& curPhase = estPhase[n];
        cv::Mat diffMat = refPhase - curPhase;
        cv::Mat sumMat  = refPhase + curPhase;

        double ssdDiff = cv::sum(diffMat.mul(diffMat))[0];
        double ssdSum  = cv::sum(sumMat.mul(sumMat))[0];

        if (ssdDiff > ssdSum) {
            estKxs[n] = -estKxs[n];
            estKys[n] = -estKys[n];
            double val = std::fmod(-estDs[n], 2.0 * CV_PI);
            if (val < 0) val += 2.0 * CV_PI;
            estDs[n] = val;
        }
    }
}

// ============================================================
// outlierDetection — Median-based outlier flagging
// ============================================================
static void outlierDetection(
    const std::vector<double>& estKxs,
    const std::vector<double>& estKys,
    std::vector<bool>& reliable)
{
    const int N = static_cast<int>(estKxs.size());

    // Collect abs(kx) and abs(ky) for reliable frames (starting from index 1)
    std::vector<double> absKxReliable, absKyReliable;
    for (int n = 1; n < N; ++n) {
        if (reliable[n]) {
            absKxReliable.push_back(std::abs(estKxs[n]));
            absKyReliable.push_back(std::abs(estKys[n]));
        }
    }

    if (absKxReliable.size() < 3) return;

    // Compute medians
    auto computeMedian = [](std::vector<double>& v) -> double {
        size_t n = v.size();
        std::nth_element(v.begin(), v.begin() + n / 2, v.end());
        if (n % 2 == 0) {
            double mid2 = v[n / 2];
            std::nth_element(v.begin(), v.begin() + n / 2 - 1, v.end());
            return (v[n / 2 - 1] + mid2) / 2.0;
        }
        return v[n / 2];
    };

    double kxMedian = computeMedian(absKxReliable);
    double kyMedian = computeMedian(absKyReliable);
    kxMedian = std::max(kxMedian, 1e-6);
    kyMedian = std::max(kyMedian, 1e-6);

    for (int n = 1; n < N; ++n) {
        if (!reliable[n]) continue;
        bool isKxOutlier = std::abs(estKxs[n]) > OUTLIER_THRESH * kxMedian;
        bool isKyOutlier = std::abs(estKys[n]) > OUTLIER_THRESH * kyMedian;
        if (isKxOutlier || isKyOutlier) {
            reliable[n] = false;
        }
    }
}

// ============================================================
// Main function: initCoeffEst_NLS
// ============================================================
DeltaEstimationResult initCoeffEst_NLS(
    const std::vector<cv::Mat>& normalizedIs,
    double maxTiltFactor,
    const cv::Mat& mask)
{
    qDebug() << "开始执行：NLS初值估计";
    auto nlsStartTime = std::chrono::high_resolution_clock::now();

    const int N = static_cast<int>(normalizedIs.size());
    if (N == 0) {
        throw std::invalid_argument("输入图像序列不能为空");
    }

    const int H = normalizedIs[0].rows;
    const int W = normalizedIs[0].cols;
    const int innerH = H - 2;
    const int innerW = W - 2;

    // Prepare ROI: crop to inner region (matching MATLAB: mask(2:end-1, 2:end-1))
    cv::Mat ROI;
    cv::Rect innerRect(1, 1, innerW, innerH);
    cv::Mat roiCropped = mask(innerRect);

    if (roiCropped.type() != CV_8U) {
        roiCropped.convertTo(ROI, CV_8U, 255.0);
    } else {
        ROI = roiCropped.clone();
    }
    cv::compare(ROI, 0, ROI, cv::CMP_NE);

    // Erode ROI to avoid boundary effects (disk SE radius 3, approximate with ellipse)
    cv::Mat se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
    cv::erode(ROI, ROI, se);

    // Compute search range
    double maxDisturbTiltPV = maxTiltFactor * 2.0 * CV_PI;
    double maxKx = maxDisturbTiltPV / W;
    double maxKy = maxDisturbTiltPV / H;

    std::vector<double> kxRange(GRID_SIZE), kyRange(GRID_SIZE);
    for (int i = 0; i < GRID_SIZE; ++i) {
        kxRange[i] = -maxKx + 2.0 * maxKx * i / (GRID_SIZE - 1);
        kyRange[i] = -maxKy + 2.0 * maxKy * i / (GRID_SIZE - 1);
    }

    // Initialize result
    DeltaEstimationResult result;
    result.estKxs.resize(N, 0.0);
    result.estKys.resize(N, 0.0);
    result.estDs.resize(N, 0.0);
    result.resErrs.resize(N, 0.0);
    result.reliable.resize(N, true);

    // Store per-frame results for thread safety
    std::vector<FrameEstResult> frameResults(N);
    std::vector<bool> frameValid(N, true);

    const cv::Mat& I1 = normalizedIs[0];

    // Coordinate grids for inner region
    // MATLAB: [X, Y] = meshgrid(2:W-1, 2:H-1)
    // In 0-based C++: x = 1..W-2, y = 1..H-2 (which map to MATLAB's 2..W-1, 2..H-1)
    // But MATLAB uses 1-based, so meshgrid(2:W-1, 2:H-1) gives x in [2, W-1], y in [2, H-1]
    // In our 0-based system, inner pixel at (row, col) in the inner image
    // corresponds to (row+1, col+1) in the full image (0-based), which is (row+2, col+2) in MATLAB 1-based
    // MATLAB code: X(idxDs) and Y(idxDs) where [X,Y] = meshgrid(2:W-1, 2:H-1)
    // So the pixel coordinates used in fitting are 1-based: x in [2..W-1], y in [2..H-1]

    // Process each frame (OpenMP parallel)
    const int loopN = N;
#pragma omp parallel for schedule(dynamic)
    for (int n = 1; n < loopN; ++n) {
        const cv::Mat& I2 = normalizedIs[n];

        // Step 1: Compute cos(psi_n) using Eq.9
        cv::Mat estCosDelta, validMask;
        computeCosDelta(I1, I2, ROI, VALID_THRESH, estCosDelta, validMask);

        // Step 2: Extract valid pixels with downsampling
        // Combined mask = ROI & validMask
        cv::Mat combinedIdx;
        cv::bitwise_and(ROI, validMask, combinedIdx);

        // Apply downsampling
        cv::Mat dsIdx = cv::Mat::zeros(innerH, innerW, CV_8U);
        for (int r = 0; r < innerH; r += DOWNSAMPLE_RATE) {
            for (int c = 0; c < innerW; c += DOWNSAMPLE_RATE) {
                dsIdx.at<uchar>(r, c) = 255;
            }
        }
        cv::Mat finalIdx;
        cv::bitwise_and(combinedIdx, dsIdx, finalIdx);

        // Collect valid pixel coordinates and values
        std::vector<double> xValid, yValid, alphaValid;
        for (int r = 0; r < innerH; ++r) {
            for (int c = 0; c < innerW; ++c) {
                if (finalIdx.at<uchar>(r, c) != 0) {
                    // 1-based coordinates matching MATLAB's meshgrid(2:W-1, 2:H-1)
                    xValid.push_back(static_cast<double>(c + 2));  // col+1 (0-based in full) + 1 (to 1-based) = c+2
                    yValid.push_back(static_cast<double>(r + 2));  // row+1 (0-based in full) + 1 (to 1-based) = r+2
                    alphaValid.push_back(static_cast<double>(estCosDelta.at<float>(r, c)));
                }
            }
        }

        int numPoints = static_cast<int>(xValid.size());
        if (numPoints < 10) {
            frameResults[n] = {0.0, 0.0, 0.0, std::numeric_limits<double>::infinity()};
            frameValid[n] = false;
            continue;
        }

        // Step 3: Grid search
        GridSearchResult gridResult = gridSearchWithLinearD(kxRange, kyRange, xValid, yValid, alphaValid);

        double bestKx = gridResult.bestKx;
        double bestKy = gridResult.bestKy;
        double bestD  = gridResult.bestD;
        double bestResidual = gridResult.bestResidual;

        // Step 4: Local refinement
        localRefine(bestKx, bestKy, bestD, bestResidual, kxRange, kyRange, xValid, yValid, alphaValid);

        // Store result with d in [0, 2*pi)
        double dMod = std::fmod(bestD, 2.0 * CV_PI);
        if (dMod < 0) dMod += 2.0 * CV_PI;

        frameResults[n] = {bestKx, bestKy, dMod, bestResidual};
    }

    // Collect results
    for (int n = 1; n < N; ++n) {
        result.estKxs[n] = frameResults[n].kx;
        result.estKys[n] = frameResults[n].ky;
        result.estDs[n]  = frameResults[n].d;
        result.resErrs[n] = frameResults[n].resErr;
        if (!frameValid[n]) {
            result.reliable[n] = false;
        }
    }

    // Sign ambiguity resolution (serial)
    resolveSignAmbiguity(normalizedIs, result.estKxs, result.estKys, result.estDs);

    // Reliability check: residual threshold
    for (int n = 1; n < N; ++n) {
        if (result.resErrs[n] >= RELIABLE_THRESH) {
            result.reliable[n] = false;
        }
    }

    // Outlier detection (serial)
    outlierDetection(result.estKxs, result.estKys, result.reliable);

    auto nlsEndTime = std::chrono::high_resolution_clock::now();
    double nlsElapsed = std::chrono::duration<double>(nlsEndTime - nlsStartTime).count();
    qDebug() << QString("执行完成：NLS初值估计，耗时 %1 秒").arg(nlsElapsed, 0, 'f', 3);
    return result;
}
