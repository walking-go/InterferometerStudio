#ifndef BASLER_CAMERA_THREAD_H
#define BASLER_CAMERA_THREAD_H
#include <QThread>
#include <QMutex>
#include "basler_camera_controller.h"
class BaslerCameraThread : public QThread {
    Q_OBJECT
public:
    explicit BaslerCameraThread(int cameraId = 0, QObject *parent = nullptr);
    ~BaslerCameraThread() override;
    // 曝光控制
    void setExposureTime(int us);
protected:
    void run() override; // 线程主函数
private slots:
    void onImageCaptured(const QImage& img); // 接收相机图像
    void onCameraError(const QString& msg); // 接收相机错误

public slots:
    // void setExposureSlot(int us);  // 新增：异步设置曝光槽
public:
    // 测量采集模式控制
    void setMeasurementCapturing(bool enabled);
    bool isMeasurementCapturing() const;

private:
    int m_cameraId;
    camera::BaslerCameraController* m_camera = nullptr;
    bool m_isRunning = false;
    bool m_measurementCapturing = false;  // 测量采集模式
    mutable QMutex m_mutex; // 线程安全锁
    int m_exposureTime = 1000; // 曝光时间缓存
signals:
    void imageReady(const QImage& image); // 图像就绪（预览用，已缩放）
    void fullResolutionImageReady(const QImage& image); // 原始分辨率图像（测量用）
    void statusUpdated(const QString& msg); // 状态更新（发送到UI线程）
    void errorOccurred(const QString& err); // 错误信息（发送到UI线程）

    void exposureParamsUpdated(int min, int max, int current);
};
#endif // BASLER_CAMERA_THREAD_H
