#ifndef INTERFE_NAMESPACE_H
#define INTERFE_NAMESPACE_H

#include <QString>
#include <QImage>
#include <opencv2/opencv.hpp>

/**
 * @brief 干涉测量相关的工具函数命名空间
 *
 * 包含图像转换、文件管理等工具函数
 */

// 图像转换相关
// ===========================================================

/**
 * @brief convertToGrayMat 将QImage转换为灰度cv::Mat
 * @param img 输入图像
 * @return 灰度Mat (CV_8UC1)，失败返回空Mat
 */
cv::Mat convertToGrayMat(const QImage& img);

// 文件管理相关
// ===========================================================

/**
 * @brief createSessionFolder 创建当前测量会话的文件夹
 * @param basePath 保存的根路径
 * @return 创建的会话路径（带时间戳，格式：basePath/yyyyMMdd_HHmmss），失败返回空字符串
 *
 * 会自动创建子文件夹 captured_images 用于存放采集的原始图像
 */
QString createSessionFolder(const QString& basePath);

/**
 * @brief saveCapturedImage 保存采集的图像
 * @param sessionPath 会话路径（由createSessionFolder返回）
 * @param image 要保存的图像
 * @param index 图像序号
 * @return 成功返回true，失败返回false
 *
 * 图像会保存到 sessionPath/captured_images/img_XXX.png
 */
bool saveCapturedImage(const QString& sessionPath, const QImage& image, int index);

/**
 * @brief countImagesInFolder 统计文件夹中的图像数量
 * @param path 文件夹路径
 * @return 图像数量（支持png, jpg, jpeg, bmp, tif, tiff格式）
 */
int countImagesInFolder(const QString& path);

#endif // INTERFE_NAMESPACE_H
