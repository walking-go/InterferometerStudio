#include "motor_controller.h"
#include <QDebug>
#include <QThread>
#include <QCoreApplication> //处理事件循环

MotorController::MotorController(QObject *parent)
    : MotorSerial(0x0483, 0x5740, parent, 9600, 10, 8),  // 初始化父类：VID/PID(默认ST设备)、波特率9600、超时10、单次接收8字节
    m_ustepsPerCir(51200),  // 初始化每圈微步数为51200（电机相关参数）
    m_errorFlag(false)       // 初始化错误标志为false（无错误）
{
    // 创建X、Y、Z、M四个轴对象（参数：是否使能、轴ID、父对象）
    m_axisX = new Axis(true, 1, this);
    m_axisY = new Axis(true, 2, this);
    m_axisZ = new Axis(true, 4, this);
    m_axisM = new Axis(true, 3, this);  // M轴初始已经使能

    // 建立「轴ID -> 轴对象」的映射（方便通过ID快速查找轴）
    m_id2Axis.insert(0x01, m_axisX);
    m_id2Axis.insert(0x02, m_axisY);
    m_id2Axis.insert(0x03, m_axisM);
    m_id2Axis.insert(0x04, m_axisZ);

    // 建立「轴名称(大写) -> 轴对象」的映射（方便通过名称快速查找轴）
    m_name2Axis.insert("X", m_axisX);
    m_name2Axis.insert("Y", m_axisY);
    m_name2Axis.insert("Z", m_axisZ);
    m_name2Axis.insert("M", m_axisM);


    // 连接串口数据接收信号（来自父类）到本类的数据处理槽函数
    connect(this, &MotorSerial::dataReceived,
            this, &MotorController::processReceivedData);
}

// 读取指定轴的当前位置（通过轴名称）
int MotorController::readX(const QString &axisName)
{
    QMutexLocker locker(&m_axisMutex);  // 加锁保护Axis对象访问
    QString name = axisName.toUpper();  // 名称统一转为大写（避免大小写匹配问题）
    if (m_name2Axis.contains(name)) {   // 检查映射中是否存在该轴
        return m_name2Axis[name]->curPlace;  // 存在则返回轴的当前位置
    }
    return 0;  // 不存在则返回默认值0
}

// 读取指定轴的ID（通过轴名称）
int MotorController::readId(const QString &axisName)
{
    QMutexLocker locker(&m_axisMutex);  // 加锁保护Axis对象访问
    QString name = axisName.toUpper();  // 名称统一转为大写
    if (m_name2Axis.contains(name)) {   // 检查映射中是否存在该轴
        return m_name2Axis[name]->axisId;  // 存在则返回轴的ID
    }
    return -1;  // 不存在则返回无效ID(-1)
}

// 轴相对移动（基于当前位置移动dx距离）
void MotorController::movedX(int dx, const QString &axisName)
{
    moveToX(readX(axisName) + dx, axisName);
}

// 设置指定轴的移动速度
void MotorController::setSpeed(int speed, const QString &axisName)
{
    QMutexLocker locker(&m_axisMutex);
    QString name = axisName.toUpper();
    if (m_name2Axis.contains(name)) {
        // 限制速度范围（3字节最大值为0xFFFFFF）
        m_name2Axis[name]->speed = qBound(1, speed, 0xFFFFFF);
        }
}

// 获取指定轴的当前速度设置
int MotorController::getSpeed(const QString &axisName) const
{
    QMutexLocker locker(&const_cast<MotorController*>(this)->m_axisMutex);
    QString name = axisName.toUpper();
    if (m_name2Axis.contains(name)) {
        return m_name2Axis[name]->speed;
    }
    return 0x000258;  // 默认速度
}

// 轴绝对移动（移动到指定位置x）
void MotorController::moveToX(int x, const QString &axisName)
{
    QString name = axisName.toUpper();

    // 先获取轴ID和速度设置（带锁保护）
    int axisId;
    int axisSpeed;
    {
        QMutexLocker locker(&m_axisMutex);
        if (!m_name2Axis.contains(name)) return;  // 轴不存在则返回
        axisId = m_name2Axis[name]->axisId;
        axisSpeed = m_name2Axis[name]->speed;
    }

    // 构造「轴ID+命令」字节：ID左移4位 + 命令码0x01（绝对移动命令，与相对移动命令码相同，通过参数区分）
    QByteArray idSet;
    idSet.append(static_cast<char>((axisId << 4) | 0x01));

    // 构造速度设置字节（使用轴的速度设置，3字节大端格式）
    QByteArray vSet;
    vSet.append(static_cast<char>((axisSpeed >> 16) & 0xFF));
    vSet.append(static_cast<char>((axisSpeed >> 8) & 0xFF));
    vSet.append(static_cast<char>(axisSpeed & 0xFF));
    qDebug()<<"vSet____"<<vSet;

    // 将目标位置x（int32）转为字节数组
    QByteArray targetXByte = int32ToBytes(x);

    // 组装发送数据并转为quint8列表
    QList<quint8> sendData;
    for (char c : idSet + vSet + targetXByte) {
        sendData.append(static_cast<quint8>(c));
    }

    // 重置轴状态（运动中）- 带锁保护
    {
        QMutexLocker locker(&m_axisMutex);
        m_name2Axis[name]->staticFlag = false;
        m_name2Axis[name]->cnt = 0;
    }

    sendUint8Array(sendData);  // 发送数据

}

// 设置轴的运动
void MotorController::moveV(int v, const QString &axisName)
{
    QString name = axisName.toUpper();

    // 先获取轴ID（带锁保护）
    int axisId;
    {
        QMutexLocker locker(&m_axisMutex);
        if (!m_name2Axis.contains(name)) return;  // 轴不存在则返回
        axisId = m_name2Axis[name]->axisId;
    }

    // 将速度值（绝对值）转为int32字节数组
    QByteArray vSetByte = int32ToBytes(qAbs(v));

    // 处理速度方向：v>=0（正方向）设最高位为1（0x80），v<0（负方向）设为0（0x00）
    if (v >= 0) {
        vSetByte[3] = static_cast<char>(0x80);
    } else {
        vSetByte[3] = static_cast<char>(0x00);
    }

    // 构造「轴ID+命令」字节：ID左移4位 + 命令码0x02（速度设置命令）
    QByteArray idSet;
    idSet.append(static_cast<char>((axisId << 4) | 0x02));

    // 构造位置占位字节（速度设置无需位置，填0）
    QByteArray xSet;
    xSet.append(static_cast<char>(0x00));
    xSet.append(static_cast<char>(0x00));
    xSet.append(static_cast<char>(0x00));

    // 组装发送数据（ID命令 + 速度 + 位置占位）
    QList<quint8> sendData;
    for (char c : idSet + vSetByte + xSet) {
        sendData.append(static_cast<quint8>(c));
    }

    // 重置轴状态（运动中）- 带锁保护
    {
        QMutexLocker locker(&m_axisMutex);
        m_name2Axis[name]->staticFlag = false;
        m_name2Axis[name]->cnt = 0;
    }

    sendUint8Array(sendData);  // 发送数据
}

//停止指定轴的运动
void MotorController::stop(const QString &axisName)
{
    QString name = axisName.toUpper();

    // 先获取轴ID（带锁保护）
    int axisId;
    {
        QMutexLocker locker(&m_axisMutex);
        if (!m_name2Axis.contains(name)) return;  // 轴不存在则返回
        axisId = m_name2Axis[name]->axisId;
    }

    // 构造「轴ID+命令」字节：ID左移4位 + 命令码0x03（停止命令）
    QByteArray idSet;
    idSet.append(static_cast<char>((axisId << 4) | 0x03));

    // 组装发送数据：1字节命令 + 7字节0（凑满8字节接收长度）
    QList<quint8> sendData;
    sendData.append(static_cast<quint8>(idSet[0]));
    for (int i = 0; i < 7; ++i) {
        sendData.append(0x00);
    }

    // 重置轴状态（停止过程中）- 带锁保护
    {
        QMutexLocker locker(&m_axisMutex);
        m_name2Axis[name]->staticFlag = false;
        m_name2Axis[name]->cnt = 0;
    }

    sendUint8Array(sendData);  // 发送停止命令
}


// 处理串口接收的十六进制数据（解析设备反馈信息）
void MotorController::processReceivedData(const QString &hexStr)
{
    //qDebug() << "原始串口数据：" << hexStr;
    // 步骤1：将十六进制字符串（如"01 02 03"）解析为字节数组
    QByteArray data;
    QStringList hexParts = hexStr.split(" ");  // 按空格分割十六进制片段
    foreach (const QString &part, hexParts) {
        bool ok;
        quint8 val = part.toUInt(&ok, 16);  // 十六进制转无符号字节
        if (ok) data.append(static_cast<char>(val));  // 转换成功则加入字节数组
    }

    if (data.size() != 8) return;  // 接收数据必须为8字节（与硬件约定一致）

    // 步骤2：解析轴ID（第1字节高4位：右移4位后取低4位）
    int axisId = (static_cast<quint8>(data[0]) >> 4) & 0x0F;

    // 用于在锁外发射信号的临时变量
    bool shouldEmitLimit = false;
    quint8 limitStateToEmit = 0;

    // 加锁保护整个Axis对象的更新过程
    {
        QMutexLocker locker(&m_axisMutex);

        if (!m_id2Axis.contains(axisId)) return;
        Axis *axis = m_id2Axis[axisId];  // 获取当前轴对象

        // 步骤3：解析轴当前位置（第2-5字节，共4字节，转int32）
        axis->curPlace = bytesToInt32(data.mid(1, 4));

        // 步骤4：解析限位状态（若轴使能限位，取第6字节）
        if (axis->limitFlag) {
            axis->limitState = static_cast<quint8>(data[5]);
            if (axis->limitState != 0){
                // 记录需要发射的信号参数，在锁外发射
                shouldEmitLimit = true;
                limitStateToEmit = axis->limitState;
            }
        }

        // 步骤5：解析工作状态（第1字节低4位）
        axis->workingState = static_cast<quint8>(data[0]) & 0x0F;
        // 若工作状态为0x00（停止），计数累加；累计4次则标记轴为静态（稳定停止）
        if (axis->workingState == 0x00) {
            axis->cnt++;
            if (axis->cnt >= 4) {
                axis->staticFlag = true;
            }
        }
    }  // 锁在这里释放

    // 步骤6：发射信号（全部在锁外发射，避免死锁）
    if (shouldEmitLimit) {
        qDebug() << "限位 ---------------------------------------------";
        emit axisLimitTriggered(axisId, limitStateToEmit);
    }
    emit axisStateUpdated(axisId);
}

// 工具函数：将32位整数转为大端字节数组（高位字节在前）
QByteArray MotorController::int32ToBytes(int n)
{
    QByteArray bytes;
    // 按大端顺序拼接4个字节（24-31位、16-23位、8-15位、0-7位）
    bytes.append(static_cast<char>((n >> 24) & 0xFF));
    bytes.append(static_cast<char>((n >> 16) & 0xFF));
    bytes.append(static_cast<char>((n >> 8) & 0xFF));
    bytes.append(static_cast<char>(n & 0xFF));
    return bytes;
}

// 工具函数：将4字节数组（大端）转为32位整数（处理正负）
int MotorController::bytesToInt32(const QByteArray &b)
{
    if (b.size() != 4) return 0;  // 必须为4字节，否则返回0

    // 按大端顺序拼接为无符号32位值
    int n = (static_cast<quint8>(b[0]) << 24) |
            (static_cast<quint8>(b[1]) << 16) |
            (static_cast<quint8>(b[2]) << 8) |
            static_cast<quint8>(b[3]);

    // 处理负数：若值 >= 0x80000000（最高位为1），转为有符号整数
    if (n >= 0x80000000) {
        // 先转64位避免溢出，再减去2^32得到负数
        n = static_cast<int>(static_cast<qint64>(n) - 0x100000000LL);
    }
    return n;
}
