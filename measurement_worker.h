#ifndef MEASUREMENT_WORKER_H
#define MEASUREMENT_WORKER_H

#include <QObject>
#include <QMutex>
#include <vector>
#include <opencv2/opencv.hpp>
#include "algorithm/alg.h"

class APDAProcessor;

/**
 * @brief MeasurementWorker 测量工作器
 *
 * 负责在工作线程中执行APDA算法处理。
 * 使用方式：
 * 1. 创建实例并移动到工作线程
 * 2. 调用 setImages() 设置待处理图像
 * 3. 调用 process() 槽函数触发处理
 * 4. 监听 finished/failed 信号获取结果
 *
 * 内存管理：
 * - setImages() 接收图像的所有权（通过移动语义）
 * - 处理完成后自动释放图像内存
 */
class MeasurementWorker : public QObject
{
    Q_OBJECT

public:
    explicit MeasurementWorker(QObject *parent = nullptr);
    ~MeasurementWorker();

    /**
     * @brief setImages 设置待处理的图像数据
     * @param images 图像数据，通过移动语义转移所有权
     * @note 此函数线程安全，可从任意线程调用
     */
    void setImages(std::vector<cv::Mat>&& images);

    /**
     * @brief setOriginalImage 设置原始图像（用于结果中的original_img）
     * @param img 原始图像
     */
    void setOriginalImage(const QImage& img);

    /**
     * @brief setSavePath 设置保存路径（会话文件夹路径）
     * @param path 保存路径
     */
    void setSavePath(const QString& path);

    /**
     * @brief setHistoryPath 设置历史图像路径（用于从文件加载处理）
     * @param path 历史图像文件夹路径
     * @param imageCount 图像数量
     */
    void setHistoryPath(const QString& path, int imageCount);

public slots:
    /**
     * @brief process 执行APDA算法处理
     * @note 此槽应在工作线程中调用（通过信号触发）
     */
    void process();

    /**
     * @brief processFromPath 从文件路径加载图像并执行APDA算法处理
     * @note 此槽应在工作线程中调用（通过信号触发）
     */
    void processFromPath();

signals:
    /**
     * @brief progress 处理进度信号
     * @param percent 进度百分比 (0-100)
     * @param message 进度描述
     */
    void progress(int percent, const QString& message);

    /**
     * @brief finished 处理完成信号
     * @param result APDA处理结果
     */
    void finished(const APDAResult& result);

    /**
     * @brief failed 处理失败信号
     * @param errorMsg 错误信息
     */
    void failed(const QString& errorMsg);

    /**
     * @brief firstImageReady 首张图像准备完成信号（用于历史图像处理）
     * @param firstImage 首张图像
     */
    void firstImageReady(const QImage& firstImage);

private:
    APDAProcessor* m_processor;
    std::vector<cv::Mat> m_images;
    QImage m_originalImage;
    QString m_savePath;
    QString m_historyPath;      // 历史图像文件夹路径
    int m_historyImageCount;    // 历史图像数量
    mutable QMutex m_mutex;

    double calculatePV(const cv::Mat& phase);
    double calculateRMS(const cv::Mat& phase);

    /**
     * @brief loadImagesFromPath 从文件夹加载图像
     * @param path 文件夹路径
     * @param maxCount 最大加载数量
     * @return 加载的图像列表
     */
    std::vector<cv::Mat> loadImagesFromPath(const QString& path, int maxCount);
};

#endif // MEASUREMENT_WORKER_H
