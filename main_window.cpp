#include "main_window.h"
#include <QMessageBox>
#include <QDebug>
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QResizeEvent>
#include "page_process2d.h"
#include "uddm_namespace.h"

// 构造函数实现：初始化UI + 连接信号槽
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), caliControlContainer(nullptr)
{
    //变量初始化
    initParas();
    //创建页面
    initPage();
    //信号槽连接
    initConnections();
    //状态初始化
    initState();
}

MainWindow::~MainWindow()
{
    // delete m_apdaProcessor;
}

//----------初始化----------
//变量初始化
void MainWindow::initParas(){
    m_apda_result = APDAResult();
    m_topo2d_style = PseudoColor;
    m_show_topo2d_legend = false;  //是否展示标尺图例
    m_target = Target();

}

//初始化状态
void MainWindow::initState(){
    onTargetUpdated();
    setTopo2DStyle(m_topo2d_style);

}

//信号槽连接
void MainWindow::initConnections(){

    // 页面切换
    connect(m_btnCalibrate, &QPushButton::clicked, this, &MainWindow::onCalibratePage_Clicked);
    connect(m_btnMeasure2d, &QPushButton::clicked, this, &MainWindow::onMeasurePage_2d_Clicked);
    connect(m_btnMeasure3d, &QPushButton::clicked, this, &MainWindow::onMeasurePage_3d_Clicked);

    //相机
    connect(m_cameraController, &CameraController::cameraError, this, [](const QString &msg) {
        qDebug() << "[相机错误]:" << msg;
    });
    connect(cameraSwitchContainer, &CameraSwitcher::switchButtonClicked,
            m_cameraController, &CameraController::switchMainSub);
    connect(cameraSwitchContainer, &CameraSwitcher::subImageClicked,
            m_cameraController, &CameraController::switchMainSub);

    //page2d
    connect(page_process2D, SIGNAL(setElementType(ElementType)),
            elementCanvas_process2DScene, SLOT(setType(ElementType)));
    connect(page_process2D, SIGNAL(setMGToolBox(ToolBox)),
            elementCanvas_process2DScene, SLOT(setMGToolBox(ToolBox)));
    connect(page_process2D, SIGNAL(setAGToolBox(ToolBox)),
            elementCanvas_process2DScene, SLOT(setAGToolBox(ToolBox)));
    connect(page_process2D, SIGNAL(deleteElements(QString)),
            elementCanvas_process2DScene, SLOT(deleteElements(QString)));
    connect(page_process2D, SIGNAL(setTopo2DStyle(Topo2DStyle)),
            this, SLOT(setTopo2DStyle(Topo2DStyle)));
    connect(page_process2D, SIGNAL(scalarStateSetted(bool)),
            page_process3D, SLOT(onScalarStateSetted(bool)));
    connect(page_process2D, SIGNAL(refreshTarget()),
            this, SLOT(onTargetUpdated()));
    connect(elementCanvas_process2DScene, SIGNAL(measurementResultsUpdated(QString)), page_process2D, SLOT(updateMeasurementResults(QString)));

    //page3d
    connect(page_process3D, SIGNAL(setElementType(ElementType)), elementCanvas_process3DGroundPlain, SLOT(setType(ElementType)));
    connect(page_process3D, SIGNAL(setAGToolBox(ToolBox)), elementCanvas_process3DGroundPlain, SLOT(setAGToolBox(ToolBox)));
    connect(page_process3D, SIGNAL(deleteElements(QString)), elementCanvas_process3DGroundPlain, SLOT(deleteElements(QString)));
    connect(page_process3D, SIGNAL(setTopo2DStyle(Topo2DStyle)), this, SLOT(setTopo2DStyle(Topo2DStyle)));
    connect(page_process3D, SIGNAL(scalarStateSetted(bool)), page_process2D, SLOT(onScalarStateSetted(bool)));
    connect(page_process3D, SIGNAL(refreshTarget()), this, SLOT(onTargetUpdated()));

    connect(elementCanvas_process3DGroundPlain, SIGNAL(elementUpdated(const Element*)), m_surfaceArea, SLOT(setElement(const Element*)));
    connect(elementCanvas_process3DGroundPlain, SIGNAL(elementSelected(const Element*)),m_surfaceArea, SLOT(setElement(const Element*)));
    connect(elementCanvas_process3DGroundPlain, SIGNAL(elementErased()), m_surfaceArea, SLOT(clearElement()));
    connect(elementCanvas_process3DGroundPlain, SIGNAL(elementUpdated(const Element*)), m_curveChart, SLOT(setElement(const Element*)));
    connect(elementCanvas_process3DGroundPlain, SIGNAL(elementSelected(const Element*)), m_curveChart, SLOT(setElement(const Element*)));
    connect(elementCanvas_process3DGroundPlain, SIGNAL(elementErased()), m_curveChart, SLOT(clearElement()));

    // 连接2D、3D页面的标尺状态变更信号
    connect(page_process2D, &PageProcess2D::legendVisibilityChanged,
            this, [this](bool visible) {
                m_show_topo2d_legend = visible;
            });
    connect(page_process3D, &PageProcess3D::legendVisibilityChanged,
            this, [this](bool visible) {
                m_show_topo2d_legend = visible;
            });

    // 连接测量控件的测量完成信号
    connect(m_measurementControlWidget, &MeasurementControlWidget::measurementCompleted,
            this, &MainWindow::onResultSolved);

    // 连接到3D页面的统计信息更新
    connect(m_measurementControlWidget, &MeasurementControlWidget::measurementCompleted,
            this, [this](const APDAResult& result) {
                if (page_process3D) {
                    page_process3D->updateStatistics(result.wavefront_pv, result.wavefront_rms);
                }
            });

    // 连接测量控件的测量开始信号（更新校准页图像）
    connect(m_measurementControlWidget, &MeasurementControlWidget::measurementStarted,
            this, &MainWindow::updateCalibrateBottomViewer);

}

//创建页面
void MainWindow::initPage(){
    // 1. 窗口基础设置
    setMinimumSize(800, 400);
    resize(1600, 800);
    setWindowTitle("校准及测量系统");

    // 2. 创建主容器和主布局（包含按钮栏和页面容器）
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 3. 添加页面切换按钮（顶部按钮栏）
    QWidget *btnBar = new QWidget();
    btnBar->setStyleSheet("background-color: #FFFFFF;");  // 白色背景
    btnBar->setFixedHeight(50); // 固定按钮栏高度
    QHBoxLayout *btnLayout = new QHBoxLayout(btnBar);
    btnLayout->setContentsMargins(20, 8, 20, 8);
    btnLayout->setSpacing(15);

    m_btnCalibrate = new QPushButton("对准实时页面");
    m_btnMeasure2d = new QPushButton("2D测量页面");
    m_btnMeasure3d = new QPushButton("3D测量页面");

    // 设置按钮为checkable，实现选中状态
    m_btnCalibrate->setCheckable(true);
    m_btnMeasure2d->setCheckable(true);
    m_btnMeasure3d->setCheckable(true);

    // 设置按钮样式，与page_process2d风格一致
    QString btnStyle = R"(
        QPushButton {
            font: bold 12pt "微软雅黑";
            color: #457B9D;
            background-color: transparent;
            border: 2px solid #457B9D;
            border-radius: 5px;
            padding: 0px 30px;
        }
        QPushButton:hover {
            background-color: rgba(69, 123, 157, 0.1);
        }
        QPushButton:pressed {
            background-color: rgba(69, 123, 157, 0.2);
        }
        QPushButton:checked {
            background-color: #457B9D;
            color: #FFFFFF;
            border: 2px solid #457B9D;
        }
    )";
    m_btnCalibrate->setStyleSheet(btnStyle);
    m_btnMeasure2d->setStyleSheet(btnStyle);
    m_btnMeasure3d->setStyleSheet(btnStyle);

    // 左右拉伸项，确保按钮居中
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnCalibrate);
    btnLayout->addWidget(m_btnMeasure2d);
    btnLayout->addWidget(m_btnMeasure3d);
    btnLayout->addStretch();
    mainLayout->addWidget(btnBar);

    // 4. 创建页面容器（QStackedWidget）
    m_stackedWidget = new QStackedWidget();
    mainLayout->addWidget(m_stackedWidget);

    // 5. 初始化校准页和测量页，并添加到容器
    initCalibratePage();     // 现有校准页逻辑
    initMeasurePage_2d();    // 初始化2d测量页
    initMeasurePage_3d();    // 初始化3d测量页

    m_stackedWidget->addWidget(m_calibratePage);
    m_stackedWidget->addWidget(m_measurePage_2d);
    m_stackedWidget->addWidget(m_measurePage_3d);

    // 默认显示校准页
    m_stackedWidget->setCurrentWidget(m_calibratePage);
    m_btnCalibrate->setChecked(true);  // 默认选中校准页面按钮
}

//----------三个页面----------
//校准页面
void MainWindow::initCalibratePage() {
    // 创建校准页容器
    m_calibratePage = new QWidget();
    QHBoxLayout *caliMainLayout = new QHBoxLayout(m_calibratePage);
    caliMainLayout->setContentsMargins(0, 0, 0, 0);
    caliMainLayout->setSpacing(0);

    //校准页面布局分配
    // 1. 左侧区域（2份）：两个相机画面切换
    cameraSwitchContainer = new CameraSwitcher(m_calibratePage);
    cameraSwitchContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    caliMainLayout->addWidget(cameraSwitchContainer,2);

    // 2. 中间区域（1份）：上下两张图片
    middleImgContainer = new QWidget(m_calibratePage);
    QVBoxLayout* middleLayout = new QVBoxLayout(middleImgContainer);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(5); // 两张图片之间的间距

    // 上方ImgViewer（用于显示2D测量结果图像）
    m_calibrateTopViewer = new ImgViewer(middleImgContainer);
    m_calibrateTopViewer->setText("2D图像（待测量）");
    m_calibrateTopViewer->setAlignment(Qt::AlignCenter);
    m_calibrateTopViewer->setStyleSheet("QLabel { background-color: black; color: #FFFFFF; font-size: 24px; border: 1px solid #ccc; }");

    // 下方ImgViewer（用于显示首张干涉图）
    m_calibrateBottomViewer = new ImgViewer(middleImgContainer);
    m_calibrateBottomViewer->setText("干涉图（待采集）");
    m_calibrateBottomViewer->setAlignment(Qt::AlignCenter);
    m_calibrateBottomViewer->setStyleSheet("QLabel { background-color: black; color: #FFFFFF; font-size: 24px; border: 1px solid #ccc; }");

    // 添加到布局
    middleLayout->addWidget(m_calibrateTopViewer);
    middleLayout->addWidget(m_calibrateBottomViewer);

    middleImgContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    caliMainLayout->addWidget(middleImgContainer, 1);

    // 3. 右侧区域（1份）：控件区
    caliControlContainer = new QWidget(m_calibratePage);
    caliControlContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    caliMainLayout->addWidget(caliControlContainer,1);

    QVBoxLayout *caliControlLayout = new QVBoxLayout(caliControlContainer);
    caliControlLayout->setContentsMargins(0, 0, 0, 0);
    caliControlLayout->setSpacing(0);

    caliScrollArea = new QScrollArea(caliControlContainer);
    caliScrollArea->setWidgetResizable(true);
    caliScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    caliControlLayout->addWidget(caliScrollArea);

    QWidget *caliScrollContent = new QWidget(caliScrollArea);
    caliScrollArea->setWidget(caliScrollContent);

    QVBoxLayout *caliScrollLayout = new QVBoxLayout(caliScrollContent);
    caliScrollLayout->setContentsMargins(10, 10, 10, 10);
    caliScrollLayout->setSpacing(15);

    // 左侧：相机控制
    m_cameraController = new CameraController(this);
    m_cameraController->setImageWidget(cameraSwitchContainer);
    m_cameraController->startSmallCamera(1);
    m_cameraController->startBaslerCamera(0);

    //右侧控件内容

    // 1. 变倍物镜面板
    m_zoomControlWidget = new ZoomControlWidget();
    m_zoomControlWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    caliScrollLayout->addWidget(m_zoomControlWidget);

    // 2. 毛玻璃旋转电机面板
    m_rotatorControlWidget = new RotatorControlWidget();
    m_rotatorControlWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    caliScrollLayout->addWidget(m_rotatorControlWidget);

    // 3. 平移台移动电机面板
    m_motorControlWidget = new MotorControlWidget();
    m_motorControlWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    caliScrollLayout->addWidget(m_motorControlWidget);

    // 4. Basler相机曝光时间控制面板
    m_baslerExposureWidget = new BaslerExposuretimeWidget(m_cameraController->baslerCameraThread());
    m_baslerExposureWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    caliScrollLayout->addWidget(m_baslerExposureWidget);

    // 5. 执行测量控件
    m_measurementControlWidget = new MeasurementControlWidget();
    m_measurementControlWidget->setBaslerCameraThread(m_cameraController->baslerCameraThread());
    m_measurementControlWidget->setMotorController(m_motorControlWidget->getMotorController());
    m_measurementControlWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    caliScrollLayout->addWidget(m_measurementControlWidget);

    caliScrollLayout->addStretch(1);


}

//2d测量页
void MainWindow::initMeasurePage_2d() {
    m_measurePage_2d = new QWidget();
    QHBoxLayout *measure2dMainLayout = new QHBoxLayout(m_measurePage_2d);
    measure2dMainLayout->setContentsMargins(0, 0, 0, 0);
    measure2dMainLayout->setSpacing(0); // 确保页面区域之间无间距

    //2d测量页面布局分配
    // 1. 2d图像显示区
    measure2dContainer = new QWidget();
    measure2dContainer->setStyleSheet("");
    QVBoxLayout *twoDLayout = new QVBoxLayout(measure2dContainer);
    twoDLayout->setContentsMargins(0, 0, 0, 0);
    twoDLayout->setSpacing(0);

    elementCanvas_process2DScene = new ElementCanvas(measure2dContainer);
    elementCanvas_process2DScene->setTarget(m_target);
    elementCanvas_process2DScene->setStyleSheet("background-color: transparent;"); // 画布背景透明，不遮挡图片
    elementCanvas_process2DScene->setMouseTracking(true); // 允许鼠标跟踪，便于绘图
    elementCanvas_process2DScene->raise(); // 保证绘图层在最上面
    twoDLayout->addWidget(elementCanvas_process2DScene);
    measure2dMainLayout->addWidget(measure2dContainer,3);


    // 2. 右侧控件区
    page_process2D = new PageProcess2D(m_show_topo2d_legend);
    page_process2D->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    measure2dMainLayout->addWidget(page_process2D,1);


}


//3d测量页面
void MainWindow::initMeasurePage_3d() {
    m_measurePage_3d = new QWidget();
    QHBoxLayout *measure3dMainLayout = new QHBoxLayout(m_measurePage_3d);
    measure3dMainLayout->setContentsMargins(0, 0, 0, 0);
    measure3dMainLayout->setSpacing(0);

    // 1. 左侧区域（3/4）
    leftContainer = new QWidget(m_measurePage_3d);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);


    //上半部分:surfaceArea + elementCanvas_process3DGroundPlain
    QWidget *upperContainer = new QWidget(leftContainer);
    QHBoxLayout *upperLayout = new QHBoxLayout(upperContainer);
    upperLayout->setContentsMargins(0, 0, 0, 0);
    upperLayout->setSpacing(0);

    // 左上：surfaceArea 三维可剖面图
    m_surfaceArea = new SurfaceArea(upperContainer);
    m_surfaceArea->setTarget(m_target);
    upperLayout->addWidget(m_surfaceArea, 1);

    // 右上：elementCanvas_process3DGroundPlain 平面便于确定剖面位置
    elementCanvas_process3DGroundPlain = new ElementCanvas(upperContainer);
    elementCanvas_process3DGroundPlain->setTarget(m_target);
    elementCanvas_process3DGroundPlain->setStyleSheet("background-color: transparent;"); // 画布背景透明，不遮挡图片
    elementCanvas_process3DGroundPlain->setMouseTracking(true); // 允许鼠标跟踪，便于绘图
    upperLayout->addWidget(elementCanvas_process3DGroundPlain, 1);  // 与左上区域平分宽度

    leftLayout->addWidget(upperContainer, 1); // 占上半部分

    //下半部分:curveChart 剖面的曲线
    QWidget *lowerContainer = new QWidget(leftContainer);
    QVBoxLayout *lowerLayout = new QVBoxLayout(lowerContainer);
    lowerLayout->setContentsMargins(0, 0, 0, 0);
    lowerLayout->setSpacing(0);

    // 创建CurveChart实例并添加到下半部分
    m_curveChart = new CurveChart(lowerContainer);  // 实例化曲线图表
    m_curveChart->setTarget(m_target);  // 关联目标数据（用于显示高度曲线）
    lowerLayout->addWidget(m_curveChart);  // 将图表添加到布局


    leftLayout->addWidget(lowerContainer, 1); // 占下半部分

    measure3dMainLayout->addWidget(leftContainer, 3); // 左侧占3/4

    // 2. 右侧控件区（1/4）
    page_process3D = new PageProcess3D(m_show_topo2d_legend);
    page_process3D->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    measure3dMainLayout->addWidget(page_process3D,1);


}

//----------切换页面----------
// 切换到校准页
void MainWindow::onCalibratePage_Clicked() {
    m_stackedWidget->setCurrentWidget(m_calibratePage);
    m_btnCalibrate->setChecked(true);
    m_btnMeasure2d->setChecked(false);
    m_btnMeasure3d->setChecked(false);
}

// 切换到2d测量页
void MainWindow::onMeasurePage_2d_Clicked() {
    m_stackedWidget->setCurrentWidget(m_measurePage_2d);
    m_btnCalibrate->setChecked(false);
    m_btnMeasure2d->setChecked(true);
    m_btnMeasure3d->setChecked(false);
}

// 切换到3d测量页
void MainWindow::onMeasurePage_3d_Clicked() {
    m_stackedWidget->setCurrentWidget(m_measurePage_3d);
    m_btnCalibrate->setChecked(false);
    m_btnMeasure2d->setChecked(false);
    m_btnMeasure3d->setChecked(true);
}


//----------更新Target----------
//用APDA结果更新m_Target
void MainWindow::onResultSolved(const APDAResult& result_)
{
    //深拷贝
    m_apda_result = result_.copy();

    // 算法结果异常时
    if(!m_apda_result.is_valid){
        if(!m_target.original_img.isNull())
            m_target.original_img = m_apda_result.original_img.copy();
        return;
    }

    // 算法结果正常时
    m_target.original_img = m_apda_result.original_img.copy();
    m_target.x_scope = m_apda_result.wavefront.cols * m_apda_result.real_per_pix;
    m_target.y_scope = m_apda_result.wavefront.rows * m_apda_result.real_per_pix;
    m_target.z_scope = m_apda_result.wavefront_pv;
    m_target.real_per_pix = m_apda_result.real_per_pix;
    m_target.real_per_gray_level = m_target.z_scope / 65535;

    QImage topoImg = matToQImage(m_apda_result.wavefront, true);
    setTopo(topoImg);//true会触发更新信号onTargetUpdated()


    // 保存结果图像到指定路径
    saveResultImages(m_apda_result.save_path, m_apda_result.original_img, m_target.topo2d);

    // 保存PV/RMS数据到TXT文件（与captured_images同级目录）
    savePVRMSData(m_apda_result.save_path, m_apda_result.wavefront_pv, m_apda_result.wavefront_rms);

    // 测量完成后更新校准页 topViewer（显示2D地形图）
    updateCalibrateTopViewer();
}


/**
 * @brief setTopo
 * 专用于设置m_target的topo
 * m_target.topo = topo_.copy()数据深拷贝
 */
void MainWindow::setTopo(const QImage &topo_, bool boardcast_)
{
    qDebug() << "[MainWindow::setTopo] topo_ isNull:" << topo_.isNull()
             << "m_target.original_img isNull:" << m_target.original_img.isNull()
             << "boardcast:" << boardcast_;

    // 检查传入图像
    if(!topo_.isNull())
        // 检查target状态
        if(!m_target.original_img.isNull()){
            // 更新target
            // 对img_做stretch道理是需要的，但在SHS算法已经做了本处理，所以这里不重复
            m_target.topo = topo_.copy();
            m_target.topo2d = processTopo2D(m_target.topo, m_topo2d_style);
            m_target.topo2d_legend = prcoessLegendTag(m_topo2d_style, m_target.z_scope, (double)m_target.topo2d.width() / m_target.topo2d.height());
            // 广播target更新事件
            if(boardcast_)
                onTargetUpdated();
        } else {
            qDebug() << "[MainWindow::setTopo] 跳过：m_target.original_img为空";
        }
    else {
        qDebug() << "[MainWindow::setTopo] 跳过：topo_为空";
    }
}

void MainWindow::setTopo2DStyle(Topo2DStyle style_, bool boardcast_)
{
    // 界面状态联动
    page_process2D->onTopo2DStyleSetted(style_);
    page_process3D->onTopo2DStyleSetted(style_);
    // 类型设置
    m_topo2d_style = style_;
    // topo为空时返回
    if(m_target.topo2d.isNull())
        return;
    // topo不为空时
    m_target.topo2d = processTopo2D(m_target.topo, style_);
    m_target.topo2d_legend = prcoessLegendTag(m_topo2d_style, m_target.z_scope, (double)m_target.topo2d.width() / m_target.topo2d.height());
    // 广播target更新事件
    if(boardcast_)
        onTargetUpdated();
}

//更新Target
void MainWindow::onTargetUpdated()
{
    // 更新 2D canvas
    elementCanvas_process2DScene->setTarget(m_target);
    elementCanvas_process2DScene->update();

    // 更新 3D surface
    m_surfaceArea->setTarget(m_target);
    m_surfaceArea->update();

    // 更新 3D ground plain
    elementCanvas_process3DGroundPlain->setTarget(m_target);
    elementCanvas_process3DGroundPlain->update();

    // 更新曲线图
    m_curveChart->setTarget(m_target);
    m_curveChart->update();

    //更新标尺图例
    if(m_show_topo2d_legend){
        elementCanvas_process2DScene->updateTag(m_target.topo2d_legend);
        elementCanvas_process3DGroundPlain->updateTag(m_target.topo2d_legend);
    }else{
        QImage img = QImage();
        elementCanvas_process2DScene->updateTag(img);
        elementCanvas_process3DGroundPlain->updateTag(img);
    }
}

//更新BottomViewer
void MainWindow::updateCalibrateBottomViewer(const QImage& firstInterferogram)
{
    qDebug() << "[MainWindow::onMeasurementStarted] 收到测量开始信号";

    // 将首张干涉图加载到 bottomViewer（采集第一张图像后立即更新）
    if (m_calibrateBottomViewer && !firstInterferogram.isNull()) {
        m_calibrateBottomViewer->setImg(firstInterferogram);

    }

}

//更新TopViewer
void MainWindow::updateCalibrateTopViewer()
{
    // 测量完成后将2D地形图加载到 topViewer
    if (m_calibrateTopViewer && !m_target.topo2d.isNull()) {
        m_calibrateTopViewer->setImg(m_target.topo2d);

    }
}


