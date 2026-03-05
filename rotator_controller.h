#ifndef ROTATOR_CONTROLLER_H
#define ROTATOR_CONTROLLER_H

#include <QObject>
#include <QLibrary>
#include <QString>
#include <QWidget>
#include <QThread>
#include <QDateTime>
#include <QDebug>


// --------------------------  API 函数指针定义 --------------------------
typedef int (*mmProtInitInterfaceFunc)(const char* interfaceDll, void (*dataCallback)(void), void (*traceCallback)(void));
typedef void (*mmProtCloseInterfaceFunc)(void);
typedef int (*mmProtOpenComFunc)(int port, int channel, int baud);
typedef void (*mmProtCloseComFunc)(void);
typedef int (*mmProtSetObjFunc)(int nodeNr, int index, int subIndex, int value, int len, unsigned int& abortCode);
typedef int (*mmProtGetObjFunc)(int nodeNr, int index, int subIndex, int& value);
typedef int (*mmIntfEnumPortsFunc)(const char** portList, const char** chanList, const char** deviceInfoList);
typedef int (*mmProtScanNodeFunc)(int nodeNr,const char** deviceName, int& deviceMode);

// -------------------------- 协议定义 --------------------------
#define NODE_NR  1                  // 节点号（EN_TI_DRIVE_ELECTRONICS §14 通用值）
#define TARGET_SPEED_INDEX  0x60FF  // 目标转速对象字典索引（EN_TI_DRIVE_ELECTRONICS §14）
#define ACTUAL_SPEED_INDEX  0x606C  // 实际转速对象字典索引（EN_TI_DRIVE_ELECTRONICS §14）
#define CONTROL_WORD_INDEX  0x6040  // 控制字索引（启动/停止，EN_TI_DRIVE_ELECTRONICS §13）
#define STATUS_WORD_INDEX  0x6041   // 状态字索引（故障判断，EN_TI_DRIVE_ELECTRONICS §13）

// -------------------------- 控制字分阶段值 --------------------------
#define CTRL_WORD_READY 0x0006          // 阶段1：驱动器就绪（Switch On）
#define CTRL_WORD_POWER_ON 0x0007       // 阶段2：驱动器上电（Enable Voltage）
#define CTRL_WORD_OP_ENABLE 0x000F      // 阶段3：电机运行使能（Operation Enable）
#define CTRL_WORD_STOP 0x00           // 停止控制字

class RotatorController : public QWidget
{
    Q_OBJECT

public:
    explicit RotatorController(QWidget *parent = nullptr);
    ~RotatorController();

    // 对外接口（无协议细节，仅暴露功能）
    bool connectUSB(int port);           // 连接指定 USB 端口（port：1,2,...）
    void disconnectUSB();                // 断开 USB 连接
    bool setTargetSpeed(int rpm);        // 设置目标转速（0~5000 RPM，EN_TI_DRIVE_ELECTRONICS §14）
    bool startMotor();                   // 启动电机
    bool stopMotor();                    // 停止电机

    QStringList getAvailableUSBPorts();  // 供 UI 层获取可用端口列表（用于填充下拉框）
    int getCurrentSpeed() ;              // 内部调用私有getActualSpeed


signals:
    // 声明 nodeScanned 信号（参数：节点号、设备名称）
    void nodeScanned(int nodeNumber, const QString& deviceName);

    // 状态反馈信号（界面层订阅）
    void usbConnected(bool success, const QString& msg);  // USB 连接结果
    void motorStateChanged(const QString& state);         // 电机状态（待机/运行/故障）
    void errorOccurred(const QString& errorMsg);          // 错误信息


private:
    // MomanLib 核心资源
    QLibrary* m_momanLib;
    QLibrary* m_mc3UsbLib;// 1. 新增：用于加载 MC3Usb.dll（接口 DLL）
    mmProtInitInterfaceFunc m_mmProtInitInterface;
    mmProtCloseInterfaceFunc m_mmProtCloseInterface;
    mmProtOpenComFunc m_mmProtOpenCom;
    mmProtCloseComFunc m_mmProtCloseCom;
    mmProtSetObjFunc m_mmProtSetObj;
    mmProtGetObjFunc m_mmProtGetObj;
    mmIntfEnumPortsFunc m_mmIntfEnumPorts;
    mmProtScanNodeFunc m_mmProtScanNode; // 新增：扫描节点的函数指针（需提前 typedef）
    int m_actualNodeNr;                  // 新增：存储扫描到的实际节点号

    // 运行状态
    bool m_isConnected;                  // USB 连接状态
    bool m_isMotorRunning;               // 电机运行状态
    int m_lastActualSpeed;               // 缓存实际转速
    int m_lastStatusWord;   // 新增：状态字缓存（遵循 §5.3.1 失败时保留有效数据

    // 私有辅助函数（协议细节封装）
    bool loadMomanLib();                 // 加载 MomanLib DLL（EN_7000_05064 §3.2）
    void unloadMomanLib();               // 释放 DLL
    bool checkMotorFault();              // 检查电机故障（通过状态字，EN_TI_DRIVE_ELECTRONICS §13）
    bool loadMc3UsbLib();
    QStringList enumUSBPorts();          // 枚举可用 USB 端口（EN_7000_05064 §6.1）
    bool scanDriveNodes();
    bool isConnected() const { return m_isConnected; } // 获取连接状态
    bool sendControlWord(int word);   // 发送控制字的辅助函数
    int getStatusWord();              // 读取状态字的辅助函数
    int getActualSpeed();                // 获取实际转速

};

#endif // ROTATOR_CONTROLLER_H
