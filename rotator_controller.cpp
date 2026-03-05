#include "rotator_controller.h"
#include <QDateTime>
#include <QDebug>
//#include "Momanintf.h" // 确保头文件路径能被找到
#include "Momancmd.h"


RotatorController::RotatorController(QWidget *parent) : QWidget(parent),
    m_momanLib(nullptr),
    m_mc3UsbLib(nullptr), // 用于接口
    m_mmProtInitInterface(nullptr),
    m_mmProtCloseInterface(nullptr),
    m_mmProtOpenCom(nullptr),
    m_mmProtCloseCom(nullptr),
    m_mmProtSetObj(nullptr),
    m_mmProtGetObj(nullptr),
    m_mmIntfEnumPorts(nullptr),
    m_isConnected(false),
    m_isMotorRunning(false),
    m_lastActualSpeed(0),
    m_lastStatusWord(0x00)  // 新增：初始化为待机状态
{
    // 1. 加载协议 DLL（CO_USB.dll）：负责通信协议
    bool isProtDllLoaded = loadMomanLib();

    // 2. 加载接口 DLL（MC3Usb.dll）：负责端口枚举
    bool isIntfDllLoaded = loadMc3UsbLib();

    if (!isProtDllLoaded || !isIntfDllLoaded) {
        emit errorOccurred("DLL 加载不完整（需 CO_USB.dll 与 MC3Usb.dll）");
    }

}

RotatorController::~RotatorController()
{
    disconnectUSB();
    unloadMomanLib();
}

// -------------------------- 加载 MC3Usb.dll --------------------------
bool RotatorController::loadMc3UsbLib()
{
    // 避免重复加载
    if (m_mc3UsbLib && m_mc3UsbLib->isLoaded()) {
        qDebug() << "MC3Usb.dll 已加载，直接返回";
        return true;
    }

    // 初始化并加载 MC3Usb.dll
    m_mc3UsbLib = new QLibrary(this);
    m_mc3UsbLib->setFileName("MC3Usb.dll"); // 接口 DLL（§2.2 Tab2 USB 接口对应）


    // 检查 DLL 加载结果
    if (!m_mc3UsbLib->load()) {
        qDebug() << "MC3Usb.dll 加载失败：" << m_mc3UsbLib->errorString();
        emit errorOccurred(QString("加载 MC3Usb.dll 失败：%1").arg(m_mc3UsbLib->errorString()));
        return false;
    }
    //qDebug() << "MC3Usb.dll 加载成功";
    // 解析枚举端口函数
    m_mmIntfEnumPorts = (mmIntfEnumPortsFunc)m_mc3UsbLib->resolve("mmIntfEnumPorts");


    // 检查枚举函数是否解析成功
    if (!m_mmIntfEnumPorts) {
        emit errorOccurred("解析 mmIntfEnumPorts 失败（MC3Usb.dll 版本不兼容或缺失函数）");
        m_mc3UsbLib->unload();
        return false;
    }

    qDebug() << "MC3Usb.dll 函数解析完成";
    return true;
}

// -------------------------- 加载 CO_USB.dll --------------------------
bool RotatorController::loadMomanLib()
{
    if (m_momanLib && m_momanLib->isLoaded()) {
        qDebug() << "CO_USB.dll 已加载，直接返回";
        return true;
    }

    m_momanLib = new QLibrary(this);
    m_momanLib->setFileName("CO_USB.dll"); // 协议 DLL（§2.2 Tab1）

    if (!m_momanLib->load()) {
        qDebug() << "CO_USB.dll 加载失败：" << m_momanLib->errorString();
        emit errorOccurred(QString("加载 CO_USB.dll 失败：%1").arg(m_momanLib->errorString()));
        return false;
    }

    // 解析协议相关函数（mmProtXXX）
    m_mmProtInitInterface = (mmProtInitInterfaceFunc)m_momanLib->resolve("mmProtInitInterface");
    m_mmProtCloseInterface = (mmProtCloseInterfaceFunc)m_momanLib->resolve("mmProtCloseInterface");
    m_mmProtOpenCom = (mmProtOpenComFunc)m_momanLib->resolve("mmProtOpenCom");
    m_mmProtCloseCom = (mmProtCloseComFunc)m_momanLib->resolve("mmProtCloseCom");
    m_mmProtSetObj = (mmProtSetObjFunc)m_momanLib->resolve("mmProtSetObj");
    m_mmProtGetObj = (mmProtGetObjFunc)m_momanLib->resolve("mmProtGetObj");
    m_mmProtScanNode = (mmProtScanNodeFunc)m_momanLib->resolve("mmProtScanNode");

    // 检查：验证协议函数
    if (!m_mmProtInitInterface || !m_mmProtCloseInterface || !m_mmProtOpenCom ||
        !m_mmProtCloseCom || !m_mmProtSetObj || !m_mmProtGetObj|| !m_mmProtScanNode) {
        qDebug() << "存在未解析的协议函数，卸载 CO_USB.dll";
        emit errorOccurred("解析 CO_USB.dll 协议函数失败（参考 §5 API 列表）");
        m_momanLib->unload();
        return false;
    }
    qDebug() << "CO_USB.dll 协议函数解析完成";
    return true;
}

// -------------------------- 卸载 CO_USB.dll --------------------------
void RotatorController::unloadMomanLib()
{
    // 卸载协议 DLL（CO_USB.dll）
    if (m_momanLib && m_momanLib->isLoaded()) {
        m_momanLib->unload();
    }
    delete m_momanLib;
    m_momanLib = nullptr;

    // 卸载接口 DLL（MC3Usb.dll）
    if (m_mc3UsbLib && m_mc3UsbLib->isLoaded()) {
        m_mc3UsbLib->unload();
    }
    delete m_mc3UsbLib;
    m_mc3UsbLib = nullptr;

    // 重置函数指针
    m_mmIntfEnumPorts = nullptr;
}

//--------------------------枚举端口--------------------------
QStringList RotatorController::enumUSBPorts()
{
    QStringList ports;
    if (!m_mmIntfEnumPorts) {
        emit errorOccurred("枚举端口函数未解析（MC3Usb.dll 加载失败）");
        return ports;
    }

    // 传递 3 个参数（均为输出指针）
    const char* portList = nullptr;    // 端口ID列表（格式："1,2,2"）
    const char* chanList = nullptr;    // 通道号列表（格式："0,1,2"）
    const char* deviceInfoList = nullptr; // 设备信息列表
    int channelCount = m_mmIntfEnumPorts(&portList, &chanList, &deviceInfoList);

    // 判断：channelCount ≤0 表示无端口
    if (channelCount <= 0 || !portList) {
        emit errorOccurred("未检测到 USB 端口");
        return ports;
    }

    // 解析端口列表（端口ID按逗号分隔，去重后生成端口名）
    QString portStr = QString::fromUtf8(portList);
    QStringList portItems = portStr.split(",", Qt::SkipEmptyParts);

    // 去重：同一端口可能对应多个通道（如 "2,2" 对应端口2的2个通道，仅保留1个端口名）
    QSet<QString> uniquePorts(portItems.begin(), portItems.end());
    for (const QString& portId : uniquePorts) {
        ports.append(QString("USB Port %1").arg(portId)); // 格式：USB Port 1
    }

    return ports;
}

// -------------------------- 连接 USB --------------------------
bool RotatorController::connectUSB(int port)
{
    if (m_isConnected) {
        emit errorOccurred("已连接 USB，无需重复操作");
        return false;
    }
    if (!loadMomanLib()) return false;

    // 1. 初始化接口（加载 MC3Usb.dll 接口 DLL）
    int initRet = m_mmProtInitInterface("MC3Usb.dll", nullptr, nullptr);
    if (initRet != 0) { // 0 = eMomanprot_ok（EN_7000_05064 §5.2.1）
        emit errorOccurred(QString("初始化 USB 接口失败，错误码：%1").arg(initRet));
        return false;
    }

    // 2. 打开 USB 端口（USB 对应 channel=0、baud=0，EN_7000_05064 §5.2.3）
    int openRet = m_mmProtOpenCom(port, 0, 0);
    if (openRet != 0) {
        emit errorOccurred(QString("打开 USB Port %1 失败，错误码：%2").arg(port).arg(openRet));
        return false;
    }

    // 3. 扫描节点（获取实际节点号）
    if (!scanDriveNodes()) {
        emit errorOccurred("扫描驱动器节点失败，将使用默认节点号 1");
        m_actualNodeNr = 1; // 降级到默认节点
    }

    // 4. 检查电机状态 + 发送连接成功信号（原有逻辑保留，需修改节点号显示）
    getStatusWord();
    m_isConnected = true;
    emit usbConnected(true, QString("成功连接 USB Port %1，节点号：%2")
                                .arg(port).arg(m_actualNodeNr));
    emit motorStateChanged("待机");
    return true;

}

// -------------------------- 断连 USB --------------------------
void RotatorController::disconnectUSB()
{
    if (!m_isConnected) return;

    // 停止电机后断开
    stopMotor();
    m_mmProtCloseCom();
    m_mmProtCloseInterface();

    m_isConnected = false;
    m_isMotorRunning = false;
    emit usbConnected(false, "已断开 USB 连接");
    emit motorStateChanged("待机");
}

// -------------------------- 扫描节点函数 --------------------------
bool RotatorController::scanDriveNodes()
{
    // 1. 检查函数指针有效性
    if (!m_mmProtScanNode) {
        emit errorOccurred("扫描节点函数未解析（CO_USB.dll 缺失或函数缺失）");
        m_actualNodeNr = 1; // 文档推荐默认节点
        qDebug() << "扫描函数未解析，fallback 到默认节点号 1";
        return false;
    }

    // 2. 初始化输出参数
    const char* deviceName = nullptr;  // 文档 §5.2.5 设备名称输出
    int deviceMode = 0;                // 文档 §5.2.5 设备模式输出（eDeviceMode 枚举值）

    // 3. 调用扫描函数（广播扫描）
    int foundNodeNr = m_mmProtScanNode(
        -1,               // nodeNr=-1：广播扫描所有节点
        &deviceName,      // 二级指针：接收设备名称
        deviceMode        // 引用传递：接收设备模式（文档明确为 int&）
        );

    // 4. 打印原始结果
    qDebug() << "【SCAN】原始返回值：" << foundNodeNr
             << "，设备名称：" << (deviceName ? deviceName : "无")
             << "，设备模式：" << deviceMode;

    // 5. 错误处理
    if (foundNodeNr == -1) { // 文档 §5.2.5 明确：-1 = 未找到节点
        emit errorOccurred("未发现任何FAULHABER驱动器，请检查USB连接");
        m_actualNodeNr = 1;
        return false;
    }

    // 6. 设备模式校验
    if (deviceMode != eDeviceMode_Faulhaber) { // 需提前定义枚举
        emit errorOccurred(QString("非FAULHABER设备（模式码：%1），拒绝连接")
                               .arg(deviceMode));
        m_actualNodeNr = 1;
        return false;
    }

    // 7. 节点号有效性检查
    if (foundNodeNr < 1 || foundNodeNr > 127) {
        emit errorOccurred(QString("无效节点号：%1（有效范围1-127）")
                               .arg(foundNodeNr));
        m_actualNodeNr = 1;
        return false;
    }

    // 8. 扫描成功
    m_actualNodeNr = foundNodeNr;
    qDebug() << "【SCAN】成功发现节点：" << m_actualNodeNr
             << "（设备名称：" << deviceName << "）";
    emit nodeScanned(m_actualNodeNr, QString::fromUtf8(deviceName)); // 自定义信号
    return true;
}

// -------------------------- 设置目标转速  --------------------------
bool RotatorController::setTargetSpeed(int rpm)
{
    if (!m_isConnected) {
        emit errorOccurred("未连接 USB，无法设置转速");
        return false;
    }
    if (rpm < 0 || rpm > 6000) {
        emit errorOccurred("转速超出范围（0~6000 RPM）");
        return false;
    }

    // 写入目标转速
    unsigned int abortCode;
    int ret = m_mmProtSetObj(m_actualNodeNr, TARGET_SPEED_INDEX, 0x00, rpm, 4, abortCode);
    if (ret != 0 || abortCode != 0) {
        emit errorOccurred(QString("设置转速失败，错误码：%1，中止码：%2").arg(ret).arg(abortCode));
        return false;
    }
    qDebug() << "[设置转速] 成功写入目标转速：" << rpm << " RPM";
    QThread::msleep(1000);
    getActualSpeed();
    return true;
}

// -------------------------- 读取实际转速 --------------------------
int RotatorController::getActualSpeed()
{
    if (!m_isConnected) {
        emit errorOccurred("未连接 USB，无法读取转速");
        return 0;
    }

    // 读取实际转速（0x606C 索引，EN_7000_05064 §5.3.1）
    int actualSpeed = 0;
    int ret = m_mmProtGetObj(m_actualNodeNr, ACTUAL_SPEED_INDEX, 0x00, actualSpeed);
    if (ret != 0) {
        emit errorOccurred(QString("读取实际转速失败，错误码：%1").arg(ret));
        return m_lastActualSpeed; // 缓存值兜底
    }

    m_lastActualSpeed = actualSpeed;
    return actualSpeed;
}

// -------------------------- 启动/停止电机 --------------------------
bool RotatorController::startMotor()
{
    if (!m_isConnected) {
        emit errorOccurred("未连接 USB，无法启动电机");
        return false;
    }

    if (m_isMotorRunning) {
        emit errorOccurred("电机已处于运行状态，无需重复启动");
        return true;
    }

    // 前置检查：电机无故障
    if (checkMotorFault()) {
        emit errorOccurred("电机存在故障，需先清除故障再启动");
        return false;
    }

    // -------------------------- 配置操作模式为速度模式 --------------------------
    unsigned int abortCode = 0; // 接收 DLL 中止码（用于故障排查）
    int ret = m_mmProtSetObj(
        m_actualNodeNr,   // 关键：使用扫描到的实际节点号
        0x6060,           // 操作模式索引
        0x00,             // 子索引
        3,               // 速度模式值（§3.3.2.10 明确 3=速度模式）
        1,                // 数据长度（1 字节=8 位，§5.3.1 操作模式参数长度）
        abortCode         // 输出：中止码（0=无故障）
        );

    // 检查配置结果（§5.2.4 成功判断规则）
    if (ret != 0 || abortCode != 0) {
        QString errorMsg = QString(
                               "配置速度模式失败！ret=%1，abortCode=%2\n"
                               "排查方向：1. 节点号 %3 是否匹配驱动器 2. 驱动器是否处于待机状态"
                               ).arg(ret).arg(abortCode).arg(m_actualNodeNr);
        emit errorOccurred(errorMsg);
        qDebug() << "[startMotor] " << errorMsg;
        return false;
    }
    //qDebug() << "[startMotor] 操作模式配置成功：速度模式（值=3），节点号=" << m_actualNodeNr;

    // ========== 阶段 1：发送「就绪」控制字（CTRL_WORD_READY = 0x0006） ==========
    if (!sendControlWord(CTRL_WORD_READY)) {
        return false;
    }
    QThread::msleep(100);  // 延迟，给驱动器响应时间（可根据实际调整）
    //qDebug() << "[启动电机] 阶段1：就绪控制字已发送，等待响应...";

    // ========== 阶段 2：发送「上电」控制字（CTRL_WORD_POWER_ON = 0x0007） ==========
    if (!sendControlWord(CTRL_WORD_POWER_ON)) {
        return false;
    }
    QThread::msleep(100);
    //qDebug() << "[启动电机] 阶段2：上电控制字已发送，等待响应...";

    // ========== 阶段 3：发送「运行使能」控制字（CTRL_WORD_OP_ENABLE = 0x000F） ==========
    if (!sendControlWord(CTRL_WORD_OP_ENABLE)) {
        return false;
    }
    QThread::msleep(100);
    //qDebug() << "[启动电机] 阶段3：运行使能控制字已发送，验证状态...";

    // ========== 验证：读取状态字，确认是否真正“运行使能” ==========
    int statusWord = getStatusWord();
    if (statusWord == 0) {
        emit errorOccurred("读取状态字失败，无法验证电机状态");
        return false;
    }

    // 解析状态字关键位（参考 CiA 402 标准）
    bool isReady = (statusWord & 0x01) != 0;        // 位0：就绪
    bool isPowerOn = (statusWord & 0x02) != 0;      // 位1：已上电
    bool isOpEnabled = (statusWord & 0x04) != 0;    // 位2：运行使能
    bool isFault = (statusWord & 0x08) != 0;        // 位3：故障

    qDebug() << "[启动电机] 状态字解析：就绪=" << isReady
             << "，已上电=" << isPowerOn
             << "，运行使能=" << isOpEnabled
             << "，故障=" << isFault;

    if (!isOpEnabled) {
        emit errorOccurred("控制字发送成功，但电机未进入「运行使能」状态（状态字位2未置位）");
        return false;
    }

    // 标记为“运行中”，发送信号更新 UI
    m_isMotorRunning = true;
    emit motorStateChanged("运行中");
    qDebug() << "[启动电机] 电机已成功进入运行状态";

    return true;
}

bool RotatorController::stopMotor()
{
    if (!m_isConnected || !m_isMotorRunning) {
        return true;  // 未连接或已停止，直接返回
    }

    // 发送「停止」控制字（CTRL_WORD_STOP = 0x00）
    unsigned int abortCode = 0;
    int ret = m_mmProtSetObj(
        m_actualNodeNr,           // 节点号
        CONTROL_WORD_INDEX, // 控制字寄存器索引
        0x00,              // 子索引
        CTRL_WORD_STOP,    // 停止控制字
        2,                 // 数据长度（2 字节 = 16 位整数）
        abortCode
        );

    if (ret != 0 || abortCode != 0) {
        emit errorOccurred(QString("停止电机失败，错误码：%1，中止码：%2").arg(ret).arg(abortCode));
        return false;
    }

    // 标记为“停止”，发送信号更新 UI
    m_isMotorRunning = false;
    emit motorStateChanged("待机");
    qDebug() << "[停止电机] 电机已成功停止";

    return true;
}

// ====================== 检查电机故障（读取状态字故障位） ======================
bool RotatorController::checkMotorFault()
{
    int statusWord = getStatusWord();
    if (statusWord == 0) {
        emit errorOccurred("读取状态字失败，无法检测故障");
        return true; // 读取失败时默认认为“存在故障”
    }

    // 状态字位3（0x08）为1表示“故障”
    bool isFault = (statusWord & 0x08) != 0;

    if (isFault) {
        emit motorStateChanged("故障");
        qDebug() << "[故障检测] 电机存在故障，状态字：0x" << QString::number(statusWord, 16);
    } else {
        qDebug() << "[故障检测] 电机无故障，状态字：0x" << QString::number(statusWord, 16);
    }

    return isFault;
}

// ====================== 辅助函数：发送控制字并检查结果 ======================
bool RotatorController::sendControlWord(int controlWord)
{
    unsigned int abortCode = 0;
    int ret = m_mmProtSetObj(
        m_actualNodeNr,           // 节点号
        CONTROL_WORD_INDEX, // 控制字寄存器索引
        0x00,              // 子索引
        controlWord,       // 要发送的控制字
        2,                 // 数据长度（2 字节 = 16 位整数）
        abortCode
        );

    if (ret != 0) {
        emit errorOccurred(QString("发送控制字 0x%1 失败，错误码：%2").arg(controlWord, 0, 16).arg(ret));
        return false;
    }

    //qDebug() << "[发送控制字] 成功发送控制字：0x" << QString::number(controlWord, 16);
    return true;
}

// ====================== 辅助函数：读取状态字 ======================
int RotatorController::getStatusWord()
{
    int statusWord = 0;
    int ret = m_mmProtGetObj(
        m_actualNodeNr,           // 节点号
        STATUS_WORD_INDEX, // 状态字寄存器索引
        0x00,              // 子索引
        statusWord         // 输出参数：存储状态字
        );

    //qDebug() << "[读取状态字] 状态字=0x" << QString::number(statusWord, 16) << "，返回码ret=" << ret;

    if (ret != 0) {
        emit errorOccurred(QString("读取状态字失败，错误码：%1").arg(ret));
        // 读取失败：返回上一次缓存的有效状态字（遵循 §5.3.1 “保留有效数据”要求）
        qDebug() << "[读取状态字] 失败，返回缓存状态字：0x" << QString::number(m_lastStatusWord, 16);
        return m_lastStatusWord;
    }

    // 读取成功：更新缓存，并返回最新状态字
    m_lastStatusWord = statusWord;
    //qDebug() << "[读取状态字] 成功，状态字：0x" << QString::number(statusWord, 16);
    return statusWord;
}

// 供 UI 层获取可用端口列表（用于显示）
QStringList RotatorController::getAvailableUSBPorts()
{
    return enumUSBPorts(); // 内部调用私有枚举函数
}

// 供 UI 层获取转速的接口
int RotatorController::getCurrentSpeed() {
    return getActualSpeed(); // 内部调用私有getActualSpeed
}
