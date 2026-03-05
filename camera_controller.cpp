#include "camera_controller.h"
#include <QDebug>
#include <QPainter>
CameraController::CameraController(QObject *parent)
    : QObject(parent)
{
}

CameraController::~CameraController()
{
    stopAll();
}

void CameraController::setImageWidget(CameraSwitcher *cameraSwitcher)
{
    m_cameraSwitcher = cameraSwitcher;
}

void CameraController::startSmallCamera(int cameraIndex)
{
    if (!m_smallCameraThread) {
        m_smallCameraThread = new SmallCameraThread(cameraIndex, this);
        connect(m_smallCameraThread, &SmallCameraThread::frameReady,
                this, &CameraController::onSmallCameraFrameReady,Qt::QueuedConnection);
        connect(m_smallCameraThread, &SmallCameraThread::errorOccurred,
                this, &CameraController::cameraError,Qt::QueuedConnection);
        m_smallCameraThread->startCamera();
        qDebug() << "小相机启动成功";
    }
}

void CameraController::startBaslerCamera(int cameraIndex)
{
    if (!m_baslerCameraThread) {
        m_baslerCameraThread = new BaslerCameraThread(cameraIndex, this);
        connect(m_baslerCameraThread, &BaslerCameraThread::imageReady,
                this, &CameraController::onBaslerCameraFrameReady,Qt::QueuedConnection);
        connect(m_baslerCameraThread, &BaslerCameraThread::statusUpdated,
                this, &CameraController::onBaslerStatus,Qt::QueuedConnection);
        connect(m_baslerCameraThread, &BaslerCameraThread::errorOccurred,
                this, &CameraController::onBaslerError,Qt::QueuedConnection);
        m_baslerCameraThread->start();
        qDebug() << "Basler相机启动成功";
    }
}

void CameraController::stopAll()
{
    if (m_smallCameraThread) {
        m_smallCameraThread->stop();
        m_smallCameraThread->wait();
        delete m_smallCameraThread;
        m_smallCameraThread = nullptr;
    }

    if (m_baslerCameraThread) {
        m_baslerCameraThread->quit();
        m_baslerCameraThread->wait();
        delete m_baslerCameraThread;
        m_baslerCameraThread = nullptr;
    }
}

void CameraController::switchMainSub()
{
    m_showSmallMain = !m_showSmallMain;
    if (m_cameraSwitcher) {
        m_cameraSwitcher->switchMainSub();
    }
}


// 处理副相机（smallCamera）的帧
void CameraController::onSmallCameraFrameReady() {
    if (!m_cameraSwitcher || !m_smallCameraThread)
        return;

    // 1. 获取原始相机帧
    QImage originalFrame = m_smallCameraThread->getFrameAsQImage();
    if (originalFrame.isNull()) {
        qDebug() << "获取相机帧失败";
        return;
    }

    // 2. 调用图像处理函数（绘制十字和最亮点）
    QImage processedFrame = processCameraFrame(originalFrame);

    // 3. 显示处理后的图像
    m_cameraSwitcher->setSubCameraFrame(processedFrame);
}

// 处理主相机（Basler）的帧
void CameraController::onBaslerCameraFrameReady(const QImage &img) {
    if (!m_cameraSwitcher)
        return;
    // 直接将主相机帧传给 TwoImgSwitcher 的主相机接口
    m_cameraSwitcher->setMainCameraFrame(img);
}

void CameraController::onBaslerStatus(const QString &msg)
{
    qDebug() << "Basler状态：" << msg;
}

void CameraController::onBaslerError(const QString &err)
{
    emit cameraError(err);
    if (m_cameraSwitcher)
        m_cameraSwitcher->showError(err);
}


// 在一张图像上：在中心画绿色十字，并在最亮点画红点。返回处理后的新 QImage（不修改原图）。
QImage CameraController::processCameraFrame(const QImage &input)
{
    if (input.isNull())
        return QImage();

    // ===== 使用原图作为输出，不覆盖背景 =====
    QImage img = input.convertToFormat(QImage::Format_ARGB32);

    const int w = img.width();
    const int h = img.height();
    const int centerX = w / 2 - 45;
    const int centerY = h / 2 + 62;

    // ===== 绘制贯穿整个画面的绿色十字 =====
    {
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing, true);

        QPen greenPen(Qt::green);
        greenPen.setWidth(2);
        p.setPen(greenPen);

        // 横线（贯穿整宽）
        p.drawLine(0, centerY, w, centerY);
        // 竖线（贯穿整高）
        p.drawLine(centerX, 0, centerX, h);
    }

    // ===== 在中心裁剪为正方形 =====
    const double cropRatio = 0.8;  // 缩放比例（可调）

    int minSide = static_cast<int>(qMin(w, h) * cropRatio); // 以短边为边长
    int cropX = centerX - minSide / 2;
    int cropY = centerY - minSide / 2;

    // 防止越界
    cropX = qMax(0, cropX);
    cropY = qMax(0, cropY);
    if (cropX + minSide > w) minSide = w - cropX;
    if (cropY + minSide > h) minSide = h - cropY;

    QImage cropped = img.copy(cropX, cropY, minSide, minSide);

    // ===== 旋转图像（逆时针 90°）=====
    QTransform transform;
    transform.rotate(-90); // 负号 = 逆时针
    QImage rotated = cropped.transformed(transform);

    return rotated;
}
