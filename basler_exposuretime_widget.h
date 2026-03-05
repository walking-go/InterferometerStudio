#ifndef BASLER_EXPOSURETIME_WIDGET_H
#define BASLER_EXPOSURETIME_WIDGET_H

#include <QWidget>

class BaslerCameraThread; // 前向声明

namespace Ui {
class BaslerExposuretimeWidget;
}

class BaslerExposuretimeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BaslerExposuretimeWidget(BaslerCameraThread *cameraThread, QWidget *parent = nullptr);
    ~BaslerExposuretimeWidget() override;

signals:
    // 曝光时间变化信号，发送给相机控制逻辑
    void exposureTimeSet(int value);

private slots:
    // 按钮点击槽函数，触发曝光时间设置
    void onSetExposureClicked();
    // 接收相机曝光参数更新
    void onExposureParamsUpdated(int min, int max, int current);

private:
    Ui::BaslerExposuretimeWidget *ui;
    BaslerCameraThread *m_baslerCamera = nullptr;
};

#endif // BASLER_EXPOSURETIME_WIDGET_H
