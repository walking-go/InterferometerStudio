#include "basler_exposuretime_widget.h"
#include "ui_basler_camera_exposuretime.h"
#include "basler_camera_thread.h"

BaslerExposuretimeWidget::BaslerExposuretimeWidget(BaslerCameraThread *cameraThread, QWidget *parent)
    : QWidget(parent), ui(new Ui::BaslerExposuretimeWidget), m_baslerCamera(cameraThread)
{
    ui->setupUi(this);

    // 连接按钮点击信号与槽函数
    connect(ui->setExposureButton, &QPushButton::clicked, this, &BaslerExposuretimeWidget::onSetExposureClicked);

    // 建立与相机线程的信号槽连接
    if (m_baslerCamera) {
        // 控件 -> 相机：设置曝光时间
        connect(this, &BaslerExposuretimeWidget::exposureTimeSet,
                m_baslerCamera, &BaslerCameraThread::setExposureTime,
                Qt::QueuedConnection);
        // 相机 -> 控件：更新曝光参数范围和当前值
        connect(m_baslerCamera, &BaslerCameraThread::exposureParamsUpdated,
                this, &BaslerExposuretimeWidget::onExposureParamsUpdated,
                Qt::QueuedConnection);
    }
}

BaslerExposuretimeWidget::~BaslerExposuretimeWidget()
{
    delete ui;
}

void BaslerExposuretimeWidget::onSetExposureClicked()
{
    int exposureValue = ui->exposureSpinBox->value();
    emit exposureTimeSet(exposureValue);
}

void BaslerExposuretimeWidget::onExposureParamsUpdated(int min, int max, int current)
{
    ui->exposureSpinBox->setRange(min, max);
    ui->exposureSpinBox->setValue(current);
}
