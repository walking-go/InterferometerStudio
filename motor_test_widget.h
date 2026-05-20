#ifndef MOTOR_TEST_WIDGET_H
#define MOTOR_TEST_WIDGET_H

// 临时测试模块：偏摆轴（X轴）微步测试
// 测试完成后可直接删除本文件及 motor_test_widget.cpp，
// 并从 main_window.cpp 和 .pro 中移除相关引用即可。

#include <QWidget>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QTimer>
#include "motor_controller.h"

class MotorTestWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MotorTestWidget(QWidget *parent = nullptr);
    void setMotorController(MotorController *controller);

private slots:
    void onPlusOneClicked();
    void onMinusOneClicked();
    void onStartClicked();
    void onStopClicked();
    void onTimerTick();

private:
    void moveXByUm(double um);

    MotorController *m_controller = nullptr;
    QTimer *m_timer = nullptr;

    // UI
    QPushButton *m_btnPlus = nullptr;
    QPushButton *m_btnMinus = nullptr;
    QSpinBox *m_spinCount = nullptr;
    QComboBox *m_comboDir = nullptr;
    QSpinBox *m_spinInterval = nullptr;
    QPushButton *m_btnStart = nullptr;
    QPushButton *m_btnStop = nullptr;
    QLabel *m_labelProgress = nullptr;

    // 运行状态
    int m_totalSteps = 0;
    int m_currentStep = 0;
};

#endif // MOTOR_TEST_WIDGET_H
