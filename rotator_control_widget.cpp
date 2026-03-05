#include "rotator_control_widget.h"
#include "ui_rotator_control.h"
#include <QDateTime>
#include <QMessageBox>
#include <QTimer>

RotatorControlWidget::RotatorControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RotatorControlWidget),
    m_rotatorController(new RotatorController(this)),
    m_statusRefreshTimer(new QTimer(this))
{
    ui->setupUi(this);
    initUI();
    connectSignals();
}

RotatorControlWidget::~RotatorControlWidget()
{
    delete ui;
}

// -------------------------- UI 初始化 --------------------------
void RotatorControlWidget::initUI()
{
    setWindowTitle("FAULHABER 电机控制（USB）");
    // 初始禁用按钮（仅连接后启用）
    ui->disconnectBtn->setEnabled(false);
    ui->setSpeedBtn->setEnabled(false);
    ui->startBtn->setEnabled(false);
    ui->stopBtn->setEnabled(false);
    ui->emergencyStopBtn->setEnabled(false);
    // 转速输入框范围（与业务层一致）
    ui->targetSpeedSpin->setRange(0, 6000);
    ui->targetSpeedSpin->setSingleStep(10);
    // 实际转速框只读
    ui->actualSpeedEdit->setReadOnly(true);
    ui->actualSpeedEdit->setText("0 RPM");
    // 日志框只读
    ui->logTextEdit->setReadOnly(true);
    // 状态刷新定时器（100ms/次，平衡实时性与性能）
    m_statusRefreshTimer->setInterval(100);
}

// -------------------------- 信号连接（界面-业务解耦核心） --------------------------
void RotatorControlWidget::connectSignals()
{
    // UI 按钮 → 业务层接口
    connect(ui->connectBtn, &QPushButton::clicked, this, &RotatorControlWidget::onConnectBtnClicked);
    connect(ui->disconnectBtn, &QPushButton::clicked, this, &RotatorControlWidget::onDisconnectBtnClicked);
    connect(ui->setSpeedBtn, &QPushButton::clicked, this, &RotatorControlWidget::onSetSpeedBtnClicked);
    connect(ui->startBtn, &QPushButton::clicked, this, &RotatorControlWidget::onStartBtnClicked);
    connect(ui->stopBtn, &QPushButton::clicked, this, &RotatorControlWidget::onStopBtnClicked);
    connect(ui->emergencyStopBtn, &QPushButton::clicked, this, &RotatorControlWidget::onEmergencyStopBtnClicked);
    // 状态刷新定时器
    connect(m_statusRefreshTimer, &QTimer::timeout, this, &RotatorControlWidget::onStatusRefreshTimerTimeout);
    // 业务层信号 → UI 反馈
    connect(m_rotatorController, &RotatorController::usbConnected, this, &RotatorControlWidget::onUSBConnected);
    connect(m_rotatorController, &RotatorController::motorStateChanged, this, &RotatorControlWidget::onMotorStateChanged);
    connect(m_rotatorController, &RotatorController::errorOccurred, this, &RotatorControlWidget::onErrorOccurred);
}

// -------------------------- UI 交互槽函数 --------------------------
void RotatorControlWidget::onConnectBtnClicked()
{
    // 枚举并填充 USB 端口（调用业务层接口）
    QStringList ports = m_rotatorController->getAvailableUSBPorts();
    if (ports.isEmpty()) {
        addLog("无可用 USB 端口", true);
        return;
    }
    ui->usbPortCombo->clear();
    ui->usbPortCombo->addItems(ports);

    // 获取选中端口（转换为业务层需要的端口号："USB Port 1" → 1）
    QString selectedPortText = ui->usbPortCombo->currentText();
    int port = selectedPortText.split(" ")[2].toInt();
    if (!m_rotatorController->connectUSB(port)) {
        addLog("USB 连接失败", true);
        return;
    }
}

void RotatorControlWidget::onDisconnectBtnClicked()
{
    m_rotatorController->disconnectUSB();
    m_statusRefreshTimer->stop();
    ui->actualSpeedEdit->setText("0 RPM");
}

void RotatorControlWidget::onSetSpeedBtnClicked()
{
    int targetSpeed = ui->targetSpeedSpin->value();
    if (m_rotatorController->setTargetSpeed(targetSpeed)) {
        addLog(QString("目标转速已设置：%1 RPM").arg(targetSpeed));

    }
}

void RotatorControlWidget::onStartBtnClicked()
{
    if (m_rotatorController->startMotor()) {
        addLog("电机启动成功");
        m_statusRefreshTimer->start(); // 启动状态刷新

    }
}

void RotatorControlWidget::onStopBtnClicked()
{
    if (m_rotatorController->stopMotor()) {
        addLog("电机停止成功");
        m_statusRefreshTimer->stop();
        ui->actualSpeedEdit->setText("0 RPM");
    }
}

void RotatorControlWidget::onEmergencyStopBtnClicked()
{
    addLog("触发急停", true);
    m_rotatorController->stopMotor();
    m_rotatorController->disconnectUSB();
    m_statusRefreshTimer->stop();
    ui->actualSpeedEdit->setText("0 RPM");
}

// -------------------------- 状态刷新与日志 --------------------------
void RotatorControlWidget::onStatusRefreshTimerTimeout()
{
    // 读取实际转速（调用业务层接口）
    int actualSpeed = m_rotatorController->getCurrentSpeed();
    ui->actualSpeedEdit->setText(QString("%1 RPM").arg(actualSpeed));
}

void RotatorControlWidget::addLog(const QString& logText, bool isError)
{
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString log = QString("[%1] %2").arg(timeStr).arg(logText);
    // 错误日志标红
    if (isError) {
        ui->logTextEdit->append(QString("<font color='red'>%1</font>").arg(log));
    } else {
        ui->logTextEdit->append(log);
    }
    // 滚动到最新行
    QTextCursor cursor = ui->logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->logTextEdit->setTextCursor(cursor);
}

// -------------------------- 业务层信号反馈 --------------------------
void RotatorControlWidget::onUSBConnected(bool success, const QString& msg)
{
    if (success) {
        ui->connectStatusLabel->setText(msg);
        ui->connectStatusLabel->setStyleSheet("color: green;");
        ui->connectBtn->setEnabled(false);
        ui->disconnectBtn->setEnabled(true);
        ui->setSpeedBtn->setEnabled(true);
        ui->startBtn->setEnabled(true);
        ui->emergencyStopBtn->setEnabled(true);
        addLog(msg);
    } else {
        ui->connectStatusLabel->setText(msg);
        ui->connectStatusLabel->setStyleSheet("color: red;");
        addLog(msg, true);
    }
}

void RotatorControlWidget::onMotorStateChanged(const QString& state)
{
    ui->motorStateLabel->setText(QString("电机状态：%1").arg(state));
    // 状态颜色区分
    if (state == "运行中") {
        ui->motorStateLabel->setStyleSheet("color: blue;");
        // 电机运行时，启用停止按钮
        ui->stopBtn->setEnabled(true);
    } else if (state == "故障") {
        ui->motorStateLabel->setStyleSheet("color: red;");
        // 故障时，禁用停止按钮（此时应先处理故障）
        ui->stopBtn->setEnabled(false);
    } else {
        ui->motorStateLabel->setStyleSheet("color: black;");
        ui->stopBtn->setEnabled(false);
    }
}

void RotatorControlWidget::onErrorOccurred(const QString& errorMsg)
{
    addLog(errorMsg, true);
    QMessageBox::warning(this, "错误", errorMsg);
}
