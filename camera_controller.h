#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <QObject>
#include "small_camera_thread.h"
#include "basler_camera_thread.h"
#include "camera_switcher.h"

class CameraController : public QObject
{
    Q_OBJECT
public:
    explicit CameraController(QObject *parent = nullptr);
    ~CameraController();

    void setImageWidget(CameraSwitcher *imgWidget);

    void startSmallCamera(int cameraIndex);
    void startBaslerCamera(int cameraIndex);
    void stopAll();
    void switchMainSub(); // 切换主副画面

    // 获取Basler相机线程指针，供外部控件建立连接
    BaslerCameraThread* baslerCameraThread() const { return m_baslerCameraThread; }

signals:
    void cameraError(const QString &msg);

private slots:
    void onSmallCameraFrameReady();
    void onBaslerCameraFrameReady(const QImage &img);
    void onBaslerStatus(const QString &msg);
    void onBaslerError(const QString &err);

private:
    CameraSwitcher *m_cameraSwitcher = nullptr; //两个相机画面切换

    // 两个相机线程
    SmallCameraThread *m_smallCameraThread = nullptr;
    BaslerCameraThread *m_baslerCameraThread = nullptr;

    bool m_showSmallMain = true; // true=小相机主画面，false=Basler主画面

    QImage processCameraFrame(const QImage& originalFrame);//处理小相机帧画面
};

#endif // CAMERA_CONTROLLER_H
