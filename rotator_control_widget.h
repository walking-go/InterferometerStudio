#ifndef ROTATOR_CONTROL_WIDGET_H
#define ROTATOR_CONTROL_WIDGET_H

#include <QWidget>
#include "rotator_controller.h"
namespace Ui {
class RotatorControlWidget;
}

class RotatorControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RotatorControlWidget(QWidget *parent = nullptr);
    ~RotatorControlWidget();

    // 对外暴露业务层接口（方便外部系统调用）
    RotatorController* getMotorService() const { return m_rotatorController; }

private slots:
    // UI 交互触发的槽函数
    void onConnectBtnClicked();          // 连接 USB 按钮
    void onDisconnectBtnClicked();       // 断开 USB 按钮
    void onSetSpeedBtnClicked();         // 设置转速按钮
    void onStartBtnClicked();            // 启动电机按钮
    void onStopBtnClicked();             // 停止电机按钮
    void onEmergencyStopBtnClicked();    // 急停按钮
    void onStatusRefreshTimerTimeout();  // 定时刷新状态（转速、状态）

    // 声明 connectSignals 槽函数
    void connectSignals();

    // 订阅业务层信号的槽函数
    void onUSBConnected(bool success, const QString& msg); // USB 连接结果
    void onMotorStateChanged(const QString& state);        // 电机状态更新
    void onErrorOccurred(const QString& errorMsg);         // 错误信息更新

private:
    Ui::RotatorControlWidget *ui;
    RotatorController* m_rotatorController; // 业务层实例（组合关系，解耦继承）
    QTimer* m_statusRefreshTimer;        // 状态刷新定时器（100ms/次）

    // UI 辅助函数
    void initUI();                       // 初始化 UI 状态
    void addLog(const QString& logText, bool isError = false); // 添加操作日志
};

#endif // ROTATOR_CONTROL_WIDGET_H
