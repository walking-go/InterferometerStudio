#ifndef BASLER_CAMERA_CONTROLLER_H
#define BASLER_CAMERA_CONTROLLER_H
#pragma warning (disable:4828)
#include <Windows.h>

// basler相机库文件
// #include "qcombobox.h"
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>

#include <QObject>
#include <QImage>
#include <QComboBox>
#include <QThread>
#include <QMutex>

namespace camera {

/**
     * @brief 相机曝光模式
     * 有如下三种情况:
     *      未知曝光模式
     *      自动曝光模式
     *      手动曝光模式
     * 初始化默认为UnKnownExpo
     */
enum ExposureMode
{
    UnKnownExpo,    /**<    未知曝光模式*/
    AutoExpo,       /**<    自动曝光模式*/
    ManualExpo      /**<    手动曝光模式*/
};

/**
     * @brief The Camera class
     *
     * 相机类，继承自QObject类
     * 作为抽象类，具有相机的通用属性及功能
     * 功能：
     * · 控制相机打开与关闭，控制相机采集图像与暂停
     * · 设置相机的曝光模式，分为自动曝光与手动曝光
     * 特点：
     * · 通过相机id来区分多个相机
     * · 检测相机是否打开，检测相机是否采集图像
     * · 自动曝光可以改变目标亮度，手动曝光可以改变曝光时间与增益
     */
class Camera : public QObject
{
    Q_OBJECT
public:
    explicit Camera(int id_):m_id(id_){};
    virtual ~Camera(){};
    // 相机本身
    std::string m_discription = "";             /** 相机描述*/
    int m_id;                                   /** 相机ID*/
    ExposureMode m_exposure_mode = UnKnownExpo; /** 曝光模式*/
    QTimer *m_fps_timer;                        /** 帧率计时器*/
    // 图
    QImage m_image;
    // 控制相机
    // 枚举设备
    virtual void enumerateDevices() = 0;
    virtual int getEnumerationDevices() = 0;
    // 控制相机状态函数
    virtual void open() = 0;
    virtual void close() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    // 获取相机状态
    virtual bool isOpen() const = 0;
    virtual bool isGrabbing() const = 0;

    virtual void executeSoftwareTrigger(){};

    virtual uint64_t GetGrabbedImages() const = 0;
    virtual uint64_t GetGrabErrors() const = 0;

    // 获取相机参数
    virtual double getExposureTime() = 0;
    virtual double getExposureTimeMax() = 0;
    virtual double getExposureTimeMin() = 0;
    virtual double getGain() = 0;
    virtual double getGainMax() = 0;
    virtual double getGainMin() = 0;
    virtual double getTargetBrightness() = 0;
    virtual double getTargetBrightnessMax() = 0;
    virtual double getTargetBrightnessMin() = 0;
    virtual ExposureMode getExposureMode() = 0;
signals:
    void captured(const QImage& img_);
    //        void fpsUpdated(int fps_);

public slots:
    virtual void setExposureTime(int value) = 0;
    virtual void setGain(int value) = 0;
    virtual void setTargetBrightness(int value) = 0;
    virtual void setExposureMode(ExposureMode  exposure_mode) = 0;
    virtual void showFps(){};
    virtual void setImgRange(int index){};

    /** HYA begin*/
    virtual void setSoftwareTrigger(){};
    virtual void setHardwareTrigger(){};
    virtual void setTriggerOff(){};
    /** HYA end*/

};

/**
     * @brief The CameraBasler class
     *
     * 相机类，继承自camera类
     * 继承自Pylon::CImageEventHandler,用于图像捕获
     * 继承自Pylon::CConfigurationEventHandler,用于响应相机拔出事件
     * 继承自Pylon::CCameraEventHandler,用于响应相机参数改变事件(如曝光时间和增益等数值上的改变)
     * 功能：
     * · 具有camera类的功能，包括控制相机打开与关闭、控制相机采集图像与暂停、设置相机的曝光模式
     * · 响应相机拔出事件，会向外发出信号
     * 特点：
     * · 会创建Pylon与相机进行连接交互
     * · 图像捕获，相机每捕获一张照片回调一次OnImageGrabbed函数,并将结果存在Pylon::CGrabResultPtr中
     * · 改变手动曝光模式中的曝光时间与增益，相机会回调OnCameraEvent函数
     * · 相机拨出后会回调OnCameraDeviceRemoved函数
     */
class BaslerCameraController : public Camera
    , public Pylon::CImageEventHandler
    , public Pylon::CConfigurationEventHandler
    , public Pylon::CCameraEventHandler
{
    Q_OBJECT
public:
    explicit BaslerCameraController(int id_);
    virtual ~BaslerCameraController() override;
    virtual void enumerateDevices() override;
    virtual int getEnumerationDevices() override;

    //重写相机控制函数
    virtual void open() override;
    virtual void close() override;
    virtual void start() override;
    virtual void stop() override;

    virtual double getExposureTime()override;
    virtual double getExposureTimeMax()override;
    virtual double getExposureTimeMin()override;

    virtual double getGain() override;
    virtual double getGainMax() override;
    virtual double getGainMin() override;

    virtual double getTargetBrightness() override;
    virtual double getTargetBrightnessMax() override;
    virtual double getTargetBrightnessMin() override;
    virtual ExposureMode getExposureMode() override;
    virtual bool isOpen() const override;
    virtual bool isGrabbing() const override;
    int getCameraId();

    virtual void executeSoftwareTrigger() override;

    virtual uint64_t GetGrabbedImages() const override;
    virtual uint64_t GetGrabErrors() const override;

private:
    const Pylon::CDeviceInfo *p_dev_info;                                   /** 相机句柄*/
    Pylon::CBaslerUniversalInstantCamera m_camera;                          /** Pylon中表示相机，用来访问相机实物*/
    Pylon::DeviceInfoList_t m_devices;                                      /** 相机设备数量*/
    Pylon::CGrabResultPtr m_ptr_grabresult;                                 /** 相机图像抓取结果*/
    Pylon::CImageFormatConverter m_format_converter;                        /** 将图像转换成QImage*/
    Pylon::CAcquireContinuousConfiguration m_continuous_configuration;      /** 启动连续采集时需要设置相机的状态*/
    Pylon::CIntegerParameter m_exposuretime_node;                           /** 手动曝光时间，用于与相机交互*/
    Pylon::CIntegerParameter m_gain_node;                                   /** 手动曝光增益，用于与相机交互*/
    bool m_invert_image = false;                                            /** 切换图像，黑白图像切换*/
    // QComboBox cameralist;

    uint64_t m_cntGrabbedImages;
    uint64_t m_cntGrabErrors;

    mutable QMutex m_MemberLock;
    // Protects the converted image.
    mutable QMutex m_imageLock;

    /** 相机列表*/
    // Operations
    void autoExposure();
    void manualExposure();
    void cameraOpen(const Pylon::CDeviceInfo& pdevInfo);

    //Config
    Pylon::IIntegerEx& getExposureTimeNode();
    Pylon::IIntegerEx& getGainNode();
    // Pylon::CImageEventHandler function
    virtual void OnImageGrabbed(Pylon::CInstantCamera& camera, const Pylon::CGrabResultPtr& grabResult ) override;
    // Pylon::CCameraEventHandler function
    virtual void OnCameraEvent(Pylon::CInstantCamera& camera, intptr_t userProvidedId, GenApi::INode* pNode ) override;
    virtual void OnCameraDeviceRemoved(Pylon::CInstantCamera& camera ) override;

protected:
    double m_ae_brightness = 20.0;                  /** 自动曝光目标亮度*/
    double m_ae_brightness_max = 80.0;              /** 自动曝光目标亮度最大值*/
    double m_ae_brightness_min = 20.0;              /** 自动曝光目标亮度最小值*/
    double m_exposuretime = 1.0;                    /** 曝光时间*/
    double m_exposuretime_max = 1.0;                /** 曝光时间最大值*/
    double m_exposuretime_min = 1.0;                /** 曝光时间最小值*/
    double m_analog_gain = 10.0;                    /** 增益*/
    double m_ag_max = 10.0;                         /** 增益最大值*/
    double m_ag_min = 10.0;                         /** 增益最小值*/
signals:
    void deviceRemoved(int id);

public slots:
    virtual void setExposureTime(int value) override;
    virtual void setGain(int value) override;
    virtual void setTargetBrightness(int value) override;
    virtual void setExposureMode(ExposureMode exposure_mode) override;
    /** HYA begin*/
    virtual void setSoftwareTrigger() override;
    virtual void setHardwareTrigger() override;
    virtual void setTriggerOff() override;
    /** HYA end*/
};

}


#endif // BASLER_CAMERA_CONTROLLER_H
