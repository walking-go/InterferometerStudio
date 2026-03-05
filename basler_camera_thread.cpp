#include "basler_camera_thread.h"
#include <QDebug>
#include <QDateTime>

BaslerCameraThread::BaslerCameraThread(int cameraId, QObject *parent)
    : QThread(parent), m_cameraId(cameraId) {}
BaslerCameraThread::~BaslerCameraThread() {
    QMutexLocker lock(&m_mutex);
    if (m_camera) {
        m_camera->stop();
        m_camera->close();
        delete m_camera;
        m_camera = nullptr;
    }
    m_isRunning = false;
}

// 设置曝光时间
void BaslerCameraThread::setExposureTime(int us) {
    QMutexLocker lock(&m_mutex);
    m_exposureTime = us;
    if (m_camera && m_camera->isOpen()) {
        qDebug() << "[曝光设置] 相机打开，设置成功";
        m_camera->setExposureTime(us);
    }

}

void BaslerCameraThread::run()
{
    // 1) 在互斥保护下创建相机对象（构造函数中不做枚举/打开）
    {
        QMutexLocker lock(&m_mutex);
        m_camera = new camera::BaslerCameraController(m_cameraId);
        m_isRunning = true;
    }

    // 2) 连接信号（使用 QueuedConnection 跨线程队列）
    connect(m_camera, &camera::BaslerCameraController::captured,
            this, &BaslerCameraThread::onImageCaptured, Qt::QueuedConnection);

    // 3) 枚举并打开相机（注意：CameraBasler 中 getEnumerationDevices() 已可用）
    if (m_camera->getEnumerationDevices() <= 0) {
        emit errorOccurred("未检测到 Basler 相机");
        // 清理并退出线程
        QMutexLocker lock(&m_mutex);
        m_isRunning = false;
        delete m_camera;
        m_camera = nullptr;
        return;
    }

    // 4) 尝试 open/start，CameraBasler::open/start 是 void，所以直接调用并捕获异常
    bool initSuccess = false;
    try {
        m_camera->open(); // open() 内部会调用 cameraOpen(...) 并打开设备
        // 在打开之后再设置参数
        m_camera->setExposureMode(camera::ManualExpo);
        m_camera->setExposureTime(m_exposureTime);
        m_camera->start(); // start() 是 void，开始连续采集
        initSuccess = true;
    } catch (const Pylon::GenericException& e) {
        emit errorOccurred(QString("相机启动异常: %1").arg(e.GetDescription()));
    } catch (const std::exception& e) {
        emit errorOccurred(QString("相机启动异常(std): %1").arg(e.what()));
    } catch (...) {
        emit errorOccurred("相机启动未知异常");
    }

    if (initSuccess) {
        emit statusUpdated("相机启动成功，正在显示图像");
        // 获取曝光参数
        int minExpo = m_camera->getExposureTimeMin();
        int maxExpo = m_camera->getExposureTimeMax();
        int currentExpo = m_camera->getExposureTime();
        emit exposureParamsUpdated(minExpo, maxExpo, currentExpo);
    }

    if (!initSuccess) {
        QMutexLocker lock(&m_mutex);
        m_isRunning = false;
        delete m_camera;
        m_camera = nullptr;
        return;
    }

    emit statusUpdated("相机启动成功，正在显示图像");

    // 5) 进入线程事件循环（保持线程存活以接收 queued 信号）
    exec();

    // 6) 线程退出后清理相机（确保关闭、释放）
    {
        QMutexLocker lock(&m_mutex);
        m_isRunning = false;
        if (m_camera) {
            try {
                m_camera->stop();
                m_camera->close();
            } catch (...) { /* 忽略关闭异常 */ }
            delete m_camera;
            m_camera = nullptr;
        }
    }
}

// 测量采集模式控制
void BaslerCameraThread::setMeasurementCapturing(bool enabled)
{
    QMutexLocker lock(&m_mutex);
    m_measurementCapturing = enabled;
    qDebug() << "[BaslerCameraThread] 测量采集模式:" << (enabled ? "开启" : "关闭");
}

bool BaslerCameraThread::isMeasurementCapturing() const
{
    QMutexLocker lock(&m_mutex);
    return m_measurementCapturing;
}

// 处理采集的图像
void BaslerCameraThread::onImageCaptured(const QImage& img) {
    static qint64 lastTime = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    bool isRunning = false;
    bool isMeasuring = false;

    {
        QMutexLocker lock(&m_mutex);
        isRunning = m_isRunning;
        isMeasuring = m_measurementCapturing;
    }

    if (!isRunning || img.isNull()) {
        return;
    }

    // 测量采集模式：发送原始分辨率图像给测量组件
    if (isMeasuring) {
        emit fullResolutionImageReady(img.copy());
    }

    // 预览始终运行：限制帧率，发送缩放图像
    if (now - lastTime < 50) return; // 限制每 50ms 一帧 ≈ 20fps
    lastTime = now;

    QImage scaled = img.scaled(640, 480, Qt::KeepAspectRatio, Qt::FastTransformation);
    emit imageReady(scaled);
}
// 处理相机错误
void BaslerCameraThread::onCameraError(const QString& msg) {
    emit statusUpdated(msg);
}
