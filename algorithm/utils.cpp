#include "utils.h"
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include "uddm_namespace.h"
// 功能函数：
std::vector<cv::Mat> loadFiles(const std::string& filePath, const std::string& type, int nCapture)
{
    std::vector<cv::Mat> images;

    // 构建文件匹配模式
    std::string pattern = filePath + "/" + type;

    // 获取文件列表
    std::vector<cv::String> fileNames;
    cv::glob(pattern, fileNames, false); 
    if (fileNames.empty()) 
    {
        std::cerr << "Error: No files found matching pattern " << pattern << std::endl;
        return images;
    }

    // 根据需要加载指定数量的图像
    int nToLoad = std::min(nCapture, (int)fileNames.size());

    // 预分配内存以提高效率
    images.reserve(nToLoad);

    // 排序文件名以确保按自然顺序加载
    for (int i = 0; i < nToLoad; ++i)
    {
        cv::Mat img = cv::imread(fileNames[i], 0);
        if (!img.empty()) {
            images.push_back(img);
        }
        else {
            std::cerr << "Warning: Could not read image " << fileNames[i] << std::endl;
        }
    }

    return images;
}
cv::Mat createMask(int height, int width) 
{
    cv::Mat mask = cv::Mat::zeros(height, width, CV_32F);
    cv::Point2f center(static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2.0f);
    float radius = std::min(width, height) / 2.0f;
    cv::circle(mask, center, radius, cv::Scalar(1.0), -1); // -1 表示填充圆形
    return mask;
}



/**
 * @brief 创建一个与伪彩图匹配的颜色图例 (Color Bar)
 * @param height 图例的高度（像素）
 * @param minVal 原始数据的最小值
 * @param maxVal 原始数据的最大值
 * @param colormap 要应用的OpenCV色彩映射，例如 cv::COLORMAP_PARULA
 * @return 返回一个包含图例和刻度标签的 cv::Mat 图像
 */
cv::Mat createColorBar(int height, double minVal, double maxVal, int colormap)
{
    int barWidth = 30; 
    int labelWidth = 100; 
    cv::Mat colorBarImage(height, barWidth, CV_8UC1);

    // 1. 创建一个从上到下值从 255 到 0 的灰度梯度条
    for (int i = 0; i < height; ++i) {
        colorBarImage.row(i).setTo(cv::Scalar(255 - (i * 255 / height)));
    }

    // 2. 应用与主图像相同的色彩映射
    cv::applyColorMap(colorBarImage, colorBarImage, colormap);

    // 3. 创建一个更大的画布，用于放置彩色条和标签
    cv::Mat canvas(height, barWidth + labelWidth, CV_8UC3, cv::Scalar(255, 255, 255));

    // 4. 将彩色条绘制到画布的左侧
    colorBarImage.copyTo(canvas(cv::Rect(0, 0, barWidth, height)));

    // 5. 添加刻度和数值标签
    int numTicks = 6;
    for (int i = 0; i < numTicks; ++i) {
        // 计算刻度的位置和对应的数值
        int y = i * (height - 1) / (numTicks - 1);
        double value = maxVal - i * (maxVal - minVal) / (numTicks - 1);

        // 格式化数值为字符串
        std::stringstream ss;
        ss << std::fixed << std::setprecision(5) << value;
        std::string label = ss.str();

        // 绘制刻度线
        cv::line(canvas, cv::Point(barWidth, y), cv::Point(barWidth + 5, y), cv::Scalar(0, 0, 0), 1);

        // 绘制文本标签
        cv::putText(canvas, label, cv::Point(barWidth + 10, y + 5),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }

    return canvas;
}

// /**
//  * @brief pseudoColor
//  * 对单通道cv::Mat做伪颜色处理，色图为cv::COLORMAP_PARULA
//  */
// cv::Mat pseudoColor(const cv::Mat mat_)
// {
//     assert(mat_.channels() == 1);
//     // 转换成浮点型
//     cv::Mat mat;
//     mat_.convertTo(mat, CV_64FC1);
//     // 找最大、最小值并拉伸
//     double min_value, max_value;
//     cv::minMaxLoc(mat, &min_value, &max_value);
//     mat -= min_value;
//     cv::minMaxLoc(mat, &min_value, &max_value);
//     mat *= 1 / max_value;
//     // 转换成CV_8UC1
//     mat.convertTo(mat, CV_8UC1, 255);
//     cv::applyColorMap(mat, mat, cv::COLORMAP_PARULA);

//     return mat;
// };

/**
 * @brief 将单通道Mat转换为伪彩图，并附带一个颜色图例(Color Bar)一起显示。
 * * @param inputMat 要显示的单通道输入矩阵 (例如 CV_32FC1, CV_64FC1, CV_8UC1)。
 * @param windowTitle 显示窗口的标题。
 * @return 返回拼接了Color Bar的最终伪彩图，如果输入无效则返回空Mat。
 */
cv::Mat showPseudoColorWithColorBar(const cv::Mat& inputMat, const std::string& windowTitle)
{
    // --- 1. 输入有效性检查 ---
    if (inputMat.empty() || inputMat.channels() != 1) {
        std::cerr << "Error: Input matrix for pseudo-coloring must be a non-empty single-channel image." << std::endl;
        return cv::Mat(); 
    }

    // --- 2. 找到原始数据的最大最小值 ---
    double minVal, maxVal;
    cv::minMaxLoc(inputMat, &minVal, &maxVal);

    // --- 3. 生成伪彩图 ---
    cv::Mat pseudoColorImage = pseudoColor(inputMat);

    // --- 4. 生成与伪彩图匹配的 Color Bar ---
    cv::Mat colorBar = createColorBar(pseudoColorImage.rows, minVal, maxVal, cv::COLORMAP_PARULA);

    // --- 5. 将伪彩图和Color Bar水平拼接 ---
    cv::Mat combinedResult;
    cv::hconcat(pseudoColorImage, colorBar, combinedResult);

    // --- 6. 显示最终结果 ---
    cv::imshow(windowTitle, combinedResult);

    // --- 7. 返回拼接后的图像，方便后续保存或其他操作 ---
    return combinedResult;
}

// 函数：获取Mat类型的字符串表示
std::string getMatType(const cv::Mat& mat) {
    int type = mat.type();
    std::string r;

    // 解析深度 (每个像素点的数据类型)
    uchar depth = type & CV_MAT_DEPTH_MASK;
    switch (depth) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
    }

    // 解析通道数
    int channels = mat.channels();
    r += "C";
    r += std::to_string(channels);

    return r;
}

/**
 * @brief 自适应显示浮点型Mat，自动将其数值范围缩放到[0.0, 1.0]
 *
 * @param window_name 显示窗口的名称
 * @param mat 待显示的浮点型矩阵 (CV_32F or CV_64F)
 */
void adaptiveImshow(const std::string& window_name, const cv::Mat& mat) {
    // 步骤 1: 输入有效性检查
    if (mat.empty()) {
        std::cerr << "错误: 输入的 Mat 为空。" << std::endl;
        return;
    }

    std::cout << getMatType(mat);

    if (mat.type() != CV_32F && mat.type() != CV_64F) {
        cv::imshow(window_name, mat);
        cv::waitKey(0);
        return;
    }

    // 步骤 2: 将输入矩阵的数值范围归一化到 [0.0, 1.0]
    cv::Mat display_mat;
    cv::normalize(mat, display_mat, 1.0, 0.0, cv::NORM_MINMAX, CV_64F);

    // 步骤 3: 使用 imshow 显示归一化后的图像
    cv::imshow(window_name, display_mat);
    cv::waitKey(0);
}

// --- 将频域矩阵 F 的幅值显示为图像 ---

void cv32fc2Imshow(const std::string& window_name, const cv::Mat& mat) {
    // 1. 计算幅值
    // F 是一个 CV_32FC2 类型的2通道矩阵 (实部, 虚部)
    std::vector <cv::Mat> channels;
    cv::split(mat, channels);
    // 将复数矩阵分离成实部和虚部

    cv::Mat magnitudeImage;
    // 计算幅值: magnitude = sqrt(real^2 + imag^2)
    cv::magnitude(channels[0], channels[1], magnitudeImage);

    // 2. 对数尺度缩放，以增强暗部细节的可视性
    // 加 1 是为了避免 log(0) 的情况
    magnitudeImage += cv::Scalar::all(1);
    cv::log(magnitudeImage, magnitudeImage);

    // 3. 归一化以便显示
    // 将幅值矩阵的数值范围线性缩放到 [0.0, 1.0]
    cv::normalize(magnitudeImage, magnitudeImage,0.0, 1.0, cv::NORM_MINMAX);

    // 4. 显示图像
    cv::imshow(window_name, magnitudeImage);

    // 5. 等待用户按键，否则窗口会一闪而过
    cv::waitKey(0);
}

struct MatHeader {
    int rows;
    int cols;
    int type;
};

// 函数的完整实现
bool saveMatToBin(const cv::Mat& mat, const std::string& filename) {
    if (mat.empty()) {
        std::cerr << "错误: 输入的 Mat 为空。" << std::endl;
        return false;
    }
    if (mat.channels() != 1) {
        std::cerr << "错误: 此函数只支持单通道 Mat。" << std::endl;
        return false;
    }

    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "错误: 无法打开文件进行写入: " << filename << std::endl; 
        return false;
    }

    MatHeader header;
    header.rows = mat.rows;
    header.cols = mat.cols;
    header.type = mat.type();

    ofs.write(reinterpret_cast<const char*>(&header), sizeof(MatHeader));
    if (ofs.fail()) {
        std::cerr << "错误: 写入文件头失败。" << std::endl;
        ofs.close();
        return false;
    }

    if (mat.isContinuous()) {
        ofs.write(reinterpret_cast<const char*>(mat.data), mat.total() * mat.elemSize());
    }
    else {
        for (int i = 0; i < mat.rows; ++i) {
            ofs.write(reinterpret_cast<const char*>(mat.ptr(i)), mat.cols * mat.elemSize());
        }
    }

    if (ofs.fail()) {
        std::cerr << "错误: 写入矩阵数据失败。" << std::endl;
        ofs.close();
        return false;
    }

    ofs.close();
    std::cout << "成功将 Mat 保存到 " << filename << std::endl;
    return true;
}
