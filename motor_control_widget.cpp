#include "motor_control_widget.h"
#include "ui_motor_control.h"
#include <QMessageBox>
#include <QDebug>
#include <QLineEdit>
#include <QSpinBox>

MotorControlWidget::MotorControlWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::MotorControlWidget)
{
    ui->setupUi(this);
    m_motorController = new MotorController(this);

    connect(m_motorController, &MotorController::axisStateUpdated,
            this, &MotorControlWidget::updateAxisState);

    connect(m_motorController, &MotorController::axisLimitTriggered, this, [=](int axisId, quint8 state) {
        ui->statusLabel->setText(QString("轴%1 触发限位！状态：0x%2").arg(axisId).arg(state, 2, 16, QChar('0')));
    });


    //回车触发设置位置信号
    auto bindSpinBox = [&](QDoubleSpinBox *box, auto slotFunc){
        QLineEdit *edit = box->findChild<QLineEdit *>();
        if (edit) {
            connect(edit, &QLineEdit::returnPressed,
                    this, slotFunc);
        }
    };

    bindSpinBox(ui->xTargetSpinBox, &MotorControlWidget::on_moveXBtn_clicked);
    bindSpinBox(ui->yTargetSpinBox, &MotorControlWidget::on_moveYBtn_clicked);
    bindSpinBox(ui->zTargetSpinBox, &MotorControlWidget::on_moveZBtn_clicked);
    bindSpinBox(ui->mTargetSpinBox, &MotorControlWidget::on_moveMBtn_clicked);

    // 设置速度SpinBox为十六进制显示
    auto setupHexSpinBox = [](QSpinBox *spinBox) {
        spinBox->setDisplayIntegerBase(16);
        spinBox->setPrefix("0x");
        spinBox->setRange(0x000001, 0xFFFFFF);
    };
    setupHexSpinBox(ui->xSpeedSpinBox);
    setupHexSpinBox(ui->ySpeedSpinBox);
    setupHexSpinBox(ui->zSpeedSpinBox);
    setupHexSpinBox(ui->mSpeedSpinBox);

    // 速度设置连接：当SpinBox值改变时设置对应轴的速度
    connect(ui->xSpeedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) { m_motorController->setSpeed(value, "X"); });
    connect(ui->ySpeedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) { m_motorController->setSpeed(value, "Y"); });
    connect(ui->zSpeedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) { m_motorController->setSpeed(value, "Z"); });
    connect(ui->mSpeedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) { m_motorController->setSpeed(value, "M"); });

    // 初始化SpinBox的默认值为当前速度设置
    ui->xSpeedSpinBox->setValue(m_motorController->getSpeed("X"));
    ui->ySpeedSpinBox->setValue(m_motorController->getSpeed("Y"));
    ui->zSpeedSpinBox->setValue(m_motorController->getSpeed("Z"));
    ui->mSpeedSpinBox->setValue(m_motorController->getSpeed("M"));
}

MotorControlWidget::~MotorControlWidget()
{
    delete m_motorController;
    delete ui;
}

//连接串口
void MotorControlWidget::on_connectBtn_clicked()
{
    if (m_motorController->openPort()) {
        QMessageBox::information(this, "连接状态", "串口连接成功");
    } else {
        QMessageBox::critical(this, "连接状态", "串口连接失败");
    }
}

//断开串口
void MotorControlWidget::on_disconnectBtn_clicked()
{
    m_motorController->closePort();
    QMessageBox::information(this, "连接状态", "串口已断开");
}

//移动轴
void MotorControlWidget::moveAxis(QDoubleSpinBox *spinBox, const QString &axisName)
{

    if (!m_motorController->isConnected()) {
        QMessageBox::warning(this, "警告", "请先连接串口");
        return;
    }

    double value = spinBox->value();
    int target = int(value /0.025 * 51200);//  * 51200/0.025

    m_motorController->movedX(target, axisName);

}
void MotorControlWidget::on_moveXBtn_clicked()
{
    moveAxis(ui->xTargetSpinBox, "X");
    ui->xTargetSpinBox->setValue(0.0);
    qDebug()<<"x轴移动";

}

void MotorControlWidget::on_moveYBtn_clicked()
{
    moveAxis(ui->yTargetSpinBox, "Y");
    ui->yTargetSpinBox->setValue(0.0);

}

void MotorControlWidget::on_moveZBtn_clicked()
{
    moveAxis(ui->zTargetSpinBox, "Z");
    ui->zTargetSpinBox->setValue(0.0);

}

void MotorControlWidget::on_moveMBtn_clicked()
{

    if (!m_motorController->isConnected()) {
        QMessageBox::warning(this, "警告", "请先连接串口");
        return;
    }

    double mvalue = ui->mTargetSpinBox->value();
    int target = int(mvalue /4 * 51200);
    m_motorController->movedX(target, "M");
    ui->mTargetSpinBox->setValue(0.0);
}

// 相移按钮：同时移动X、Y、Z三轴
void MotorControlWidget::on_phaseShiftBtn_clicked()
{
    moveAxis(ui->xTargetSpinBox, "X");
    QThread::msleep(200);
    moveAxis(ui->yTargetSpinBox, "Y");
    QThread::msleep(200);
    moveAxis(ui->zTargetSpinBox, "Z");
}
//执行测量：同时移动X、Y、Z三轴各1微米
void MotorControlWidget::moveXYZBeforeMeasure()
{
    m_motorController->movedX(2048, "X");
    QThread::msleep(200);
    m_motorController->movedX(2048, "Y");
    QThread::msleep(200);
    m_motorController->movedX(2048, "Z");
}
//更新轴状态
void MotorControlWidget::updateAxisState(int axisId)
{
    switch (axisId) {
    case 1: {
        double x = m_motorController->readX("X") / 51200.0 *0.025; //使用51200.0强制浮点数
        ui->xPosLabel->setText(QString::number(x, 'f', 3));
        break;
    }
    case 2: {
        double y = m_motorController->readX("Y") / 51200.0 *0.025;
        ui->yPosLabel->setText(QString::number(y, 'f', 3));
        break;
    }
    case 3: {
        double m = m_motorController->readX("M") / 51200.0 *4;
        ui->mPosLabel->setText(QString::number(m, 'f', 3));
        break;
    }
    case 4: {
        double z = m_motorController->readX("Z") / 51200.0 *0.025;
        ui->zPosLabel->setText(QString::number(z, 'f', 3));
        break;
    }
    }
}

// 通用微调函数
void MotorControlWidget::jogAxis(const QString &axisName, double distance)
{
    if (!m_motorController->isConnected()) {
        QMessageBox::warning(this, "警告", "请先连接串口");
        return;
    }

    QString name = axisName.toUpper();
    int steps;

    // 根据不同轴使用不同的转换系数
    if (name == "M") {
        steps = int(distance / 4 * 51200);
    } else {
        // X、Y、Z 轴使用相同的转换系数
        steps = int(distance / 0.025 * 51200);
    }

    m_motorController->movedX(steps, name);
}

// X轴微调槽函数实现
void MotorControlWidget::on_xCoarseMinusBtn_clicked()
{
    jogAxis("X", -0.1);
}

void MotorControlWidget::on_xFineMinusBtn_clicked()
{
    jogAxis("X", -0.01);
}

void MotorControlWidget::on_xFinePlusBtn_clicked()
{
    jogAxis("X", 0.01);
}

void MotorControlWidget::on_xCoarsePlusBtn_clicked()
{
    jogAxis("X", 0.1);
}

// Y轴微调槽函数实现
void MotorControlWidget::on_yCoarseMinusBtn_clicked()
{
    jogAxis("Y", -0.1);
}

void MotorControlWidget::on_yFineMinusBtn_clicked()
{
    jogAxis("Y", -0.01);
}

void MotorControlWidget::on_yFinePlusBtn_clicked()
{
    jogAxis("Y", 0.01);
}

void MotorControlWidget::on_yCoarsePlusBtn_clicked()
{
    jogAxis("Y", 0.1);
}

// Z轴微调槽函数实现
void MotorControlWidget::on_zCoarseMinusBtn_clicked()
{
    jogAxis("Z", -0.1);
}

void MotorControlWidget::on_zFineMinusBtn_clicked()
{
    jogAxis("Z", -0.01);
}

void MotorControlWidget::on_zFinePlusBtn_clicked()
{
    jogAxis("Z", 0.01);
}

void MotorControlWidget::on_zCoarsePlusBtn_clicked()
{
    jogAxis("Z", 0.1);
}

// M轴微调槽函数实现
void MotorControlWidget::on_mCoarseMinusBtn_clicked()
{
    jogAxis("M", -0.1);
}

void MotorControlWidget::on_mFineMinusBtn_clicked()
{
    jogAxis("M", -0.01);
}

void MotorControlWidget::on_mFinePlusBtn_clicked()
{
    jogAxis("M", 0.01);
}

void MotorControlWidget::on_mCoarsePlusBtn_clicked()
{
    jogAxis("M", 0.1);
}


