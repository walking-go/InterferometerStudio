#include "motor_test_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>

MotorTestWidget::MotorTestWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox *group = new QGroupBox("偏摆轴测试 (临时)");
    mainLayout->addWidget(group);

    QVBoxLayout *groupLayout = new QVBoxLayout(group);
    groupLayout->setSpacing(8);

    // 第1行：单步 +1um / -1um
    QHBoxLayout *row1 = new QHBoxLayout();
    m_btnPlus = new QPushButton("+1μm");
    m_btnMinus = new QPushButton("-1μm");
    row1->addWidget(m_btnPlus);
    row1->addWidget(m_btnMinus);
    groupLayout->addLayout(row1);

    // 第2行：连续移动参数
    QGridLayout *grid = new QGridLayout();
    grid->addWidget(new QLabel("次数:"), 0, 0);
    m_spinCount = new QSpinBox();
    m_spinCount->setRange(1, 10000);
    m_spinCount->setValue(10);
    grid->addWidget(m_spinCount, 0, 1);

    grid->addWidget(new QLabel("方向:"), 0, 2);
    m_comboDir = new QComboBox();
    m_comboDir->addItem("+1μm", 1);
    m_comboDir->addItem("-1μm", -1);
    m_comboDir->addItem("+1/-1 循环", 0);
    grid->addWidget(m_comboDir, 0, 3);

    grid->addWidget(new QLabel("间隔(ms):"), 1, 0);
    m_spinInterval = new QSpinBox();
    m_spinInterval->setRange(50, 60000);
    m_spinInterval->setValue(500);
    m_spinInterval->setSingleStep(50);
    grid->addWidget(m_spinInterval, 1, 1);

    m_labelProgress = new QLabel("就绪");
    grid->addWidget(m_labelProgress, 1, 2, 1, 2);

    groupLayout->addLayout(grid);

    // 第3行：开始/停止
    QHBoxLayout *row3 = new QHBoxLayout();
    m_btnStart = new QPushButton("开始连续移动");
    m_btnStop = new QPushButton("停止");
    m_btnStop->setEnabled(false);
    row3->addWidget(m_btnStart);
    row3->addWidget(m_btnStop);
    groupLayout->addLayout(row3);

    // 定时器
    m_timer = new QTimer(this);

    // 信号连接
    connect(m_btnPlus, &QPushButton::clicked, this, &MotorTestWidget::onPlusOneClicked);
    connect(m_btnMinus, &QPushButton::clicked, this, &MotorTestWidget::onMinusOneClicked);
    connect(m_btnStart, &QPushButton::clicked, this, &MotorTestWidget::onStartClicked);
    connect(m_btnStop, &QPushButton::clicked, this, &MotorTestWidget::onStopClicked);
    connect(m_timer, &QTimer::timeout, this, &MotorTestWidget::onTimerTick);
}

void MotorTestWidget::setMotorController(MotorController *controller)
{
    m_controller = controller;
}

void MotorTestWidget::moveXByUm(double um)
{
    if (!m_controller) return;
    if (!m_controller->isConnected()) {
        QMessageBox::warning(this, "警告", "请先连接串口");
        return;
    }
    // X轴: steps = distance_mm / 0.025 * 51200
    double mm = um * 0.001;
    int steps = static_cast<int>(mm / 0.025 * 51200);
    m_controller->movedX(steps, "X");
}

void MotorTestWidget::onPlusOneClicked()
{
    moveXByUm(1.0);
}

void MotorTestWidget::onMinusOneClicked()
{
    moveXByUm(-1.0);
}

void MotorTestWidget::onStartClicked()
{
    if (!m_controller || !m_controller->isConnected()) {
        QMessageBox::warning(this, "警告", "请先连接串口");
        return;
    }

    m_totalSteps = m_spinCount->value();
    m_currentStep = 0;
    m_labelProgress->setText(QString("0 / %1").arg(m_totalSteps));

    // 锁定参数输入
    m_spinCount->setEnabled(false);
    m_comboDir->setEnabled(false);
    m_spinInterval->setEnabled(false);
    m_btnStart->setEnabled(false);
    m_btnPlus->setEnabled(false);
    m_btnMinus->setEnabled(false);
    m_btnStop->setEnabled(true);

    // 立即执行第一步，然后启动定时器
    onTimerTick();
    m_timer->start(m_spinInterval->value());
}

void MotorTestWidget::onStopClicked()
{
    m_timer->stop();
    m_labelProgress->setText(QString("已停止 (%1 / %2)").arg(m_currentStep).arg(m_totalSteps));

    m_spinCount->setEnabled(true);
    m_comboDir->setEnabled(true);
    m_spinInterval->setEnabled(true);
    m_btnStart->setEnabled(true);
    m_btnPlus->setEnabled(true);
    m_btnMinus->setEnabled(true);
    m_btnStop->setEnabled(false);
}

void MotorTestWidget::onTimerTick()
{
    if (m_currentStep >= m_totalSteps) {
        onStopClicked();
        m_labelProgress->setText(QString("完成 (%1 / %2)").arg(m_totalSteps).arg(m_totalSteps));
        return;
    }

    int mode = m_comboDir->currentData().toInt();
    int dir = (mode != 0) ? mode : ((m_currentStep % 2 == 0) ? 1 : -1);
    moveXByUm(dir * 1.0);
    m_currentStep++;
    m_labelProgress->setText(QString("%1 / %2").arg(m_currentStep).arg(m_totalSteps));
}
