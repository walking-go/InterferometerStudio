#ifndef SIMPLE_CAMERA_H
#define SIMPLE_CAMERA_H

#include <QThread>
#include <opencv2/opencv.hpp>
#include <QMutex>
#include <QImage>

// 支持指定索引的相机类（可打开任意相机，如0=电脑相机，1=小相机）
class SmallCameraThread : public QThread
{
    Q_OBJECT
public:
    // 构造函数：接受相机索引（默认0）
    explicit SmallCameraThread(int cameraIndex = 0, QObject *parent = nullptr);
    // 析构函数
    ~SmallCameraThread() override;

    // 获取当前帧（返回副本，避免线程冲突）
    cv::Mat getFrame();
    QImage getFrameAsQImage();


signals:
    void frameReady(); // 新帧就绪信号（通知UI刷新）
    void errorOccurred(const QString& msg); // 错误信号（如相机打开失败）

public slots:
    void startCamera(); // 启动相机
    void stop(); // 停止相机

protected:
    // 线程执行函数（重写QThread）
    void run() override;

private:
    int m_cameraIndex; // 相机索引（存储指定的相机索引）
    cv::VideoCapture m_capture; // OpenCV相机捕获对象
    cv::Mat m_frame; // 当前帧缓存
    QMutex m_mutex; // 线程锁（保护帧缓存的读写安全）
    bool m_running; // 运行状态标记

    // 内部转换cv::Mat→QImage（私有，不暴露实现）
    QImage cvMatToQImage(const cv::Mat& mat);
};

#endif // SIMPLE_CAMERA_H
