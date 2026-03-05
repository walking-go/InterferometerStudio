#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "motor_serial.h"
#include <QObject>
#include <QList>
#include <QMap>
#include <QString>
#include <QMutex>  // 用于线程同步

// 轴状态类：用于存储单个轴（如X、Y、Z轴）的实时状态和属性
class Axis : public QObject
{
    Q_OBJECT  // 支持Qt信号槽机制

public:
    explicit Axis(bool limitFlag = true, int axisId = -1, QObject *parent = nullptr)
        : QObject(parent),
        curPlace(0),       // 当前位置（初始化为0）
        limitState(0),     // 限位状态（0：无限位，其他值表示不同限位状态）
        limitFlag(limitFlag),  // 是否启用限位
        axisId(axisId),    // 轴的唯一标识ID
        workingState(0),   // 工作状态（0：停止，其他值表示运行中/错误等）
        staticFlag(true),  // 是否静止（true：静止，false：运动中）
        cnt(0),             // 静止计数（连续多次检测到静止时确认状态）
        isStopping(false),
        speed(0x000258)    // 默认速度600（3字节大端格式）
    {}

    // 成员变量：轴的状态参数
    int curPlace;       // 当前位置值
    int limitState;     // 限位状态（如机械限位触发情况）
    bool limitFlag;     // 是否启用限位检测
    int axisId;         // 轴的ID（用于区分不同轴）
    int workingState;   // 工作状态（运行/停止/错误等）
    bool staticFlag;    // 静止标志（判断轴是否已停止运动）
    int cnt;            // 静止计数器（累计静止状态次数，用于防抖）
    bool isStopping;    // 标记是否正在执行停止流程
    int speed;          // 轴的移动速度设置（3字节大端格式，默认0x000258 = 600）

    int m_ustepsPerCir;
};


// 移动平台控制类：继承自串口工具类
class MotorController : public MotorSerial
{
    Q_OBJECT  // 支持Qt信号槽机制

public:

    explicit MotorController(QObject *parent = nullptr);// 参数：parent（父对象，用于内存管理）

    int readX(const QString &axisName);// 读取指定轴的当前位置
    int readId(const QString &axisName);// 读取指定轴的ID
    void movedX(int dx, const QString &axisName);// 相对移动指定轴（在当前位置基础上移动dx）
    void moveToX(int x, const QString &axisName);// 绝对移动指定轴到目标位置x
    void moveV(int v, const QString &axisName);// 控制指定轴的运动
    void stop(const QString &axisName);// 停止指定轴的运动

    // // 相移：同时移动X、Y、Z三轴（各轴使用独立速度和位移）
    // void phaseShift(int xSpeed, int xDisplacement,
    //                 int ySpeed, int yDisplacement,
    //                 int zSpeed, int zDisplacement);

    // 速度设置相关
    void setSpeed(int speed, const QString &axisName);// 设置指定轴的移动速度
    int getSpeed(const QString &axisName) const;// 获取指定轴的当前速度设置


signals:

    void axisStateUpdated(int axisId);// 轴状态更新信号
    void axisLimitTriggered(int axisId, quint8 limitState); // 轴ID + 限位状态


private slots:

    void processReceivedData(const QString &hexStr);// 处理接收到的串口数据（私有槽函数，由串口数据接收信号触发）

private:

    QByteArray int32ToBytes(int n);// 将32位整数转换为字节数组（用于串口发送数据时的格式转换）
    int bytesToInt32(const QByteArray &b);// 将字节数组转换为32位整数（用于解析串口接收的数据）

    // 轴实例：X、Y、Z、M四个轴
    Axis *m_axisX;
    Axis *m_axisY;
    Axis *m_axisZ;
    Axis *m_axisM;

    // 映射表：通过轴ID快速查找对应的Axis对象
    QMap<int, Axis*> m_id2Axis;
    // 映射表：通过轴名称（如"X"）快速查找对应的Axis对象
    QMap<QString, Axis*> m_name2Axis;


    int m_ustepsPerCir;// 每圈的微步数（用于位置与实际物理位移的换算，根据设备参数设置）
    bool m_errorFlag;// 错误标志：标识是否发生数据解析或控制错误
    QMutex m_axisMutex;// 互斥锁：保护所有Axis对象的并发访问
};

// 结束头文件保护宏
#endif // MOBILE_UTILS_H
