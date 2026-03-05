#include "measurement_worker.h"
#include "algorithm/APDAProcessor.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QImage>

MeasurementWorker::MeasurementWorker(QObject *parent)
    : QObject(parent)
    , m_processor(new APDAProcessor())
    , m_historyImageCount(0)
{
}

MeasurementWorker::~MeasurementWorker()
{
    delete m_processor;
    m_processor = nullptr;

    // 确保释放图像内存
    m_images.clear();
    m_images.shrink_to_fit();
}

void MeasurementWorker::setImages(std::vector<cv::Mat>&& images)
{
    QMutexLocker locker(&m_mutex);
    // 移动语义，转移所有权，避免拷贝
    m_images = std::move(images);
    qDebug() << "[MeasurementWorker] 接收图像数量:" << m_images.size();
}

void MeasurementWorker::setOriginalImage(const QImage& img)
{
    QMutexLocker locker(&m_mutex);
    m_originalImage = img.copy();  // 深拷贝
}

void MeasurementWorker::setSavePath(const QString& path)
{
    QMutexLocker locker(&m_mutex);
    m_savePath = path;
}

void MeasurementWorker::process()
{
    qDebug() << "[MeasurementWorker] 开始处理";

    // 获取图像数据（加锁）
    std::vector<cv::Mat> images;
    QImage originalImg;
    QString savePath;
    {
        QMutexLocker locker(&m_mutex);
        if (m_images.empty()) {
            emit failed("无图像数据");
            return;
        }
        // 移动数据到本地变量
        images = std::move(m_images);
        m_images.clear();
        originalImg = m_originalImage;
        m_originalImage = QImage();  // 释放
        savePath = m_savePath;
        m_savePath.clear();
    }

    emit progress(10, "检查图像数据...");

    // 检查图像有效性
    if (images.empty()) {
        emit failed("图像数据为空");
        return;
    }


    emit progress(20, "APDA算法处理中...");

    // 调用APDA算法
    cv::Mat finalPhase;
    finalPhase = m_processor->run(images);

    // 立即释放输入图像内存
    images.clear();
    images.shrink_to_fit();

    emit progress(90, "生成结果...");

    // 检查结果
    if (finalPhase.empty()) {
        emit failed("APDA算法返回空结果");
        return;
    }

    // 构建结果
    APDAResult result;
    result.wavefront = finalPhase.clone();  // 深拷贝确保数据独立
    result.is_valid = true;
    result.wavefront_pv = calculatePV(finalPhase);
    result.wavefront_rms = calculateRMS(finalPhase);
    result.real_per_pix = 1.0;  // 默认值，MainWindow会根据需要调整
    result.original_img = originalImg;
    result.save_path = savePath;  // 传递保存路径

    emit progress(100, "处理完成");
    emit finished(result);
}

double MeasurementWorker::calculatePV(const cv::Mat& phase)
{
    if (phase.empty()) return 0.0;
    double minVal, maxVal;
    cv::minMaxLoc(phase, &minVal, &maxVal);
    return maxVal - minVal;
}

double MeasurementWorker::calculateRMS(const cv::Mat& phase)
{
    if (phase.empty()) return 0.0;

    // 计算均值
    cv::Scalar meanVal = cv::mean(phase);
    double mean = meanVal[0];

    // 计算 RMS
    cv::Mat diff = phase - mean;
    cv::Mat squared;
    cv::multiply(diff, diff, squared);
    cv::Scalar sumSquared = cv::sum(squared);
    double rms = std::sqrt(sumSquared[0] / (phase.rows * phase.cols));
    return rms;
}

void MeasurementWorker::setHistoryPath(const QString& path, int imageCount)
{
    QMutexLocker locker(&m_mutex);
    m_historyPath = path;
    m_historyImageCount = imageCount;
}

void MeasurementWorker::processFromPath()
{
    qDebug() << "[MeasurementWorker] 开始处理历史图像";

    // 获取历史图像路径（加锁）
    QString historyPath;
    int imageCount;
    {
        QMutexLocker locker(&m_mutex);
        if (m_historyPath.isEmpty()) {
            emit failed("未设置历史图像路径");
            return;
        }
        historyPath = m_historyPath;
        imageCount = m_historyImageCount;
        m_historyPath.clear();
        m_historyImageCount = 0;
    }

    emit progress(5, "加载历史图像...");

    // 从文件夹加载图像
    std::vector<cv::Mat> images = loadImagesFromPath(historyPath, imageCount);

    if (images.empty()) {
        emit failed("无法加载历史图像");
        return;
    }

    emit progress(20, "APDA算法处理中...");

    // 调用APDA算法
    cv::Mat finalPhase = m_processor->run(images);

    // 立即释放输入图像内存
    images.clear();
    images.shrink_to_fit();

    emit progress(90, "生成结果...");

    // 检查结果
    if (finalPhase.empty()) {
        emit failed("APDA算法返回空结果");
        return;
    }

    // 获取原始图像（之前已设置）
    QImage originalImg;
    {
        QMutexLocker locker(&m_mutex);
        originalImg = m_originalImage;
        m_originalImage = QImage();
    }

    // 构建结果
    APDAResult result;
    result.wavefront = finalPhase.clone();
    result.is_valid = true;
    result.wavefront_pv = calculatePV(finalPhase);
    result.wavefront_rms = calculateRMS(finalPhase);
    result.real_per_pix = 1.0;
    result.original_img = originalImg;
    result.save_path = "";  // 历史图像处理不保存结果图像(original_img.png和topo2d.png)

    emit progress(100, "处理完成");
    emit finished(result);
}

std::vector<cv::Mat> MeasurementWorker::loadImagesFromPath(const QString& path, int maxCount)
{
    std::vector<cv::Mat> images;

    QDir dir(path);
    if (!dir.exists()) {
        qDebug() << "[MeasurementWorker] 目录不存在:" << path;
        return images;
    }

    // 支持多种图像格式
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.tif" << "*.tiff";
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);

    QFileInfoList fileList = dir.entryInfoList(QDir::Files);

    if (fileList.isEmpty()) {
        qDebug() << "[MeasurementWorker] 目录中没有找到图像文件:" << path;
        return images;
    }

    int loadCount = (maxCount > 0) ? qMin(maxCount, fileList.size()) : fileList.size();

    // 加载第一张图像并发送信号
    bool firstImageSent = false;

    for (int i = 0; i < loadCount; ++i) {
        QString filePath = fileList[i].absoluteFilePath();
        cv::Mat img = cv::imread(filePath.toStdString(), cv::IMREAD_GRAYSCALE);

        if (img.empty()) {
            qDebug() << "[MeasurementWorker] 无法读取图像:" << filePath;
            continue;
        }

        // 发送首张图像信号
        if (!firstImageSent) {
            // 将首张图像转换为 QImage
            QImage firstQImage(img.data, img.cols, img.rows, img.step, QImage::Format_Grayscale8);
            QImage firstImageCopy = firstQImage.copy();  // 深拷贝

            // 同时设置为原始图像
            {
                QMutexLocker locker(&m_mutex);
                m_originalImage = firstImageCopy;
            }

            emit firstImageReady(firstImageCopy);
            firstImageSent = true;
        }

        images.push_back(img);

        // 更新进度（加载阶段占5%-20%）
        int loadProgress = 5 + (i + 1) * 15 / loadCount;
        emit progress(loadProgress, QString("加载图像 (%1/%2)...").arg(i + 1).arg(loadCount));
    }

    qDebug() << "[MeasurementWorker] 加载完成，总图像数:" << images.size();
    return images;
}
