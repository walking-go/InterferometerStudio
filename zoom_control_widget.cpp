#include "zoom_control_widget.h"
#include "ui_zoom_control.h"
#include "zoom_controller.h"
#include <QMessageBox>
#include <QSerialPortInfo>

// ZoomControlWidget::ZoomControlWidget(ZoomController *zoomLogic, QWidget *parent) :
//     QWidget(parent),
//     ui(new Ui::ZoomControlWidget),
//     m_zoomLogic(zoomLogic)
// {
//     ui->setupUi(this);
//     initSerialPortComboBox();

//     // this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//     // this->layout()->setSizeConstraint(QLayout::SetDefaultConstraint);

//     // ui->lab_serialStatus->setText("串口状态：未连接");
//     // ui->lab_currentZoom->setText("当前倍率：未获取");
//     // 建立与业务逻辑层的信号槽连接
//     if (m_zoomController) {
//         connectSignals();
//     }
// }
ZoomControlWidget::ZoomControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ZoomControlWidget)
{
    m_zoomController= new ZoomController(this);
    ui->setupUi(this);
    initSerialPortComboBox();

    // this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // this->layout()->setSizeConstraint(QLayout::SetDefaultConstraint);

    // ui->lab_serialStatus->setText("串口状态：未连接");
    // ui->lab_currentZoom->setText("当前倍率：未获取");
    // 建立与业务逻辑层的信号槽连接
    if (m_zoomController) {
        connectSignals();
    }
}
ZoomControlWidget::~ZoomControlWidget()
{
    delete ui;
}
void ZoomControlWidget::connectSignals()
{
    // 1. 连接/断开串口按钮
    connect(ui->btn_connect, &QPushButton::clicked, this, [this]() {
        if (m_zoomController->isSerialOpen()) {
            // 已连接，执行断开
            m_zoomController->closeSerial();
        } else {
            // 未连接，执行连接
            QString portName = getSelectedSerialPort();
            if (portName.isEmpty()) {
                showError("请选择可用串口！");
                return;
            }
            // 验证串口是否存在
            bool portExists = false;
            for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
                if (info.portName() == portName) {
                    portExists = true;
                    break;
                }
            }
            if (!portExists) {
                showError("串口 " + portName + " 不存在，请检查连接！");
                return;
            }
            m_zoomController->setSerialPort(portName);
            m_zoomController->openSerial();
        }
    });

    // 2. UI按钮→业务层：固定倍率
    connect(this, &ZoomControlWidget::fixedZoomClicked,
            m_zoomController, &ZoomController::FixedZoom);
    // 3. UI按钮→业务层：普通变倍（开始/停止）
    connect(this, &ZoomControlWidget::zoomStart,
            m_zoomController, &ZoomController::StartZoom);
    connect(this, &ZoomControlWidget::zoomStop,
            m_zoomController, &ZoomController::StopZoom);
    // 4. UI按钮→业务层：微调（开始/停止）
    connect(this, &ZoomControlWidget::fineTuneStart,
            m_zoomController, &ZoomController::StartFineTune);
    connect(this, &ZoomControlWidget::fineTuneStop,
            m_zoomController, &ZoomController::StopFineTune);

    // 5. 业务层→UI：更新显示（逻辑层的信号反馈给UI）
    connect(m_zoomController, &ZoomController::zoomUpdated,
            this, &ZoomControlWidget::updateZoomDisplay);
    connect(m_zoomController, &ZoomController::serialStatusChanged,
            this, &ZoomControlWidget::updateSerialStatus);
    connect(m_zoomController, &ZoomController::errorOccurred,
            this, &ZoomControlWidget::showError);
}

// 获取选中的串口名（如"COM1"）
QString ZoomControlWidget::getSelectedSerialPort() const
{
    if (ui->cbx_serialPort->currentText().isEmpty()) return "";
    // 提取下拉框中的端口名（格式："COM1 (USB Serial Port)"）
    return ui->cbx_serialPort->currentText().split(" ")[0];
}

// 接收业务逻辑信号：更新倍率显示
void ZoomControlWidget::updateZoomDisplay(QString currentZoom)
{
    ui->lab_currentZoom->setText("当前倍率：" + currentZoom);
}

// 接收业务逻辑信号：更新串口状态显示
void ZoomControlWidget::updateSerialStatus(QString status)
{
    bool isConnected = (status == "已连接");
    ui->btn_connect->setText(isConnected ? "断开串口" : "连接串口");
    ui->cbx_serialPort->setEnabled(!isConnected);
}

// 接收业务逻辑信号：显示错误弹窗
void ZoomControlWidget::showError(QString errorMsg)
{
    QMessageBox::warning(this, "警告", errorMsg);
}

// 初始化串口下拉框（扫描可用串口，文档2.1/2.2节）
void ZoomControlWidget::initSerialPortComboBox()
{
    ui->cbx_serialPort->clear();
    QList<QSerialPortInfo> portInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : portInfos) {
        // 显示格式："端口名 (设备描述)"，方便用户选择
        QString item = info.portName() + " (" + info.description() + ")";
        ui->cbx_serialPort->addItem(item);
    }
    // 无可用串口时提示
    if (portInfos.isEmpty()) {
        ui->cbx_serialPort->addItem("无可用串口（检查硬件连接）");
        ui->cbx_serialPort->setEnabled(false);
        ui->btn_connect->setEnabled(false);
    }
}

// 固定倍率按钮：发射对应指令码信号（文档定倍指令）
void ZoomControlWidget::on_btn_07x_clicked() { emit fixedZoomClicked(0x20); } // 0.7x=0x20
void ZoomControlWidget::on_btn_10x_clicked() { emit fixedZoomClicked(0x21); } // 1.0x=0x21
void ZoomControlWidget::on_btn_15x_clicked() { emit fixedZoomClicked(0x22); } // 1.5x=0x22
void ZoomControlWidget::on_btn_20x_clicked() { emit fixedZoomClicked(0x23); } // 2.0x=0x23
void ZoomControlWidget::on_btn_25x_clicked() { emit fixedZoomClicked(0x24); } // 2.5x=0x24
void ZoomControlWidget::on_btn_30x_clicked() { emit fixedZoomClicked(0x25); } // 3.0x=0x25
void ZoomControlWidget::on_btn_35x_clicked() { emit fixedZoomClicked(0x26); } // 3.5x=0x26
void ZoomControlWidget::on_btn_40x_clicked() { emit fixedZoomClicked(0x27); } // 4.0x=0x27
void ZoomControlWidget::on_btn_45x_clicked() { emit fixedZoomClicked(0x28); } // 4.5x=0x28

// 普通变倍按钮：按下发射开始信号，松开发射停止信号（文档变倍指令）
void ZoomControlWidget::on_btn_zoomOut_pressed() { emit zoomStart(0x06); } // 变倍-=0x06
void ZoomControlWidget::on_btn_zoomOut_released() { emit zoomStop(); }
void ZoomControlWidget::on_btn_zoomIn_pressed() { emit zoomStart(0x07); } // 变倍+=0x07
void ZoomControlWidget::on_btn_zoomIn_released() { emit zoomStop(); }

// 微调按钮：按下发射开始信号，松开发射停止信号（文档微调指令）
void ZoomControlWidget::on_btn_fineTuneIn_pressed() { emit fineTuneStart(0x08); } // 微调+=0x08
void ZoomControlWidget::on_btn_fineTuneIn_released() { emit fineTuneStop(); }
void ZoomControlWidget::on_btn_fineTuneInPlus_pressed() { emit fineTuneStart(0x05); } // 微调++=0x05
void ZoomControlWidget::on_btn_fineTuneInPlus_released() { emit fineTuneStop(); }

