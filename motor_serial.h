#ifndef MOTOR_SERIAL_H
#define MOTOR_SERIAL_H

#include <QObject>           // Qt对象基类，支持信号槽
#include <QSerialPort>       // Qt串口通信核心类
#include <QSerialPortInfo>   // 用于获取串口设备信息
#include <QThread>           // 线程支持
#include <QQueue>            // 队列容器，用于缓存接收数据
#include <QMutex>            // 互斥锁，用于多线程数据同步
#include <QByteArray>        // 字节数组，用于数据存储
#include <QList>             // 列表容器


/**
 * @brief 自动连接特定USB转串口设备的类（支持十六进制数据处理）
 * 功能：自动识别指定VID/PID的设备，处理串口数据收发（以十六进制形式），并提供连接状态通知
 */
class MotorSerial : public QObject
{
    Q_OBJECT  // Qt元对象宏，使类支持信号槽机制

public:
    /**
     * @brief 构造函数
     * @param vid 目标设备的Vendor ID（默认0x0483，常见于ST设备）
     * @param pid 目标设备的Product ID（默认0x5740）
     * @param parent 父对象，用于Qt对象树管理
     * @param baudrate 串口波特率（默认9600）
     * @param timeout 超时时间（单位未明确，可能用于连接或接收）
     * @param receive_len 单次接收数据长度（默认8字节）
     */
    explicit MotorSerial(quint16 vid = 0x0483, quint16 pid = 0x5740,
                                      QObject *parent = nullptr, int baudrate = 9600,
                                      int timeout = 1, int receive_len = 8);

    ~MotorSerial();  // 析构函数，用于资源释放

    bool openPort(const QString &com = QString());         //打开串口
    void closePort();                                      // 关闭串口
    bool isConnected() const { return m_isConnected; }     // 获取当前连接状态
    void sendBytes(const QByteArray &byteData);            //发送字节数据
    void sendUint8Array(const QList<quint8> &uint8Array);  //发送无符号8位整数列表,转为字节发送
    QList<QString> getHexData();                           //获取接收的十六进制数据列表

signals:
    void dataReceived(const QString &hexStr);      // 收到数据时发射，携带十六进制字符串
    void connectionStatusChanged(bool connected);  // 连接状态变化时发射，携带新状态

private slots:
    void onReadyRead();  // 串口有数据可读时的处理槽函数（接收数据）

private:

    QList<QString> findUsbVcpDevice();             //查找匹配VID/PID的USB虚拟串口设备
    QString byteArrayToHex(const QByteArray &data);//将字节数组转换为十六进制字符串

    // 成员变量
    quint16 m_vid;          // 设备Vendor ID
    quint16 m_pid;          // 设备Product ID
    int m_baudrate;         // 波特率
    int m_timeout;          // 超时时间
    int m_receiveLen;       // 单次接收数据长度
    bool m_isConnected;     // 连接状态标志

    QSerialPort *m_serialPort;  // 串口对象指针，用于实际串口操作
    QQueue<QString> m_recvQueue;// 接收数据队列，缓存收到的十六进制字符串
    QMutex m_mutex;          // 互斥锁，保护队列操作的线程安全
    QByteArray m_receiveBuffer; // 接收缓冲区，用于累积不完整的数据包
};

#endif // SERIAL_UTILS_H
