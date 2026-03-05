#include "ZoomLens.h"
#include <QSerialPortInfo>

ZoomLens::ZoomLens(QObject *parent) : QObject(parent)
{
    // 初始化串口（按文档RS232参数：波特率9600、无校验等）
    m_serial = new QSerialPort(this);
    m_serial->setBaudRate(QSerialPort::Baud9600);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    // 连接串口接收信号（解析控制盒返回数据）
    connect(m_serial, &QSerialPort::readyRead, this, &ZoomLens::readSerialData);
    // 连接串口错误信号
    connect(m_serial, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) {
            emit errorOccurred("串口错误：" + m_serial->errorString());
        }
    });

    // 初始化普通变倍定时器（间隔100ms，文档连续变倍频率）
    m_zoomTimer = new QTimer(this);
    m_zoomTimer->setInterval(100);
    m_zoomTimer->setSingleShot(false);
    connect(m_zoomTimer, &QTimer::timeout, this, [this]() {
        sendControlCmd(m_currentZoomCmd);
    });

    // 初始化微调定时器（间隔100ms，文档微调频率）
    m_fineTuneTimer = new QTimer(this);
    m_fineTuneTimer->setInterval(100);
    m_fineTuneTimer->setSingleShot(false);
    connect(m_fineTuneTimer, &QTimer::timeout, this, [this]() {
        sendControlCmd(m_currentFineTuneCmd);
    });
}

ZoomLens::~ZoomLens()
{
    closeSerial();
    m_zoomTimer->stop();
    m_fineTuneTimer->stop();
    delete m_zoomTimer;
    delete m_fineTuneTimer;
    delete m_serial;
}

// 串口配置：设置串口名
void ZoomLens::setSerialPort(const QString &portName)
{
    m_serial->setPortName(portName);
}

// 打开串口
bool ZoomLens::openSerial()
{
    if (m_serial->open(QIODevice::ReadWrite)) {
        emit serialStatusChanged("已连接");
        // 发送文档注13的“读信息”指令（0x65），获取当前倍率
        sendControlCmd(0x65);
        return true;
    } else {
        emit serialStatusChanged("未连接");
        emit errorOccurred("串口打开失败：" + m_serial->errorString());
        return false;
    }
}

// 关闭串口
void ZoomLens::closeSerial()
{
    if (m_serial->isOpen()) {
        m_serial->close();
        emit serialStatusChanged("未连接");
    }
}

// 判断串口是否打开
bool ZoomLens::isSerialOpen() const
{
    return m_serial->isOpen();
}

// 接收界面信号：执行固定倍率切换（文档定倍指令）
void ZoomLens::FixedZoom(quint8 cmd)
{
    if (!m_serial->isOpen()) {
        emit errorOccurred("串口未连接，无法切换倍率！");
        return;
    }
    sendControlCmd(cmd); // 发送对应定倍指令（如0x21=1.0x）
}

// 接收界面信号：开始普通变倍（文档变倍指令）
void ZoomLens::StartZoom(quint8 cmd)
{
    if (!m_serial->isOpen()) {
        emit errorOccurred("串口未连接，无法变倍！");
        return;
    }
    m_currentZoomCmd = cmd;
    m_zoomTimer->start();
}

// 接收界面信号：停止普通变倍（文档停止指令0x0A）
void ZoomLens::StopZoom()
{
    m_zoomTimer->stop();
    sendControlCmd(0x0A); // 发送停止指令
}

// 接收界面信号：开始微调（文档微调指令）
void ZoomLens::StartFineTune(quint8 cmd)
{
    if (!m_serial->isOpen()) {
        emit errorOccurred("串口未连接，无法微调！");
        return;
    }
    m_currentFineTuneCmd = cmd;
    m_fineTuneTimer->start();
}

// 接收界面信号：停止微调（文档停止指令0x0A）
void ZoomLens::StopFineTune()
{
    m_fineTuneTimer->stop();
    sendControlCmd(0x0A); // 发送停止指令
}



void ZoomLens::readSerialData()
{
    // 1. 把新收到的所有数据追加到缓存（关键：解决分包）
    m_serialBuffer.append(m_serial->readAll());
    qDebug() << "当前缓存数据（十六进制）：" << m_serialBuffer.toHex().toUpper();

    // 2. 循环解析：只要缓存长度≥5，就持续处理（支持连续多组5字节数据）
    while (m_serialBuffer.length() >= 5) {
        // 2.1 提取缓存中前5字节作为完整数据解析
        QByteArray completeData = m_serialBuffer.left(5);
        // 2.2 从缓存中删除已提取的5字节（避免重复解析）
        m_serialBuffer = m_serialBuffer.mid(5);

        // 3. 按文档格式校验数据（头码0xFF + 指令码0x5A）
        if (static_cast<unsigned char>(completeData.at(0)) != 0xFF) {
            qDebug() << "头码错误，丢弃数据：" << completeData.toHex().toUpper();
            continue; // 头码不对，跳过当前5字节
        }
        quint8 cmd = static_cast<unsigned char>(completeData.at(1));
        qDebug() << "接收到指令码：" << QString::number(cmd, 16).toUpper();

        // 4. 仅解析“返回编码器位置”的指令（0x5A，文档定义）
        if (cmd == 0x5A) {
            // 4.1 解析3字节位置（高位在前，文档注7）
            quint32 position = (static_cast<unsigned char>(completeData.at(2)) << 16) |
                               (static_cast<unsigned char>(completeData.at(3)) << 8) |
                               static_cast<unsigned char>(completeData.at(4));
            qDebug() << "解析出编码器位置（十进制）：" << position;

            // 4.2 转换为倍率并发射信号（触发label更新）
            QString zoom = positionToZoom(position);
            emit zoomUpdated(zoom); // 此信号必须发射，label才会变
        } else {
            qDebug() << "非位置数据，忽略（指令码：" << QString::number(cmd, 16) << "）";
        }
    }
}


// 发送文档规定的7字节控制指令（头码+指令+4参数+校验和）
void ZoomLens::sendControlCmd(quint8 cmd, QByteArray params)
{
    QByteArray cmdData;
    cmdData.append(0xFF); // 字节1：头码（文档固定0xFF）
    cmdData.append(cmd);  // 字节2：指令码（如0x21=1.0x）

    // 字节3-6：参数（不足4字节补0x00，文档允许任意值）
    for (int i = 0; i < 4; i++) {
        cmdData.append(i < params.length() ? params.at(i) : 0x00);
    }

    // 字节7：校验和（字节2-6之和 & 0xFF，文档协议）
    quint8 checkSum = 0;
    for (int i = 1; i < 6; i++) {
        checkSum += static_cast<quint8>(cmdData.at(i));
    }
    cmdData.append(checkSum & 0xFF);

    // 发送指令
    qint64 bytesWritten = m_serial->write(cmdData);
    if (bytesWritten != 7) {
        emit errorOccurred("指令发送失败，仅发送" + QString::number(bytesWritten) + "字节");
    }
}

// 根据编码器位置转成倍率（严格匹配文档注7的位置对应表）
QString ZoomLens::positionToZoom(quint32 position)
{

    position -= 100;
    if (position <= 8436) {          // 0.7x：8436
        return "0.7x";
    } else if (position <= 40763) {  // 1.0x：40763
        return "1.0x";
    } else if (position <= 77028) {  // 1.5x：77028
        return "1.5x";
    } else if (position <= 104537) { // 2.0x：104537
        return "2.0x";
    } else if (position <= 125201) { // 2.5x：125201
        return "2.5x";
    } else if (position <= 142441) { // 3.0x：142441
        return "3.0x";
    } else if (position <= 155335) { // 3.5x：155335
        return "3.5x";
    } else if (position <= 168623) { // 4.0x：168623
        return "4.0x";
    } else if (position <= 178292) { // 4.5x：178292
        return "4.5x";
    } else {
        return"超出范围";
    }

}
