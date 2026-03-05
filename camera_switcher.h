#ifndef CAMERA_SWITCHER_H
#define CAMERA_SWITCHER_H

#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include "small_camera_thread.h"
#include "utils/img_viewer.h"

class CameraSwitcher : public QWidget
{
    Q_OBJECT
public:
    explicit CameraSwitcher(QWidget *parent = nullptr);
    ~CameraSwitcher() override;

    // 接收相机帧（外部传入，不自己获取）
    void setMainCameraFrame(const QImage& frame);
    void setSubCameraFrame(const QImage& frame);

    // 触发主副图切换（外部控制）
    void switchMainSub();
    // 显示错误信息（外部通知）
    void showError(const QString& msg);

signals:

    void switchButtonClicked(); // 切换按钮被点击
    void subImageClicked();     // 副图被点击
private:
    // UI控件
    ImgViewer* mainViewer;    // 主图（支持缩放、移动）
    QLabel* subLabel;         // 副图（右下角小图）
    QToolButton* switchButton;// 切换按钮

    QImage m_lastCameraFrame;// 最新相机帧（外部传入）
    bool m_isMainCamera = true;

    // 工具函数
    void setupUI();
    void updateFrames();

    QImage m_MaincameraFrame; // 相机1的帧
    QImage m_SubcameraFrame; // 相机2的帧



private slots:

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override; // 小图点击检测
};

#endif // CAMERA_SWITCHER_H
