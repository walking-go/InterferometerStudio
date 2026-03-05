#ifndef MOTOR_CONTROL_WIDGET_H
#define MOTOR_CONTROL_WIDGET_H

#include <QMainWindow>
#include "motor_controller.h"
#include <QDoubleSpinBox>
//QT_BEGIN_NAMESPACE
namespace Ui { class MotorControlWidget; }
//QT_END_NAMESPACE

class MotorControlWidget : public QWidget
{
    Q_OBJECT

public:
    MotorControlWidget(QWidget *parent = nullptr);
    ~MotorControlWidget();

    /**
     * @brief getMotorController 获取电机控制器指针
     * @return 电机控制器指针
     */
    MotorController* getMotorController() const { return m_motorController; }

private slots:
    void on_connectBtn_clicked();
    void on_disconnectBtn_clicked();
    void on_moveXBtn_clicked();
    void on_moveYBtn_clicked();
    void on_moveZBtn_clicked();
    void on_moveMBtn_clicked();
    void on_phaseShiftBtn_clicked();
    void updateAxisState(int axisId);
    void moveXYZBeforeMeasure();

    // X轴微调槽函数
    void on_xCoarseMinusBtn_clicked();
    void on_xFineMinusBtn_clicked();
    void on_xFinePlusBtn_clicked();
    void on_xCoarsePlusBtn_clicked();

    // Y轴微调槽函数
    void on_yCoarseMinusBtn_clicked();
    void on_yFineMinusBtn_clicked();
    void on_yFinePlusBtn_clicked();
    void on_yCoarsePlusBtn_clicked();

    // Z轴微调槽函数
    void on_zCoarseMinusBtn_clicked();
    void on_zFineMinusBtn_clicked();
    void on_zFinePlusBtn_clicked();
    void on_zCoarsePlusBtn_clicked();

    // M轴微调槽函数
    void on_mCoarseMinusBtn_clicked();
    void on_mFineMinusBtn_clicked();
    void on_mFinePlusBtn_clicked();
    void on_mCoarsePlusBtn_clicked();

private:
    Ui::MotorControlWidget *ui;
    MotorController *m_motorController;
    void moveAxis(QDoubleSpinBox *spinBox, const QString &axisName);
    void jogAxis(const QString &axisName, double distance);
};
#endif // MAINWINDOW_H
