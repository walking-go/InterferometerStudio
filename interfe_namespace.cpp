#include "interfe_namespace.h"
#include <QDateTime>
#include <QDir>
#include <QDebug>

// 图像转换相关
// ===========================================================

cv::Mat convertToGrayMat(const QImage& img)
{
    if (img.isNull()) {
        return cv::Mat();
    }

    cv::Mat result;

    switch (img.format()) {
    case QImage::Format_Grayscale8:
        // 直接复制灰度数据
        result = cv::Mat(img.height(), img.width(), CV_8UC1,
                         const_cast<uchar*>(img.bits()),
                         static_cast<size_t>(img.bytesPerLine())).clone();
        break;

    case QImage::Format_Grayscale16:
        // 16位灰度转8位
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
        // BGRA格式转灰度
        {
            cv::Mat bgra(img.height(), img.width(), CV_8UC4,
                         const_cast<uchar*>(img.bits()),
                         static_cast<size_t>(img.bytesPerLine()));
            cv::cvtColor(bgra, result, cv::COLOR_BGRA2GRAY);
        }
        break;

    case QImage::Format_RGB888:
        // RGB格式转灰度
        {
            cv::Mat rgb(img.height(), img.width(), CV_8UC3,
                        const_cast<uchar*>(img.bits()),
                        static_cast<size_t>(img.bytesPerLine()));
            cv::cvtColor(rgb, result, cv::COLOR_RGB2GRAY);
        }
        break;

    default:
        // 其他格式先转为灰度QImage
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

// 文件管理相关
// ===========================================================

QString createSessionFolder(const QString& basePath)
{
    if (basePath.isEmpty()) {
        qDebug() << "[interfe_namespace] 未设置保存路径，无法创建会话文件夹";
        return QString();
    }

    // 使用时间戳创建会话文件夹 (格式: yyyyMMdd_HHmmss)
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString sessionPath = basePath + "/" + timestamp;

    QDir dir;
    if (!dir.mkpath(sessionPath)) {
        qDebug() << "[interfe_namespace] 创建会话文件夹失败:" << sessionPath;
        return QString();
    }

    // 创建captured_images子文件夹存放采集的原始图像
    QString capturedImagesPath = sessionPath + "/captured_images";
    if (!dir.mkpath(capturedImagesPath)) {
        qDebug() << "[interfe_namespace] 创建captured_images文件夹失败";
        return QString();
    }

    return sessionPath;
}

bool saveCapturedImage(const QString& sessionPath, const QImage& image, int index)
{
    if (sessionPath.isEmpty() || image.isNull()) {
        return false;
    }

    // 保存到captured_images子文件夹
    QString filePath = sessionPath + "/captured_images/"
                       + QString("img_%1.png").arg(index, 3, 10, QChar('0'));

    if (image.save(filePath)) {
        // qDebug() << "[interfe_namespace] 保存图像:" << filePath;
        return true;
    } else {
        qDebug() << "[interfe_namespace] 保存图像失败:" << filePath;
        return false;
    }
}

int countImagesInFolder(const QString& path)
{
    QDir dir(path);
    if (!dir.exists()) {
        return 0;
    }

    // 支持多种图像格式
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.tif" << "*.tiff";
    dir.setNameFilters(filters);

    return dir.entryList(QDir::Files).count();
}
