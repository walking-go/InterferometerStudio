#include "basler_camera_controller.h"
#include "qreadwritelock.h"
#include "qtimer.h"
#include <QDebug>
using namespace camera;

extern QReadWriteLock g_lock_realtime_img;

BaslerCameraController::BaslerCameraController(int id_)
    : Camera(id_)
{
    Pylon::PylonInitialize();
    m_camera.RegisterImageEventHandler(this, Pylon::RegistrationMode_ReplaceAll, Pylon::Cleanup_None);
    m_camera.RegisterConfiguration(new Pylon::CAcquireContinuousConfiguration(), Pylon::RegistrationMode_ReplaceAll, Pylon::Cleanup_Delete);
    m_camera.RegisterConfiguration(this, Pylon::RegistrationMode_Append, Pylon::Cleanup_None);
    m_format_converter.OutputPixelFormat = Pylon::PixelType_BGRA8packed;
    //enumerateDevices();
    // open();
    // start();
    m_cntGrabbedImages = 0;
    m_cntGrabErrors = 0;
}

BaslerCameraController::~BaslerCameraController()
{
    close();
    Pylon::PylonTerminate();
}


// 枚举设备逻辑
void BaslerCameraController::enumerateDevices()
{
    Pylon::DeviceInfoList_t devices;
    try{
        Pylon::CTlFactory& TlFactory = Pylon::CTlFactory::GetInstance();
        TlFactory.EnumerateDevices(devices);
    }
    catch(const Pylon::GenericException& e){
        PYLON_UNUSED( e );
        devices.clear();
        qDebug() << e.GetDescription();
    }
    m_devices = devices;
    int deviceCount = m_devices.size();
    qDebug("枚举到设备数量: %d", deviceCount);
    if(deviceCount == 0){
        qDebug() <<"未检测到相机设备";
    }
}

int BaslerCameraController::getEnumerationDevices()
{
    enumerateDevices();  // 确保每次调用都枚举
    return m_devices.size();
}

// 修改打开相机逻辑，直接使用第一个设备
void BaslerCameraController::open()
{
    enumerateDevices();
    if (m_devices.empty()) {
        qDebug() <<"无设备可打开";
        return;
    }

    // 直接使用第一个设备，无需通过QComboBox选择
    p_dev_info = &m_devices[0];
    try{
        cameraOpen(*p_dev_info);
    }
    catch(const Pylon::GenericException& e) {
        PYLON_UNUSED(e);
        qDebug() << e.GetDescription();
        qDebug()<<"相机打开失败";
        throw;
    }
}

void BaslerCameraController::start()
{
    if (!m_camera.IsOpen() || m_camera.IsGrabbing())
        return;
    m_camera.AcquisitionMode.TrySetValue( Basler_UniversalCameraParams::AcquisitionMode_Continuous );
    m_camera.StartGrabbing( Pylon::GrabStrategy_OneByOne, Pylon::GrabLoop_ProvidedByInstantCamera);
}

void BaslerCameraController::stop()
{
    m_camera.StopGrabbing();
}

void BaslerCameraController::cameraOpen(const Pylon::CDeviceInfo& devInfo)
{
    try{
        Pylon::IPylonDevice* pDevice = Pylon::CTlFactory::GetInstance().CreateDevice(devInfo);
        m_camera.Attach( pDevice, Pylon::Cleanup_Delete );
        m_camera.Open();

        if(m_camera.ExposureTime.IsValid()){
            m_camera.ExposureTime.GetAlternativeIntegerRepresentation(m_exposuretime_node);
        }
        if(m_camera.Gain.IsValid()){
            m_camera.Gain.GetAlternativeIntegerRepresentation(m_gain_node);
        }

        if(m_exposuretime_node.IsValid()){
            m_camera.RegisterCameraEventHandler(this, m_exposuretime_node.GetNode()->GetName(), 0, Pylon::RegistrationMode_Append, Pylon::Cleanup_None, Pylon::CameraEventAvailability_Optional);
        }

        if(m_gain_node.IsValid()){
            m_camera.RegisterCameraEventHandler(this, m_gain_node.GetNode()->GetName(), 0, Pylon::RegistrationMode_Append, Pylon::Cleanup_None, Pylon::CameraEventAvailability_Optional);
        }

        m_camera.RegisterCameraEventHandler(this, "PixelFormat", 0, Pylon::RegistrationMode_Append, Pylon::Cleanup_None, Pylon::CameraEventAvailability_Optional);
        m_camera.RegisterCameraEventHandler(this, "TriggerMode", 0, Pylon::RegistrationMode_Append, Pylon::Cleanup_None, Pylon::CameraEventAvailability_Optional);
        m_camera.RegisterCameraEventHandler(this, "TriggerSource", 0, Pylon::RegistrationMode_Append, Pylon::Cleanup_None, Pylon::CameraEventAvailability_Optional);
    }
    catch(const Pylon::GenericException& e){
        PYLON_UNUSED(e);
        qDebug() << e.GetDescription();
    }

    if(getExposureTimeNode().IsReadable())
    {
        m_exposuretime = getExposureTimeNode().GetValue();
        m_exposuretime_max = getExposureTimeNode().GetMax();
        m_exposuretime_min = getExposureTimeNode().GetMin();
    }
    getExposureTimeNode().IsWritable();

    if(getGainNode().IsReadable())
    {
        m_analog_gain = getGainNode().GetValue();
        m_ag_max = getGainNode().GetMax();
        m_ag_min = getGainNode().GetMin();
    }
    getGainNode().IsWritable();
}

void BaslerCameraController::executeSoftwareTrigger()
{
    if (!isGrabbing())
        return;

    if (m_camera.TriggerSource.GetValue() == Basler_UniversalCameraParams::TriggerSource_Software
        && m_camera.TriggerMode.GetValue() == Basler_UniversalCameraParams::TriggerMode_On)
    {
        try
        {
            m_camera.WaitForFrameTriggerReady( 3000, Pylon::TimeoutHandling_ThrowException );
        }
        catch (const Pylon::GenericException& e)
        {
            qDebug() << e.GetDescription();
        }
    }

    try
    {
        m_camera.ExecuteSoftwareTrigger();
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << e.GetDescription();
    }
}

uint64_t BaslerCameraController::GetGrabbedImages() const
{
    return m_cntGrabbedImages;
}

uint64_t BaslerCameraController::GetGrabErrors() const
{
    return m_cntGrabErrors;
}

void BaslerCameraController::close()
{
    m_camera.StopGrabbing();
    m_ptr_grabresult.Release();
    QImage dummy;
    m_image.swap(dummy);
    m_camera.DeregisterCameraEventHandler(this, "TriggerSource");
    m_camera.DeregisterCameraEventHandler(this, "TriggerMode");
    m_camera.DeregisterCameraEventHandler(this, "PixelFormat");

    if(m_gain_node.IsValid()){
        m_camera.DeregisterCameraEventHandler(this, m_gain_node.GetNode()->GetName());
    }

    if(m_exposuretime_node.IsValid()){
        m_camera.DeregisterCameraEventHandler(this, m_exposuretime_node.GetNode()->GetName());
    }

    m_exposuretime_node.Release();
    m_gain_node.Release();
    m_camera.DestroyDevice();
}

Pylon::IIntegerEx &BaslerCameraController::getExposureTimeNode()
{
    return m_exposuretime_node;
}

Pylon::IIntegerEx &BaslerCameraController::getGainNode()
{
    return m_gain_node;
}

void BaslerCameraController::OnImageGrabbed(Pylon::CInstantCamera &camera, const Pylon::CGrabResultPtr &grabResult)
{
    QMutexLocker lockGrabResult( &m_MemberLock );
    m_ptr_grabresult = grabResult;
    lockGrabResult.unlock();

    if (grabResult.IsValid() && grabResult->GrabSucceeded()){
        if (m_invert_image && Pylon::BitDepth(grabResult->GetPixelType()) == 8){
            size_t imageSize = Pylon::ComputeBufferSize(grabResult->GetPixelType(), grabResult->GetWidth(), grabResult->GetHeight(), grabResult->GetPaddingX());
            uint8_t* p = reinterpret_cast<uint8_t*>(grabResult->GetBuffer());
            uint8_t* const pEnd = p + (imageSize / sizeof(uint8_t));
            for(; p < pEnd; ++p)
                *p = 255 - *p;
        }

        QMutexLocker lockImage( &m_imageLock );
        if((static_cast<uint32_t>(m_image.width()) != grabResult->GetWidth())
            || (static_cast<uint32_t>(m_image.height()) != grabResult->GetHeight()))
            m_image = QImage(grabResult->GetWidth(), grabResult->GetHeight(), QImage::Format_RGB32);

        void *pBuffer = m_image.bits();
        size_t bufferSize = m_image.sizeInBytes();
        m_format_converter.Convert(pBuffer, bufferSize, grabResult);
        lockImage.unlock();

        QMutexLocker lockImageCount( &m_MemberLock );
        ++m_cntGrabbedImages;
        lockImageCount.unlock();
    }
    else{
        QMutexLocker lockImage( &m_imageLock );
        m_image.fill(0);
        lockImage.unlock();
        qDebug() << __FUNCTION__ << " 采集失败. 错误码: " << grabResult->GetErrorCode();

        QMutexLocker lockErrorCount( &m_MemberLock );
        ++m_cntGrabErrors;
        lockErrorCount.unlock();
    }

    emit captured(m_image.copy());
    PYLON_UNUSED(camera);
}

void BaslerCameraController::OnCameraEvent(Pylon::CInstantCamera &camera, intptr_t userProvidedId, GenApi::INode *pNode)
{
    if(pNode == NULL)
        return;
    qDebug() << "参数变化: " << pNode->GetName().c_str();
    PYLON_UNUSED(camera);
    PYLON_UNUSED(userProvidedId);
}

void BaslerCameraController::OnCameraDeviceRemoved(Pylon::CInstantCamera &camera)
{
    emit deviceRemoved(m_id);
    PYLON_UNUSED(camera);
}

int BaslerCameraController::getCameraId()
{
    return m_id;
}

void BaslerCameraController::setExposureTime(int value)
{
    getExposureTimeNode().TrySetValue(value);
}

void BaslerCameraController::setGain(int value)
{
    getGainNode().TrySetValue(value);
}

void BaslerCameraController::setTargetBrightness(int value)
{
    m_ae_brightness = (double)value;
    m_camera.AutoTargetBrightness.SetValue((float)(m_ae_brightness / 100.0));
}

void BaslerCameraController::setExposureMode(ExposureMode exposure_mode)
{
    m_exposure_mode = exposure_mode;
    if(m_exposure_mode == AutoExpo){
        autoExposure();
        return;
    }
    else if(m_exposure_mode == ManualExpo){
        manualExposure();
        return;
    }
}

void BaslerCameraController::setSoftwareTrigger()
{
    m_camera.TriggerMode.TrySetValue(Basler_UniversalCameraParams::TriggerMode_On);
    m_camera.TriggerSource.TrySetValue(Basler_UniversalCameraParams::TriggerSource_Software);
}

void BaslerCameraController::setHardwareTrigger()
{
    m_camera.TriggerMode.TrySetValue(Basler_UniversalCameraParams::TriggerMode_On);
    m_camera.TriggerSource.TrySetValue(Basler_UniversalCameraParams::TriggerSource_Line1);
}

void BaslerCameraController::setTriggerOff()
{
    m_camera.TriggerMode.TrySetValue(Basler_UniversalCameraParams::TriggerMode_Off);
}

double BaslerCameraController::getExposureTime()
{
    return m_exposuretime;
}

double BaslerCameraController::getExposureTimeMax()
{
    return m_exposuretime_max;
}

double BaslerCameraController::getExposureTimeMin()
{
    return m_exposuretime_min;
}

double BaslerCameraController::getGain()
{
    return m_analog_gain;
}

double BaslerCameraController::getGainMax()
{
    return m_ag_max;
}

double BaslerCameraController::getGainMin()
{
    return m_ag_min;
}

double BaslerCameraController::getTargetBrightness()
{
    return m_ae_brightness;
}

double BaslerCameraController::getTargetBrightnessMax()
{
    return m_ae_brightness_max;
}

double BaslerCameraController::getTargetBrightnessMin()
{
    return m_ae_brightness_min;
}

ExposureMode BaslerCameraController::getExposureMode()
{
    return m_exposure_mode;
}

bool BaslerCameraController::isOpen() const
{
    return m_camera.IsOpen();
}

bool BaslerCameraController::isGrabbing() const
{
    return m_camera.IsGrabbing();
}

void BaslerCameraController::autoExposure()
{
    if(!m_camera.ExposureAuto.IsWritable()){
        qDebug() << "相机不支持自动曝光";
        return;
    }
    if(m_camera.GetSfncVersion() >= Pylon::Sfnc_2_0_0){
        m_camera.AutoTargetBrightness.SetValue(m_ae_brightness / 100.0);
        qDebug() << "自动曝光开启";
        m_camera.ExposureAuto.SetValue(Basler_UniversalCameraParams::ExposureAuto_Continuous);
    }
    else{
        m_camera.AutoTargetValue.SetValue(80);
        qDebug() << "自动曝光开启";
        m_camera.ExposureAuto.SetValue(Basler_UniversalCameraParams::ExposureAuto_Continuous);
    }
}

void BaslerCameraController::manualExposure()
{
    m_camera.ExposureAuto.SetValue(Basler_UniversalCameraParams::ExposureAuto_Off);
}
