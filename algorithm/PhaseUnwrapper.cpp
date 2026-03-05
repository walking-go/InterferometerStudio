#include "PhaseUnwrapper.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <numeric> 
#include <limits> 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

cv::Mat PhaseUnwrapper::unwrapPathMask(const cv::Mat& wrap, const cv::Mat& mask) {
    CV_Assert(wrap.type() == CV_64FC1);
    CV_Assert(mask.type() == CV_8UC1);
    CV_Assert(wrap.size() == mask.size());
    cv::Mat I_mask = wrap.clone();
    const double nan_val = std::numeric_limits<double>::quiet_NaN();
    for (int r = 0; r < mask.rows; ++r) {
        for (int c = 0; c < mask.cols; ++c) {
            if (mask.at<uchar>(r, c) == 0) {
                I_mask.at<double>(r, c) = nan_val;
            }
        }
    }
    cv::Mat unwrapped = unwrapPhaseOptimized(I_mask);
    cv::Mat is_nan_mask = (unwrapped != unwrapped); 
    unwrapped.setTo(0.0, is_nan_mask);

    return unwrapped;
}

cv::Mat PhaseUnwrapper::unwrapPhaseOptimized(cv::Mat& img) {
    const int Ny = img.rows;
    const int Nx = img.cols;
    const int num_pixels = Ny * Nx;

    //  计算可靠性图
    cv::Mat reliability = getReliability(img);

    std::vector<Edge> edges;
    edges.reserve(2 * num_pixels);
    getEdges(reliability, edges);

    // 对边按可靠性降序排序
    std::sort(edges.begin(), edges.end(), std::greater<Edge>());
    std::vector<int> group(num_pixels);
    std::iota(group.begin(), group.end(), 0); 
    std::vector<int> is_grouped(num_pixels, 0); 
    std::vector<std::vector<int>> group_members(num_pixels);
    for (int i = 0; i < num_pixels; ++i) {
        group_members[i].push_back(i);
    }

    std::vector<int> num_members_group(num_pixels, 1);
    cv::Mat res_img = img; 
    double* res_ptr = res_img.ptr<double>();

    for (const auto& edge : edges) {
        if (edge.reliability < 0) {
            continue;
        }
        int idx1 = edge.pixel1_idx;
        int idx2 = edge.pixel2_idx;
        if (group[idx1] == group[idx2]) continue;
        bool all_grouped = false; 

        if (is_grouped[idx1]) {
            if (!is_grouped[idx2]) {
                std::swap(idx1, idx2);
            }
            else if (num_members_group[group[idx1]] > num_members_group[group[idx2]]) {
                std::swap(idx1, idx2);
                all_grouped = true;
            }
            else {
                all_grouped = true;
            }
        }
        double diff = res_ptr[idx2] - res_ptr[idx1];
        double dval = std::floor((diff + M_PI) / (2 * M_PI)) * 2 * M_PI;
        int g1 = group[idx1];
        int g2 = group[idx2];

        std::vector<int> pixels_to_change;

        if (all_grouped) {
            pixels_to_change = group_members[g1]; 
        }
        else {
            pixels_to_change.push_back(idx1); 
        }
        if (std::abs(dval) > 1e-9) { 
            for (int pix_idx : pixels_to_change) {
                res_ptr[pix_idx] += dval; 
            }
        }

        int len_g1 = pixels_to_change.size();
        group_members[g2].insert(group_members[g2].end(), pixels_to_change.begin(), pixels_to_change.end());

        for (int pix_idx : pixels_to_change) {
            group[pix_idx] = g2;
        }
        num_members_group[g2] += len_g1;

        if (all_grouped) {
            group_members[g1].clear();
            num_members_group[g1] = 0;
        }

        is_grouped[idx1] = 1;
        is_grouped[idx2] = 1;
    }

    return res_img;
}

// 可靠性计算实现
cv::Mat PhaseUnwrapper::getReliability(const cv::Mat& img) {
    cv::Mat rel = cv::Mat::zeros(img.size(), CV_64FC1);
    const double nan_val = std::numeric_limits<double>::quiet_NaN();
    const double pi = M_PI;

    // 定义gamma匿名函数
    auto gamma = [&](double x) {
        if (std::isnan(x)) return nan_val;
        double sign = (x > 0.0) ? 1.0 : ((x < 0.0) ? -1.0 : 0.0);
        return sign * std::fmod(std::abs(x), pi);
        };

    // 遍历所有内部像素计算二阶导数
    for (int r = 1; r < img.rows - 1; ++r) {
        for (int c = 1; c < img.cols - 1; ++c) {
            const double* p_prev = img.ptr<double>(r - 1);
            const double* p_curr = img.ptr<double>(r);
            const double* p_next = img.ptr<double>(r + 1);

            double center = p_curr[c];
            if (std::isnan(center)) {
                rel.at<double>(r, c) = nan_val;
                continue;
            }
            double im1_jm1 = p_prev[c - 1], i_jm1 = p_curr[c - 1], ip1_jm1 = p_next[c - 1]; 
            double im1_j = p_prev[c], ip1_j = p_next[c];           
            double im1_jp1 = p_prev[c + 1], i_jp1 = p_curr[c + 1], ip1_jp1 = p_next[c + 1];

            im1_jm1 = p_prev[c - 1];
            i_jm1 = p_curr[c - 1]; 
            ip1_jm1 = p_next[c - 1];
            im1_j = p_prev[c];         
            ip1_j = p_next[c];
            im1_jp1 = p_prev[c + 1]; 
            i_jp1 = p_curr[c + 1]; 
            ip1_jp1 = p_next[c + 1];


            double H = gamma(im1_j - center) - gamma(center - ip1_j);
            double V = gamma(i_jm1 - center) - gamma(center - i_jp1);
            double D1 = gamma(im1_jm1 - center) - gamma(center - ip1_jp1);
            double D2 = gamma(im1_jp1 - center) - gamma(center - ip1_jm1);

            double D_sq = H * H + V * V + D1 * D1 + D2 * D2;
            if (D_sq < 1e-12 || std::isnan(D_sq)) {
                rel.at<double>(r, c) = 0.0;
            }
            else {
                rel.at<double>(r, c) = 1.0 / std::sqrt(D_sq);
            }
        }
    }
    // 将img中的NaN位置也设置到rel中
    for (int r = 0; r < img.rows; ++r) {
        for (int c = 0; c < img.cols; ++c) {
            if (std::isnan(img.at<double>(r, c))) {
                rel.at<double>(r, c) = nan_val;
            }
            else if (r == 0 || r == img.rows - 1 || c == 0 || c == img.cols - 1) {
                if (std::isnan(rel.at<double>(r, c))) {
                    rel.at<double>(r, c) = 0.0;
                }
            }
        }
    }
    return rel;
}


// 边的收集与计算实现 (与之前相同)
void PhaseUnwrapper::getEdges(const cv::Mat& reliability, std::vector<Edge>& edges) {
    const int Ny = reliability.rows;
    const int Nx = reliability.cols;
    const double nan_val = std::numeric_limits<double>::quiet_NaN();

    for (int r = 0; r < Ny; ++r) {
        const double* rel_row = reliability.ptr<double>(r);
        const double* rel_row_next = (r < Ny - 1) ? reliability.ptr<double>(r + 1) : nullptr;

        for (int c = 0; c < Nx; ++c) {
            double rel1 = rel_row[c];
            if (std::isnan(rel1)) continue;
            if (c < Nx - 1) {
                double rel2 = rel_row[c + 1];
                if (!std::isnan(rel2)) {
                    edges.push_back({ rel1 + rel2, r * Nx + c, r * Nx + c + 1 });
                }
            }

            // 垂直边 (Vertical edge)
            if (r < Ny - 1) {
                double rel2 = rel_row_next[c];
                if (!std::isnan(rel2)) {
                    edges.push_back({ rel1 + rel2, r * Nx + c, (r + 1) * Nx + c });
                }
            }
        }
    }
}