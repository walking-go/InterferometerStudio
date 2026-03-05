#include "image_cropper.h"
#include <QDebug>

ImageCropper::ImageCropper()
{
}

void ImageCropper::setThreshold(int threshold)
{
    m_threshold = qBound(0, threshold, 255);
}

void ImageCropper::setMargin(int margin)
{
    m_margin = margin;  // 允许正负值
}

void ImageCropper::setFixedROI(const QRect& roi)
{
    if (roi.isValid() && !roi.isEmpty()) {
        m_fixedROI = roi;
        m_useFixedROI = true;
        qDebug() << "[ImageCropper] 设置固定ROI:" << roi;
    }
}

void ImageCropper::clearFixedROI()
{
    m_fixedROI = QRect();
    m_useFixedROI = false;
    qDebug() << "[ImageCropper] 清除固定ROI，恢复动态检测模式";
}

void ImageCropper::lockCurrentROI()
{
    if (m_lastROI.isValid() && !m_lastROI.isEmpty()) {
        m_fixedROI = m_lastROI;
        m_useFixedROI = true;
        qDebug() << "[ImageCropper] 锁定当前ROI:" << m_fixedROI;
    } else {
        qDebug() << "[ImageCropper] 无法锁定ROI，上次检测无效";
    }
}

bool ImageCropper::calibrateROI(const QImage& referenceImage)
{
    if (referenceImage.isNull()) {
        return false;
    }

    cv::Mat grayMat = qimageToGrayMat(referenceImage);
    if (grayMat.empty()) {
        return false;
    }

    QRect roi = detectROI(grayMat);
    if (roi.isEmpty() || !roi.isValid()) {
        qDebug() << "[ImageCropper] 校准失败：无法检测有效ROI";
        return false;
    }

    // 应用边距后设置为固定ROI
    QSize imgSize(referenceImage.width(), referenceImage.height());
    m_fixedROI = applyMarginToROI(roi, imgSize);
    m_useFixedROI = true;
    m_lastROI = m_fixedROI;

    qDebug() << "[ImageCropper] 校准成功，固定ROI:" << m_fixedROI
             << "原始检测:" << roi;
    return true;
}

bool ImageCropper::calibrateROI(const cv::Mat& referenceImage)
{
    if (referenceImage.empty()) {
        return false;
    }

    cv::Mat grayMat;
    if (referenceImage.channels() == 1) {
        grayMat = referenceImage;
    } else if (referenceImage.channels() == 3) {
        cv::cvtColor(referenceImage, grayMat, cv::COLOR_BGR2GRAY);
    } else if (referenceImage.channels() == 4) {
        cv::cvtColor(referenceImage, grayMat, cv::COLOR_BGRA2GRAY);
    } else {
        return false;
    }

    QRect roi = detectROI(grayMat);
    if (roi.isEmpty() || !roi.isValid()) {
        qDebug() << "[ImageCropper] 校准失败：无法检测有效ROI";
        return false;
    }

    // 应用边距后设置为固定ROI
    QSize imgSize(referenceImage.cols, referenceImage.rows);
    m_fixedROI = applyMarginToROI(roi, imgSize);
    m_useFixedROI = true;
    m_lastROI = m_fixedROI;

    qDebug() << "[ImageCropper] 校准成功，固定ROI:" << m_fixedROI
             << "原始检测:" << roi;
    return true;
}

QSize ImageCropper::getOutputSize() const
{
    if (m_useFixedROI && m_fixedROI.isValid()) {
        return m_fixedROI.size();
    }
    return m_lastROI.size();
}

QRect ImageCropper::applyMarginToROI(const QRect& roi, const QSize& imageSize)
{
    if (roi.isEmpty() || !roi.isValid()) {
        return roi;
    }

    // 正值外扩，负值内缩
    int x = roi.x() - m_margin;
    int y = roi.y() - m_margin;
    int w = roi.width() + 2 * m_margin;
    int h = roi.height() + 2 * m_margin;

    // 确保在图像范围内
    x = qMax(0, x);
    y = qMax(0, y);
    w = qMin(w, imageSize.width() - x);
    h = qMin(h, imageSize.height() - y);

    // 最小尺寸检查
    if (w < 64 || h < 64) {
        qDebug() << "[ImageCropper] 边距调整后ROI过小，使用原始ROI";
        return roi;
    }

    return QRect(x, y, w, h);
}

QImage ImageCropper::crop(const QImage& input)
{
    if (input.isNull()) {
        m_isValid = false;
        return input;
    }

    QRect roi;
    QSize imgSize = input.size();

    // 使用固定ROI或动态检测
    if (m_useFixedROI && m_fixedROI.isValid()) {
        roi = m_fixedROI;
        // 确保固定ROI在当前图像范围内
        roi = roi.intersected(QRect(0, 0, imgSize.width(), imgSize.height()));
    } else {
        // 动态检测模式
        cv::Mat grayMat = qimageToGrayMat(input);
        if (grayMat.empty()) {
            m_isValid = false;
            return input;
        }

        QRect detectedROI = detectROI(grayMat);
        if (detectedROI.isEmpty() || !detectedROI.isValid()) {
            m_isValid = false;
            return input;
        }

        // 应用边距
        roi = applyMarginToROI(detectedROI, imgSize);
    }

    if (roi.isEmpty() || !roi.isValid()) {
        m_isValid = false;
        return input;
    }

    // 裁剪QImage
    QImage cropped = input.copy(roi);
    if (cropped.isNull()) {
        m_isValid = false;
        return input;
    }

    m_isValid = true;
    m_lastROI = roi;

    // qDebug() << "[ImageCropper] 裁剪成功，原始尺寸:" << input.size()
    //          << "裁剪后:" << cropped.size() << "ROI:" << roi
    //          << (m_useFixedROI ? "(固定ROI)" : "(动态检测)");

    return cropped;
}

cv::Mat ImageCropper::crop(const cv::Mat& input)
{
    if (input.empty()) {
        m_isValid = false;
        return input;
    }

    QRect roi;
    QSize imgSize(input.cols, input.rows);

    // 使用固定ROI或动态检测
    if (m_useFixedROI && m_fixedROI.isValid()) {
        roi = m_fixedROI;
        // 确保固定ROI在当前图像范围内
        roi = roi.intersected(QRect(0, 0, imgSize.width(), imgSize.height()));
    } else {
        // 动态检测模式 - 转换为灰度图
        cv::Mat grayMat;
        if (input.channels() == 1) {
            grayMat = input;
        } else if (input.channels() == 3) {
            cv::cvtColor(input, grayMat, cv::COLOR_BGR2GRAY);
        } else if (input.channels() == 4) {
            cv::cvtColor(input, grayMat, cv::COLOR_BGRA2GRAY);
        } else {
            m_isValid = false;
            return input;
        }

        QRect detectedROI = detectROI(grayMat);
        if (detectedROI.isEmpty() || !detectedROI.isValid()) {
            m_isValid = false;
            return input;
        }

        // 应用边距
        roi = applyMarginToROI(detectedROI, imgSize);
    }

    if (roi.isEmpty() || !roi.isValid()) {
        m_isValid = false;
        return input;
    }

    // 裁剪cv::Mat
    cv::Rect cvRoi(roi.x(), roi.y(), roi.width(), roi.height());

    // 确保ROI在图像范围内
    cvRoi &= cv::Rect(0, 0, input.cols, input.rows);
    if (cvRoi.width <= 0 || cvRoi.height <= 0) {
        m_isValid = false;
        return input;
    }

    cv::Mat cropped = input(cvRoi).clone();

    m_isValid = true;
    m_lastROI = roi;

    qDebug() << "[ImageCropper] 裁剪成功，原始尺寸:" << input.cols << "x" << input.rows
             << "裁剪后:" << cropped.cols << "x" << cropped.rows
             << (m_useFixedROI ? "(固定ROI)" : "(动态检测)");

    return cropped;
}

QRect ImageCropper::detectROI(const cv::Mat& grayImage)
{
    if (grayImage.empty() || grayImage.channels() != 1) {
        return QRect();
    }

    // 方法1：尝试使用霍夫圆检测（针对圆形干涉图）
    QRect circleROI = detectCircleROI(grayImage);
    if (circleROI.isValid() && !circleROI.isEmpty()) {
        qDebug() << "[ImageCropper] 使用霍夫圆检测结果:" << circleROI;
        return circleROI;
    }

    // 方法2：回退到轮廓检测
    qDebug() << "[ImageCropper] 霍夫圆检测失败，使用轮廓检测";
    return detectContourROI(grayImage);
}

QRect ImageCropper::detectCircleROI(const cv::Mat& grayImage)
{
    // 高斯模糊减少噪声
    cv::Mat blurred;
    cv::GaussianBlur(grayImage, blurred, cv::Size(9, 9), 2);

    // 霍夫圆检测 - 使用更宽松的参数以检测完整的干涉图边界
    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(blurred, circles, cv::HOUGH_GRADIENT,
                     1,                          // dp: 累加器分辨率与图像分辨率的比值
                     grayImage.rows / 4,         // minDist: 圆心之间的最小距离
                     80,                         // param1: Canny边缘检测的高阈值（降低以检测更多边缘）
                     40,                         // param2: 累加器阈值（降低以检测更多圆）
                     grayImage.rows / 10,        // minRadius: 最小半径
                     grayImage.rows * 2 / 3);    // maxRadius: 最大半径（增大）

    if (circles.empty()) {
        qDebug() << "[ImageCropper] 霍夫圆检测未找到圆";
        return QRect();
    }

    // 找到最大的圆（假设干涉图是最大的圆形区域）
    float maxRadius = 0;
    cv::Vec3f bestCircle;
    for (const auto& circle : circles) {
        if (circle[2] > maxRadius) {
            maxRadius = circle[2];
            bestCircle = circle;
        }
    }

    float cx = bestCircle[0];  // 圆心x
    float cy = bestCircle[1];  // 圆心y
    float r = bestCircle[2];   // 半径

    // 增加5%的安全系数，确保完整包含圆形
    r = r * 1.05f;

    qDebug() << "[ImageCropper] 检测到圆形干涉图，圆心:(" << cx << "," << cy
             << ") 半径(含安全系数):" << r;

    // 计算正方形外接矩形（完整包含圆形）
    int x = static_cast<int>(cx - r);
    int y = static_cast<int>(cy - r);
    int size = static_cast<int>(2 * r);

    // 确保在图像范围内
    x = qMax(0, x);
    y = qMax(0, y);
    int w = qMin(size, grayImage.cols - x);
    int h = qMin(size, grayImage.rows - y);

    // 保持正方形：取较小边
    int squareSize = qMin(w, h);
    if (squareSize < 64) {
        qDebug() << "[ImageCropper] 圆形ROI尺寸过小:" << squareSize;
        return QRect();
    }

    return QRect(x, y, squareSize, squareSize);
}

QRect ImageCropper::detectContourROI(const cv::Mat& grayImage)
{
    // 二值化：高于阈值的为白色（非黑色区域）
    cv::Mat binary;
    cv::threshold(grayImage, binary, m_threshold, 255, cv::THRESH_BINARY);

    // 形态学操作去除噪点，使用较大的核以连接干涉条纹
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(21, 21));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);  // 先闭操作填充缝隙
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);   // 再开操作去除噪点

    // 再次进行膨胀操作，确保边缘区域被包含
    cv::Mat dilateKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(11, 11));
    cv::dilate(binary, binary, dilateKernel);

    // 查找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        qDebug() << "[ImageCropper] 未找到有效轮廓";
        return QRect();
    }

    // 找到最大轮廓
    double maxArea = 0;
    int maxIdx = -1;
    for (size_t i = 0; i < contours.size(); ++i) {
        double area = cv::contourArea(contours[i]);
        if (area > maxArea) {
            maxArea = area;
            maxIdx = static_cast<int>(i);
        }
    }

    if (maxIdx < 0) {
        return QRect();
    }

    // 使用最小外接圆而非矩形，以更好地适应圆形干涉图
    cv::Point2f center;
    float radius;
    cv::minEnclosingCircle(contours[maxIdx], center, radius);

    // 增加5%的安全系数
    radius = radius * 1.05f;

    qDebug() << "[ImageCropper] 轮廓检测到区域，最小外接圆圆心:(" << center.x << "," << center.y
             << ") 半径(含安全系数):" << radius;

    // 计算正方形外接矩形
    int x = static_cast<int>(center.x - radius);
    int y = static_cast<int>(center.y - radius);
    int size = static_cast<int>(2 * radius);

    // 确保在图像范围内
    x = qMax(0, x);
    y = qMax(0, y);
    int w = qMin(size, grayImage.cols - x);
    int h = qMin(size, grayImage.rows - y);

    // 保持正方形
    int squareSize = qMin(w, h);
    if (squareSize < 64) {
        qDebug() << "[ImageCropper] ROI尺寸过小:" << squareSize;
        return QRect();
    }

    return QRect(x, y, squareSize, squareSize);
}

cv::Mat ImageCropper::qimageToGrayMat(const QImage& img)
{
    if (img.isNull()) {
        return cv::Mat();
    }

    cv::Mat result;

    switch (img.format()) {
    case QImage::Format_Grayscale8:
        result = cv::Mat(img.height(), img.width(), CV_8UC1,
                         const_cast<uchar*>(img.bits()),
                         static_cast<size_t>(img.bytesPerLine())).clone();
        break;

    case QImage::Format_Grayscale16:
        {
            cv::Mat mat16(img.height(), img.width(), CV_16UC1,
                          const_cast<uchar*>(img.bits()),
                          static_cast<size_t>(img.bytesPerLine()));
            mat16.convertTo(result, CV_8UC1, 255.0 / 65535.0);
        }
        break;

    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        {
            cv::Mat bgra(img.height(), img.width(), CV_8UC4,
                         const_cast<uchar*>(img.bits()),
                         static_cast<size_t>(img.bytesPerLine()));
            cv::cvtColor(bgra, result, cv::COLOR_BGRA2GRAY);
        }
        break;

    case QImage::Format_RGB888:
        {
            cv::Mat rgb(img.height(), img.width(), CV_8UC3,
                        const_cast<uchar*>(img.bits()),
                        static_cast<size_t>(img.bytesPerLine()));
            cv::cvtColor(rgb, result, cv::COLOR_RGB2GRAY);
        }
        break;

    default:
        {
            QImage grayImg = img.convertToFormat(QImage::Format_Grayscale8);
            if (!grayImg.isNull()) {
                result = cv::Mat(grayImg.height(), grayImg.width(), CV_8UC1,
                                 const_cast<uchar*>(grayImg.bits()),
                                 static_cast<size_t>(grayImg.bytesPerLine())).clone();
            }
        }
        break;
    }

    return result;
}
