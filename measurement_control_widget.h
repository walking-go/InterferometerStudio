#ifndef MEASUREMENT_CONTROL_WIDGET_H
#define MEASUREMENT_CONTROL_WIDGET_H

#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QString>
#include <QRect>
#include <QtConcurrent>
#include <vector>
#include <opencv2/opencv.hpp>
#include "algorithm/alg.h"
#include "image_cropper.h"
#include "interfe_namespace.h"

// 前向声明
class BaslerCameraThread;
class MeasurementWorker;
class MotorController;
class ROISelectionDialog;

namespace Ui {
class MeasurementControlWidget;
}

/**
 * @brief MeasurementControlWidget 测量控制控件
 *
 * 功能：
 * 1. 提供UI界面设置采集参数
 * 2. 从Basler相机采集指定数量的图像
 * 3. 将图像交给MeasurementWorker处理
 * 4. 显示进度并发送结果
 *
 * 工作流程：
 * 1. 用户点击"执行测量" -> 开始采集
 * 2. 监听相机 imageReady 信号，收集图像
 * 3. 采集完成 -> 将图像移交给Worker
 * 4. 触发Worker处理 -> 显示处理进度
 * 5. 处理完成 -> 发送 measurementCompleted 信号
 *
 * 内存管理：
 * - 采集时直接转为灰度cv::Mat存储（减少内存占用）
 * - 采集完成后通过移动语义将数据转移给Worker（零拷贝）
 * - Worker处理完成后自动释放
 */
class MeasurementControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MeasurementControlWidget(QWidget *parent = nullptr);
    ~MeasurementControlWidget();

    /**
     * @brief setBaslerCameraThread 设置Basler相机线程
     * @param thread 相机线程指针（不转移所有权）
     */
    void setBaslerCameraThread(BaslerCameraThread* thread);

    /**
     * @brief setMotorController 设置电机控制器
     * @param controller 电机控制器指针（不转移所有权）
     */
    void setMotorController(MotorController* controller);

signals:
    /**
     * @brief measurementCompleted 测量完成信号
     * @param result APDA算法结果
     */
    void measurementCompleted(const APDAResult& result);

    /**
     * @brief measurementStarted 测量开始信号（首张图像采集完成后发送）
     * @param firstInterferogram 首张干涉图
     */
    void measurementStarted(const QImage& firstInterferogram);

    /**
     * @brief statusChanged 状态变化信号
     * @param status 状态描述
     */
    void statusChanged(const QString& status);

private slots:
    // UI事件
    void onStartButtonClicked();
    void onBrowsePathClicked();
    void onSelectROIClicked();

    // 历史图像处理相关
    void onBrowseHistoryPathClicked();
    void onProcessHistoryClicked();

    // 相机图像接收（单次采集完成）
    void onSingleImageCaptured(const QImage& image);

    // Worker信号处理
    void onWorkerProgress(int percent, const QString& message);
    void onWorkerFinished(const APDAResult& result);
    void onWorkerFailed(const QString& errorMsg);
    void onWorkerFirstImageReady(const QImage& firstImage);

private:
    Ui::MeasurementControlWidget *ui;

    // 相机
    BaslerCameraThread* m_cameraThread = nullptr;

    // 电机控制器
    MotorController* m_motorController = nullptr;

    // 工作线程
    QThread* m_workerThread = nullptr;
    MeasurementWorker* m_worker = nullptr;

    // 采集状态
    enum State {
        Idle,       // 空闲
        Capturing,  // 采集中
        Processing  // 处理中
    };
    State m_state = Idle;

    // 采集数据
    int m_targetCount = 50;
    std::vector<cv::Mat> m_capturedImages;
    QImage m_firstImage;  // 保存第一张原始图像
    mutable QMutex m_mutex;

    // 往返移动测量相关
    int m_currentStep = 0;           // 当前步骤（0-49）
    bool m_waitingForImage = false;  // 是否正在等待图像采集

    // 定时采集相关
    QTimer* m_captureTimer = nullptr;   // 定时采集定时器
    int m_captureInterval = 150;        // 采集间隔(ms)，可调整测试

    // 图像裁剪器（自动检测并裁剪，暂时只使用固定ROI函数）
    ImageCropper m_imageCropper;

    // 手动框选的ROI区域
    QRect m_manualROI;               // 用户手动框选的ROI
    bool m_hasManualROI = false;     // 是否已设置手动ROI

    // 保存路径相关
    QString m_savePath;              // 用户选择的保存根路径
    QString m_currentSessionPath;    // 当前测量会话的保存路径（带时间戳）

    // 历史图像处理相关
    QString m_historyImagePath;      // 历史图像文件夹路径
    int m_historyImageCount;         // 历史图像数量

    // 电机初始位置（用于测量后复位）
    int m_initialPositionX = 0;
    int m_initialPositionY = 0;
    int m_initialPositionZ = 0;
    bool m_hasInitialPosition = false;  // 是否已记录初始位置

    // 辅助函数
    void setState(State state);
    void updateUI();
    void updateStatus(const QString& status);
    void updateProgress(int percent);
    void startCapture();
    void stopCapture();
    void startProcessing();

    // /**
    //  * @brief moveMotorAndCapture 执行电机移动并触发图像采集
    //  * 移动规则：单轴分步进退，每6步为一个周期
    //  * X前进 -> Y前进 -> Z前进 -> X后退 -> Y后退 -> Z后退 -> 循环
    //  */
    // void moveMotorAndCapture();

    /**
     * @brief onCaptureTimerTimeout 定时采集定时器触发
     * 并行执行：采集当前图像 + 发送下一次电机命令
     */
    void onCaptureTimerTimeout();

    /**
     * @brief sendMotorCommand 发送电机移动命令（不等待）
     * @param step 当前步骤号
     */
    void sendMotorCommand(int step);

    /**
     * @brief moveSingleAxis 移动单个轴指定微步数
     * @param axis 轴名称 ("X", "Y", 或 "Z")
     * @param usteps 微步数（正数前进，负数后退）
     */
    void moveSingleAxis(const QString& axis, int usteps);

    /**
     * @brief initSavePath 初始化保存路径（从配置文件读取）
     */
    void initSavePath();

    /**
     * @brief initHistoryPath 初始化历史图像路径（从配置文件读取）
     */
    void initHistoryPath();

    /**
     * @brief showROISelectionDialog 显示ROI框选对话框
     * @return 成功返回true，取消返回false
     */
    bool showROISelectionDialog();

    /**
     * @brief updateROIStatusLabel 更新ROI状态显示
     */
    void updateROIStatusLabel();

    /**
     * @brief startHistoryProcessing 开始处理历史图像
     */
    void startHistoryProcessing();

    /**
     * @brief resetMotorsToInitialPosition 将电机复位到测量前的初始位置
     * 用于解决采集帧数非6倍数时电机位置偏移问题
     */
    void resetMotorsToInitialPosition();
};

#endif // MEASUREMENT_CONTROL_WIDGET_H
