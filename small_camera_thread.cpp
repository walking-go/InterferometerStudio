#include "small_camera_thread.h"
#include <QMutexLocker> // 用于线程锁

// 构造函数：初始化相机索引和成员变量
SmallCameraThread::SmallCameraThread(int cameraIndex, QObject *parent)
    : QThread(parent), m_cameraIndex(cameraIndex), m_running(false)
{
    // 初始化列表已完成关键初始化，无额外逻辑
}

// 析构函数：停止线程并释放相机资源
SmallCameraThread::~SmallCameraThread()
{
    stop(); // 停止相机
    wait(); // 等待线程结束
    if (m_capture.isOpened()) {
        m_capture.release(); // 释放相机设备
    }
}

// 获取当前帧（返回副本，避免线程冲突）
cv::Mat SmallCameraThread::getFrame()
{
    QMutexLocker locker(&m_mutex); // 自动加锁/解锁
    return m_frame.clone(); // 返回帧的副本，防止外部修改原数据
}

// 启动相机
void SmallCameraThread::startCamera()
{
    m_running = true;
    if (!isRunning()) { // 若线程未运行，则启动
        start();
    }
}

// 停止相机
void SmallCameraThread::stop()
{
    m_running = false; // 标记为停止，线程会在循环中退出
}

// 线程执行函数：相机帧捕获逻辑
void SmallCameraThread::run()
{
    // 打开指定索引的相机
    if (!m_capture.open(m_cameraIndex)) {
        emit errorOccurred(QString("相机%1打开失败").arg(m_cameraIndex));
        m_running = false;
        return;
    }

    // 循环捕获帧（直到m_running为false）
    while (m_running) {
        cv::Mat frame;
        m_capture >> frame; // 从相机读取一帧

        if (!frame.empty()) { // 若帧有效
            QMutexLocker locker(&m_mutex); // 加锁保护m_frame
            m_frame = frame; // 更新帧缓存
            emit frameReady(); // 发送新帧就绪信号
        }

        msleep(33); // 控制帧率（约30fps）
    }

    // 线程退出时释放相机
    m_capture.release();
}

// 获取QImage
QImage SmallCameraThread::getFrameAsQImage()
{
    QMutexLocker locker(&m_mutex); // 线程安全
    return cvMatToQImage(m_frame.clone()); // 返回QImage，外部无需依赖OpenCV
}

// 内部实现转换
QImage SmallCameraThread::cvMatToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(
                   rgb.data,
                   static_cast<int>(rgb.cols),
                   static_cast<int>(rgb.rows),
                   static_cast<int>(rgb.step),
                   QImage::Format_RGB888
                   ).copy();
    } else if (mat.type() == CV_8UC1) {
        return QImage(
                   mat.data,
                   static_cast<int>(mat.cols),
                   static_cast<int>(mat.rows),
                   static_cast<int>(mat.step),
                   QImage::Format_Grayscale8
                   ).copy();
    }
    return QImage();
}
