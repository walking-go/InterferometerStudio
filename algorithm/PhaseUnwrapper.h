#ifndef PHASEUNWRAPPER_H
#define PHASEUNWRAPPER_H

#include <opencv2/opencv.hpp>
#include <vector>
struct Edge {
    double reliability;
    int pixel1_idx;
    int pixel2_idx;

    bool operator>(const Edge& other) const {
        return reliability > other.reliability;
    }
};


class PhaseUnwrapper {
public:
    PhaseUnwrapper() {}
   cv::Mat unwrapPathMask(const cv::Mat& wrap, const cv::Mat& mask);

private:
    cv::Mat unwrapPhaseOptimized(cv::Mat& img);
    cv::Mat getReliability(const cv::Mat& img);
    void getEdges(const cv::Mat& reliability, std::vector<Edge>& edges);
};

#endif // PHASEUNWRAPPER_H