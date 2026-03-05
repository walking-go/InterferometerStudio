#include "motor_serial.h"
#include <QDebug>

MotorSerial::MotorSerial(quint16 vid, quint16 pid,
                                                   QObject *parent, int baudrate,
                                                   int timeout, int receive_len)
    : QObject(parent),  // 初始化父类QObject
    m_vid(vid),       // 赋值目标设备VID（厂商ID）
    m_pid(pid),       // 赋值目标设备PID（产品ID）
    m_baudrate(baudrate),  // 赋值串口波特率
    m_timeout(timeout),    // 赋值超时时间（用于发送等待）
    m_receiveLen(receive_len),  // 赋值单次接收数据长度
    m_isConnected(false)   // 初始化连接状态为“未连接”
{
    m_serialPort = new QSerialPort(this);  // 创建QSerialPort串口对象（父对象为当前类，自动管理内存）

    connect(m_serialPort, &QSerialPort::readyRead,
            this, &MotorSerial::onReadyRead); // 绑定串口readyRead信号到接收处理s槽函数onReadyRead
}


MotorSerial::~MotorSerial()
{
    closePort();          // 关闭串口（避免资源泄漏）
    // delete m_serialPort;  // 释放串口对象内存
    m_serialPort->close();
}

// 打开串口：支持自动查找设备或指定串口号
bool MotorSerial::openPort(const QString &com)
{
    if (m_isConnected) return true;  // 已连接则直接返回成功

    QString portName;  // 目标串口号
    // 1. 若未指定串口号，自动查找匹配VID/PID的USB串口设备
    if (com.isEmpty()) {
        QList<QString> ports = findUsbVcpDevice();  // 调用设备查找函数
        if (ports.isEmpty()) {  // 未找到匹配设备
            qDebug() << "[串口] 没有找到匹配的USB VCP设备";
            return false;
        }
        // 找到多个设备时输出日志提示
        if (ports.size() > 1) {
            qDebug() << "[串口] 发现多个符合条件的设备:";
            for (const auto &port : ports) {
                qDebug() << "[串口] 设备:" << port;
            }
        }
        portName = ports.first();  // 默认选择第一个找到的设备
    } else {
        // 2. 若指定了串口号，直接使用
        portName = com;
    }

    // 配置串口参数（硬件通信协议）
    m_serialPort->setPortName(portName);    // 设置串口号
    m_serialPort->setBaudRate(m_baudrate);  // 设置波特率
    m_serialPort->setDataBits(QSerialPort::Data8);     // 8位数据位
    m_serialPort->setParity(QSerialPort::NoParity);    // 无校验位
    m_serialPort->setStopBits(QSerialPort::OneStop);   // 1位停止位
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);  // 无流控
    m_serialPort->setReadBufferSize(1024);  // 设置接收缓冲区大小（1024字节）

    // 尝试以“读写模式”打开串口
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_isConnected = true;  // 更新连接状态为“已连接”
        emit connectionStatusChanged(true);  // 发射“连接成功”信号
        return true;
    } else {
        // 打开失败，输出错误原因
        qDebug() << "[串口] 连接失败:" << m_serialPort->errorString();
        return false;
    }
}

// 关闭串口：释放串口资源并更新状态
void MotorSerial::closePort()
{
    // 仅当"已连接"且"串口已打开"时执行关闭
    if (m_isConnected && m_serialPort->isOpen()) {
        m_serialPort->close();          // 关闭串口
        m_isConnected = false;          // 更新连接状态为"未连接"
        m_receiveBuffer.clear();        // 清空接收缓冲区，确保重连时状态干净
        qDebug() << "[串口] 已断开";    // 输出断开日志
        emit connectionStatusChanged(false);  // 发射"连接断开"信号
    }
}

// 发送字节数组：串口发送（无锁，避免与 onReadyRead 死锁）
void MotorSerial::sendBytes(const QByteArray &byteData)
{
    // 注意：不在此处加锁，因为 waitForBytesWritten 可能处理事件队列，
    // 若此时 readyRead 触发 onReadyRead，会导致死锁
    if (m_isConnected && m_serialPort->isOpen()) {
        m_serialPort->write(byteData);  // 写入字节数据到串口
        // 等待数据发送完成（超时时间为m_timeout）
        m_serialPort->waitForBytesWritten(m_timeout);
    }
}

// 发送无符号8位整数列表：先转字节数组再调用sendBytes
void MotorSerial::sendUint8Array(const QList<quint8> &uint8Array)
{
    QByteArray data;  // 临时字节数组（用于转换）
    // 遍历uint8列表，将每个元素转为char后加入字节数组
    for (quint8 val : uint8Array) {
        data.append(static_cast<char>(val));
    }
    sendBytes(data);  // 调用字节发送函数
}

// 获取接收的十六进制数据：线程安全地读取接收队列
QList<QString> MotorSerial::getHexData()
{
    QMutexLocker locker(&m_mutex);  // 加互斥锁（保护队列操作）
    QList<QString> result;          // 存储结果的列表
    // 将接收队列中的所有数据出队，存入结果列表
    while (!m_recvQueue.isEmpty()) {
        result.append(m_recvQueue.dequeue());
    }
    return result;  // 返回所有接收的十六进制字符串
}

// 串口接收处理槽函数：读取并处理串口数据
void MotorSerial::onReadyRead()
{
    // 1. 一次性读取所有可用数据到缓冲区（非阻塞，避免信号风暴）
    m_receiveBuffer.append(m_serialPort->readAll());

    // 2. 循环处理所有完整的数据包（每包m_receiveLen字节）
    while (m_receiveBuffer.size() >= m_receiveLen) {
        // 取出一个完整数据包
        QByteArray data = m_receiveBuffer.left(m_receiveLen);
        m_receiveBuffer.remove(0, m_receiveLen);

        // 转为十六进制字符串
        QString hexStr = byteArrayToHex(data);
        // 注意：移除了队列操作和锁，因为队列从未被使用，且锁可能导致死锁
        emit dataReceived(hexStr);  // 发射"数据接收"信号（携带数据）
    }
    // 不足m_receiveLen字节的数据保留在缓冲区，等待后续数据到达
}

// 查找匹配VID/PID的USB串口设备：遍历所有可用端口
QList<QString> MotorSerial::findUsbVcpDevice()
{
    QList<QString> efficientPorts;  // 存储匹配的串口号列表
    // 遍历系统中所有可用的串口设备信息
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        if (info.vendorIdentifier() == m_vid && info.productIdentifier() == m_pid) {
            efficientPorts.append(info.portName());  // 加入匹配列表
        }
    }
    return efficientPorts;  // 返回所有匹配的串口号
}

// 字节数组转十六进制字符串：格式化为“两位大写十六进制+空格”
QString MotorSerial::byteArrayToHex(const QByteArray &data)
{
    QString hexStr;  // 存储结果字符串
    // 遍历字节数组，每个字节转为两位大写十六进制
    for (char c : data) {
        // %1：占位符；2：固定两位（不足补0）；16：十六进制；QChar('0')：补位字符；toUpper()：大写
        hexStr += QString("%1 ").arg(static_cast<quint8>(c), 2, 16, QChar('0')).toUpper();
    }
    return hexStr.trimmed();  // 去除末尾多余空格后返回
}
