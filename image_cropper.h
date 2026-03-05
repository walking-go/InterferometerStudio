#ifndef IMAGE_CROPPER_H
#define IMAGE_CROPPER_H

#include <QImage>
#include <QRect>
#include <QSize>
#include <opencv2/opencv.hpp>

/**
 * @brief ImageCropper 图像裁剪器
 *
 * 用于去除相机图像中的黑色边缘，提取中间的干涉图核心区域。
 *
 * 算法原理：
 * 1. 将图像转为灰度图
 * 2. 使用阈值分割找出非黑色区域
 * 3. 查找最大连通区域的外接矩形
 * 4. 按该矩形裁剪原图
 *
 * 支持两种模式：
 * - 动态检测模式：每次裁剪都重新检测ROI（默认）
 * - 固定ROI模式：使用预设或锁定的ROI，保证所有图片裁剪尺寸一致
 */
class ImageCropper
{
public:
    ImageCropper();

    /**
     * @brief setThreshold 设置黑色判定阈值
     * @param threshold 灰度阈值，低于此值视为黑色（默认20）
     */
    void setThreshold(int threshold);

    /**
     * @brief setMargin 设置裁剪边距
     * @param margin 正值向外扩展，负值向内收缩（默认0，不收缩）
     */
    void setMargin(int margin);

    /**
     * @brief setFixedROI 设置固定的裁剪区域
     * @param roi 固定的ROI矩形
     *
     * 设置后，所有裁剪操作将使用此固定区域，确保输出尺寸一致
     */
    void setFixedROI(const QRect& roi);

    /**
     * @brief clearFixedROI 清除固定ROI，恢复动态检测模式
     */
    void clearFixedROI();

    /**
     * @brief lockCurrentROI 锁定当前检测到的ROI
     *
     * 将上次检测到的ROI设为固定ROI，后续裁剪将使用相同区域
     */
    void lockCurrentROI();

    /**
     * @brief isROILocked 检查是否处于固定ROI模式
     * @return true表示使用固定ROI，false表示动态检测
     */
    bool isROILocked() const { return m_useFixedROI; }

    /**
     * @brief getFixedROI 获取固定的ROI
     * @return 固定的ROI矩形，如果未设置返回空矩形
     */
    QRect getFixedROI() const { return m_fixedROI; }

    /**
     * @brief calibrateROI 校准ROI - 使用参考图像检测并锁定ROI
     * @param referenceImage 参考图像
     * @return true表示校准成功，false表示失败
     *
     * 此方法用于初始化时确定裁剪区域，后续所有图片将使用相同区域
     */
    bool calibrateROI(const QImage& referenceImage);
    bool calibrateROI(const cv::Mat& referenceImage);

    /**
     * @brief crop 裁剪QImage，去除黑色边缘
     * @param input 输入图像
     * @return 裁剪后的图像，如果裁剪失败返回原图
     */
    QImage crop(const QImage& input);

    /**
     * @brief crop 裁剪cv::Mat，去除黑色边缘
     * @param input 输入图像
     * @return 裁剪后的图像，如果裁剪失败返回原图
     */
    cv::Mat crop(const cv::Mat& input);

    /**
     * @brief detectROI 检测感兴趣区域（非黑色区域）
     * @param grayImage 输入灰度图像
     * @return 检测到的ROI矩形，如果检测失败返回空矩形
     */
    QRect detectROI(const cv::Mat& grayImage);

    /**
     * @brief getLastROI 获取上次检测到的ROI
     * @return 上次检测到的ROI矩形
     */
    QRect getLastROI() const { return m_lastROI; }

    /**
     * @brief isValid 检查上次裁剪是否有效
     * @return true表示裁剪成功，false表示使用了原图
     */
    bool isValid() const { return m_isValid; }

    /**
     * @brief getOutputSize 获取输出图像尺寸
     * @return 如果使用固定ROI，返回固定尺寸；否则返回上次裁剪尺寸
     */
    QSize getOutputSize() const;

private:
    int m_threshold = 20;       // 黑色判定阈值
    int m_margin = 0;           // 裁剪边距（正值外扩，负值内缩）
    QRect m_lastROI;            // 上次检测到的ROI
    QRect m_fixedROI;           // 固定的ROI
    bool m_useFixedROI = false; // 是否使用固定ROI
    bool m_isValid = false;     // 上次裁剪是否有效

    /**
     * @brief qimageToGrayMat 将QImage转换为灰度cv::Mat
     */
    cv::Mat qimageToGrayMat(const QImage& img);

    /**
     * @brief applyMarginToROI 对ROI应用边距
     * @param roi 原始ROI
     * @param imageSize 图像尺寸（用于边界检查）
     * @return 应用边距后的ROI
     */
    QRect applyMarginToROI(const QRect& roi, const QSize& imageSize);

    /**
     * @brief detectCircleROI 使用霍夫圆检测识别圆形干涉图区域
     * @param grayImage 输入灰度图像
     * @return 检测到的正方形ROI（完整包含圆形），失败返回空矩形
     */
    QRect detectCircleROI(const cv::Mat& grayImage);

    /**
     * @brief detectContourROI 使用轮廓检测识别干涉图区域（备选方法）
     * @param grayImage 输入灰度图像
     * @return 检测到的正方形ROI，失败返回空矩形
     */
    QRect detectContourROI(const cv::Mat& grayImage);
};

#endif // IMAGE_CROPPER_H
