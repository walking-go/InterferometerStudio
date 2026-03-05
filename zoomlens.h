#ifndef ZOOMLENS_H
#define ZOOMLENS_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QByteArray>
#include <QDebug>

// 只负责变倍业务逻辑，完全脱离UI
class ZoomLens : public QObject
{
    Q_OBJECT
public:
    explicit ZoomLens(QObject *parent = nullptr);
    ~ZoomLens();

    // 给外部（如界面层）提供的串口配置接口
    void setSerialPort(const QString &portName); // 设置串口名
    bool openSerial();                           // 打开串口
    void closeSerial();                          // 关闭串口
    bool isSerialOpen() const;                   // 判断串口是否打开

signals:
    // 业务逻辑层向界面层发送的信号（如更新倍率、提示错误）
    void zoomUpdated(QString currentZoom);       // 倍率更新（给界面显示用）
    void serialStatusChanged(QString status);    // 串口状态变化（给界面显示用）
    void errorOccurred(QString errorMsg);        // 错误信息（给界面弹窗用）

public slots:
    // 接收界面层的信号，执行对应逻辑（按文档指令实现）
    // 1. 固定倍率（文档定倍指令：0x20=0.7x~0x28=4.5x）
    void FixedZoom(quint8 cmd);         // 接收“固定倍率按钮点击”信号
    // 2. 普通变倍（文档变倍指令：0x06=变倍-，0x07=变倍+）
    void StartZoom(quint8 cmd);                // 接收“变倍按钮按下”信号
    void StopZoom();                           // 接收“变倍按钮松开”信号
    // 3. 微调（文档微调指令：0x05=微调++，0x08=微调+）
    void StartFineTune(quint8 cmd);            // 接收“微调按钮按下”信号
    void StopFineTune();                       // 接收“微调按钮松开”信号
    // 4. 读取串口数据（解析控制盒返回的5字节数据，文档第二章第二节）
    void readSerialData();

private:
    // 核心函数：发送文档规定的7字节控制指令
    void sendControlCmd(quint8 cmd, QByteArray params = QByteArray());
    // 辅助函数：根据编码器位置转成倍率（文档注7的位置对应表）
    QString positionToZoom(quint32 position);

    QSerialPort *m_serial;       // 串口对象（文档串口通信核心）
    QTimer *m_zoomTimer;         // 普通变倍定时器（长按连续变倍）
    QTimer *m_fineTuneTimer;     // 微调定时器（长按连续微调）
    quint8 m_currentZoomCmd;     // 当前普通变倍指令（0x06/0x07）
    quint8 m_currentFineTuneCmd; // 当前微调指令（0x05/0x08）
    QByteArray m_serialBuffer; // 新增：缓存串口数据，解决分包问题
};

#endif // ZOOMLENS_H
