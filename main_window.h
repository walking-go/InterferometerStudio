#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QFrame>
#include "camera_switcher.h"
#include "camera_controller.h"
#include "zoom_control_widget.h"
#include "rotator_control_widget.h"
#include "motor_control_widget.h"
#include "basler_exposuretime_widget.h"

#include <QScrollArea>
#include <QStackedWidget>
#include "page_process2d.h"
#include "page_process3d.h"
#include "utils/element_canvas.h"
#include "uddm_namespace.h"
#include "utils/surface_area.h"
#include "utils/curve_chart.h"
#include "algorithm/alg.h"
#include "measurement_control_widget.h"
#include "utils/img_viewer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr); // 仅声明，实现移到cpp
    ~MainWindow() override; // 析构，释放相机


private slots:
    // 页面切换槽函数
    void onCalibratePage_Clicked();
    void onMeasurePage_2d_Clicked();
    void onMeasurePage_3d_Clicked();

public slots:
    //更新Target
    void setTopo(const QImage& topo_, bool boardcast_ = true);
    void setTopo2DStyle(Topo2DStyle style_, bool boardcast_ = true);
    void onResultSolved(const APDAResult& result_);
    void onTargetUpdated();

    // 测量开始时更新校准页BotttomViewer
    void updateCalibrateBottomViewer(const QImage& firstInterferogram);

    // 测量完成后更新校准页topViewer
    void updateCalibrateTopViewer();

private:
    // ------------页面切换相关------------
    QStackedWidget *m_stackedWidget = nullptr; // 页面容器
    //------切换按钮-------
    QPushButton *m_btnCalibrate = nullptr;     // 校准页面按钮
    QPushButton *m_btnMeasure2d = nullptr;       // 2d测量页面按钮
    QPushButton *m_btnMeasure3d = nullptr;       // 3d测量页面按钮
    //------页面容器-------
    QWidget *m_calibratePage = nullptr;        // 校准页容器
    QWidget *m_measurePage_2d = nullptr;          // 2d测量页容器
    QWidget *m_measurePage_3d = nullptr;          // 3d测量页容器

    // ------------页面的初始化函数声明------------
    void initCalibratePage();  // 校准页布局初始化
    void initMeasurePage_2d();    // 2d测量页布局初始化
    void initMeasurePage_3d();    // 3d测量页布局初始化

    void initPage();
    void initConnections();
    void initState();
    void initParas();

    //------------校准页------------
    QWidget *caliControlContainer; // 右侧控件区容器
    CameraSwitcher *cameraSwitchContainer; //左侧相机画面切换区
    QWidget *middleImgContainer; //中间区域图片
    ImgViewer *m_calibrateTopViewer = nullptr;    // 校准页上方图像控件
    ImgViewer *m_calibrateBottomViewer = nullptr; // 校准页下方图像控件

    CameraController *m_cameraController;// 相机
    ZoomControlWidget *m_zoomControlWidget;  // UI层
    RotatorControlWidget *m_rotatorControlWidget;  // 电机UI层
    MotorControlWidget *m_motorControlWidget; //平移台电机及移相电机
    BaslerExposuretimeWidget *m_baslerExposureWidget = nullptr;  //basler相机曝光时间设置
    MeasurementControlWidget *m_measurementControlWidget = nullptr;  // 执行测量控件

    QScrollArea *caliScrollArea; // 右侧滚动区域


    // ------------2d测量页------------
    QWidget *measure2dContainer = nullptr;  // 测量页2D图像容器
    QWidget *measure2dControlContainer = nullptr;   // 测量页右侧区域容器
    PageProcess2D* page_process2D;              //右侧控件区
    ElementCanvas *elementCanvas_process2DScene; // 2D画布

    //------------3d测量页------------
    QWidget *leftContainer;          // 3D测量页左侧显示区容器
    QWidget *rightControlContainer;  // 3D测量页右侧控件区容器
    PageProcess3D* page_process3D;  //右侧控件区
    SurfaceArea *m_surfaceArea;  // 3D 表面显示控件
    CurveChart *m_curveChart;  // 3D的曲线图表控件
    ElementCanvas *elementCanvas_process3DGroundPlain; // 3D画布


    //Target变量
    Target m_target;
    APDAResult m_apda_result;
    Topo2DStyle m_topo2d_style;
    bool m_show_topo2d_legend; // 更新标尺图例

};

#endif // MAIN_WINDOW_H
