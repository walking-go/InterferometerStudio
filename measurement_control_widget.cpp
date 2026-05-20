#include "measurement_control_widget.h"
#include "ui_measurement_control.h"
#include "measurement_worker.h"
#include "basler_camera_thread.h"
#include "motor_controller.h"
#include "roi_selection_dialog.h"
#include "uddm_namespace.h"
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QFileDialog>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QThread>
#include <cmath>

MeasurementControlWidget::MeasurementControlWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MeasurementControlWidget)
{
    ui->setupUi(this);

    // 创建工作线程和Worker
    m_workerThread = new QThread(this);
    m_worker = new MeasurementWorker();
    m_worker->moveToThread(m_workerThread);

    // 连接Worker信号（使用QueuedConnection确保跨线程安全）
    connect(m_worker, &MeasurementWorker::progress,
            this, &MeasurementControlWidget::onWorkerProgress,
            Qt::QueuedConnection);
    connect(m_worker, &MeasurementWorker::finished,
            this, &MeasurementControlWidget::onWorkerFinished,
            Qt::QueuedConnection);
    connect(m_worker, &MeasurementWorker::failed,
            this, &MeasurementControlWidget::onWorkerFailed,
            Qt::QueuedConnection);
    connect(m_worker, &MeasurementWorker::firstImageReady,
            this, &MeasurementControlWidget::onWorkerFirstImageReady,
            Qt::QueuedConnection);

    // 线程结束时清理Worker
    connect(m_workerThread, &QThread::finished,
            m_worker, &QObject::deleteLater);

    // 启动工作线程
    m_workerThread->start();

    // 连接UI按钮
    connect(ui->btn_startMeasurement, &QPushButton::clicked,
            this, &MeasurementControlWidget::onStartButtonClicked);
    connect(ui->btn_browsePath, &QPushButton::clicked,
            this, &MeasurementControlWidget::onBrowsePathClicked);
    connect(ui->btn_selectROI, &QPushButton::clicked,
            this, &MeasurementControlWidget::onSelectROIClicked);

    // 历史图像处理按钮
    connect(ui->btn_browseHistoryPath, &QPushButton::clicked,
            this, &MeasurementControlWidget::onBrowseHistoryPathClicked);
    connect(ui->btn_processHistory, &QPushButton::clicked,
            this, &MeasurementControlWidget::onProcessHistoryClicked);

    // Zernike去除项按钮
    connect(ui->btn_zPiston, &QPushButton::toggled, this, &MeasurementControlWidget::onZernikeToggled);
    connect(ui->btn_zTilt, &QPushButton::toggled, this, &MeasurementControlWidget::onZernikeToggled);
    connect(ui->btn_zPower, &QPushButton::toggled, this, &MeasurementControlWidget::onZernikeToggled);
    connect(ui->btn_zAstigmatism, &QPushButton::toggled, this, &MeasurementControlWidget::onZernikeToggled);
    connect(ui->btn_zComa, &QPushButton::toggled, this, &MeasurementControlWidget::onZernikeToggled);
    connect(ui->btn_zSpherical, &QPushButton::toggled, this, &MeasurementControlWidget::onZernikeToggled);

    // 初始化保存路径
    initSavePath();

    // 初始化历史图像路径
    initHistoryPath();

    // 初始化UI状态
    updateUI();
    updateROIStatusLabel();
    updateStatus("待机");
    updateProgress(0);

    // 初始化历史图像处理相关变量
    m_historyImageCount = 0;

    // 预分配内存
    m_capturedImages.reserve(100);
}

MeasurementControlWidget::~MeasurementControlWidget()
{
    // 停止工作线程
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        if (!m_workerThread->wait(3000)) {
            m_workerThread->terminate();
            m_workerThread->wait();
        }
    }

    // 清理采集数据
    {
        QMutexLocker locker(&m_mutex);
        m_capturedImages.clear();
        m_capturedImages.shrink_to_fit();
    }

    delete ui;
}

void MeasurementControlWidget::setBaslerCameraThread(BaslerCameraThread* thread)
{
    // 断开旧连接，避免重复连接
    if (m_cameraThread) {
        disconnect(m_cameraThread, &BaslerCameraThread::fullResolutionImageReady,
                   this, &MeasurementControlWidget::onSingleImageCaptured);
    }

    m_cameraThread = thread;

    // 建立新连接 - 使用原始分辨率图像信号
    if (m_cameraThread) {
        connect(m_cameraThread, &BaslerCameraThread::fullResolutionImageReady,
                this, &MeasurementControlWidget::onSingleImageCaptured,
                Qt::QueuedConnection);
    }
}

void MeasurementControlWidget::setMotorController(MotorController* controller)
{
    m_motorController = controller;
}

void MeasurementControlWidget::onStartButtonClicked()
{
    switch (m_state) {
    case Idle:
        startCapture();
        break;
    case Capturing:
        stopCapture();
        break;
    case Processing:
        // 处理中不允许操作
        QMessageBox::information(this, "提示", "正在处理中，请等待...");
        break;
    }
}

void MeasurementControlWidget::startCapture()
{
    // 检查相机
    if (!m_cameraThread) {
        QMessageBox::warning(this, "警告", "Basler相机未连接");
        return;
    }

    // 检查是否设置了裁剪区域
    if (!m_hasManualROI) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(QStringLiteral("提示"));
        msgBox.setText(QStringLiteral("尚未设置裁剪区域，是否现在框选？"));
        msgBox.setInformativeText(QStringLiteral("点击[是]框选区域，点击[否]使用原始图像。"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int reply = msgBox.exec();

        if (reply == QMessageBox::Yes) {
            if (!showROISelectionDialog()) {
                return;  // 用户取消了框选
            }
        } else if (reply == QMessageBox::Cancel) {
            return;  // 用户取消操作
        }
        // 点击"否"则继续，使用原始图像（不裁剪）
    }

    // 检查电机控制器
    if (!m_motorController) {
        QMessageBox::warning(this, "警告", "电机控制器未连接");
        return;
    }

    if (!m_motorController->isConnected()) {
        QMessageBox::warning(this, "警告", "电机串口未连接，请先在电机控制面板中连接串口");
        return;
    }

    // 记录电机初始位置（用于测量完成后复位）
    m_initialPositionX = m_motorController->readX("X");
    m_initialPositionY = m_motorController->readX("Y");
    m_initialPositionZ = m_motorController->readX("Z");
    m_hasInitialPosition = true;


    // 设置裁剪器的固定ROI（如果有）
    if (m_hasManualROI) {
        m_imageCropper.setFixedROI(m_manualROI);
        qDebug() << "[MeasurementControlWidget] 使用手动框选的ROI:" << m_manualROI;
    } else {
        m_imageCropper.clearFixedROI();
        qDebug() << "[MeasurementControlWidget] 未设置ROI，使用原始图像";
    }

    // 初始化采集状态
    {
        QMutexLocker locker(&m_mutex);
        m_targetCount = ui->spinBox_imageCount->value();
        m_stepSizeUsteps = static_cast<int>(ui->doubleSpinBox_stepSize->value() * 2048.0);
        m_capturedImages.clear();
        m_capturedImages.reserve(m_targetCount);
        m_firstImage = QImage();
        m_currentStep = 0;
        m_waitingForImage = false;
        m_currentSessionPath.clear();
    }

    // 创建会话文件夹（如果设置了保存路径）
    if (!m_savePath.isEmpty()) {
        m_currentSessionPath = createSessionFolder(m_savePath);
    }

    // 开启测量采集模式（获取原始分辨率图像）
    m_cameraThread->setMeasurementCapturing(true);

    setState(Capturing);
    updateStatus(QString("采集中 (0/%1)").arg(m_targetCount));
    updateProgress(0);


    // 严格串行流水线：运动 → 稳定(200ms) → 采集 → 等待(100ms) → 运动
    // 发送第一次电机命令，等待稳定后采集
    sendMotorCommand(0);
    QTimer::singleShot(SETTLE_TIME_MS, this, &MeasurementControlWidget::onSettlingComplete);
}

void MeasurementControlWidget::stopCapture()
{
    // 关闭测量采集模式
    if (m_cameraThread) {
        m_cameraThread->setMeasurementCapturing(false);
    }

    {
        QMutexLocker locker(&m_mutex);
        m_capturedImages.clear();
        m_capturedImages.shrink_to_fit();
        m_firstImage = QImage();
        m_currentStep = 0;
        m_waitingForImage = false;
    }

    // 采集取消后也复位电机
    resetMotorsToInitialPosition();

    // 清除固定ROI，以便下次采集重新校准
    m_imageCropper.clearFixedROI();

    setState(Idle);
    updateStatus("采集已取消，电机已复位");
    updateProgress(0);

    qDebug() << "[MeasurementControlWidget] 采集已取消";
}

void MeasurementControlWidget::onSingleImageCaptured(const QImage& image)
{
    // 只在采集状态下且正在等待图像时处理
    if (m_state != Capturing || image.isNull() || !m_waitingForImage) {
        return;
    }

    QMutexLocker locker(&m_mutex);

    // 标记已收到图像
    m_waitingForImage = false;

    // 使用手动框选的ROI进行裁剪（如果已设置）
    QImage croppedImage;
    if (m_hasManualROI && m_imageCropper.isROILocked()) {
        croppedImage = m_imageCropper.crop(image);
    } else {
        // 未设置ROI，使用原始图像
        croppedImage = image;
    }

    // 保存第一张图像（用于original_img，保存裁剪后的）
    if (m_capturedImages.empty()) {
        m_firstImage = croppedImage.copy();
        // 发送测量开始信号，传递首张干涉图
        emit measurementStarted(m_firstImage);
    }

    // 异步保存裁剪后的图像到文件（不阻塞主线程）
    int imageIndex = static_cast<int>(m_capturedImages.size()) + 1;
    if (!m_currentSessionPath.isEmpty()) {
        QString filePath = m_currentSessionPath + "/captured_images/"
                           + QString("img_%1.bmp").arg(imageIndex, 3, 10, QChar('0'));
        QImage imageToSave = croppedImage.copy();  // 复制一份用于异步保存
        QtConcurrent::run([filePath, imageToSave]() {
            if (imageToSave.save(filePath)) {
                // qDebug() << "[MeasurementControlWidget] 异步保存图像:" << filePath;
            } else {
                // qDebug() << "[MeasurementControlWidget] 异步保存图像失败:" << filePath;
            }
        });
    }

    // 转换为灰度Mat存储（节省内存）
    cv::Mat grayMat = convertToGrayMat(croppedImage);
    if (grayMat.empty()) {
        qDebug() << "[MeasurementControlWidget] 图像转换失败，跳过";
        return;
    }


    m_capturedImages.push_back(std::move(grayMat));

    int current = static_cast<int>(m_capturedImages.size());
    int target = m_targetCount;
    m_currentStep = current;

    locker.unlock();

    // 更新UI（采集阶段占70%进度）
    int progress = current * 70 / target;
    updateProgress(progress);
    updateStatus(QString("采集中 (%1/%2)").arg(current).arg(target));

    // 严格串行：采集完成后等待POST_CAPTURE_WAIT_MS，再进入下一步
    QTimer::singleShot(POST_CAPTURE_WAIT_MS, this, &MeasurementControlWidget::onPostCaptureComplete);
}

void MeasurementControlWidget::startProcessing()
{
    // 关闭测量采集模式，恢复预览
    if (m_cameraThread) {
        m_cameraThread->setMeasurementCapturing(false);
    }

    // 清除固定ROI，以便下次采集重新校准
    m_imageCropper.clearFixedROI();

    setState(Processing);
    updateStatus("采集完成，准备处理...");

    // 将数据移交给Worker
    {
        QMutexLocker locker(&m_mutex);

        // 设置原始图像
        m_worker->setOriginalImage(m_firstImage);
        m_firstImage = QImage();  // 释放本地引用

        // 设置保存路径
        m_worker->setSavePath(m_currentSessionPath);

        // 移动图像数据给Worker（零拷贝）
        m_worker->setImages(std::move(m_capturedImages));
        m_capturedImages.clear();
        m_capturedImages.shrink_to_fit();
    }

    // 触发Worker处理（通过invokeMethod确保在Worker线程执行）
    QMetaObject::invokeMethod(m_worker, "process", Qt::QueuedConnection);
}

void MeasurementControlWidget::onWorkerProgress(int percent, const QString& message)
{
    // Worker进度（处理阶段占30%，即70-100%）
    int totalProgress = 70 + (percent * 30 / 100);
    updateProgress(totalProgress);
    updateStatus(message);
}

void MeasurementControlWidget::onWorkerFinished(const APDAResult& result)
{
    // 测量完成后自动复位电机到初始位置
    resetMotorsToInitialPosition();

    // 保存结果用于 Zernike 重处理
    m_lastResult = result.copy();

    setState(Idle);
    updateProgress(100);
    updateStatus("测量完成，电机已复位");

    // 发送结果信号（结果已包含original_img）
    emit measurementCompleted(result);
}

void MeasurementControlWidget::onWorkerFailed(const QString& errorMsg)
{
    // 处理失败后也复位电机
    resetMotorsToInitialPosition();

    setState(Idle);
    updateProgress(0);
    updateStatus("处理失败: " + errorMsg);

    qDebug() << "[MeasurementControlWidget] 处理失败:" << errorMsg;

    QMessageBox::warning(this, "测量失败", errorMsg);
}

void MeasurementControlWidget::setState(State state)
{
    m_state = state;
    updateUI();
}

void MeasurementControlWidget::updateUI()
{
    switch (m_state) {
    case Idle:
        ui->btn_startMeasurement->setText("执行测量");
        ui->btn_startMeasurement->setEnabled(true);
        ui->spinBox_imageCount->setEnabled(true);
        ui->doubleSpinBox_stepSize->setEnabled(true);
        break;
    case Capturing:
        ui->btn_startMeasurement->setText("停止采集");
        ui->btn_startMeasurement->setEnabled(true);
        ui->spinBox_imageCount->setEnabled(false);
        ui->doubleSpinBox_stepSize->setEnabled(false);
        break;
    case Processing:
        ui->btn_startMeasurement->setText("处理中...");
        ui->btn_startMeasurement->setEnabled(false);
        ui->spinBox_imageCount->setEnabled(false);
        ui->doubleSpinBox_stepSize->setEnabled(false);
        break;
    }
}

void MeasurementControlWidget::updateStatus(const QString& status)
{
    ui->label_status->setText("状态：" + status);
    emit statusChanged(status);
}

void MeasurementControlWidget::updateProgress(int percent)
{
    ui->progressBar_measurement->setValue(percent);
}


// void MeasurementControlWidget::moveMotorAndCapture()
// {
//     // 检查状态
//     if (m_state != Capturing) {
//         return;
//     }

//     // 获取当前步骤
//     int step = m_currentStep;
//     int target = m_targetCount;

//     if (step >= target) {
//         return;
//     }

//     // 单轴分步进退逻辑：
//     // 每6步为一个周期：X前进 -> Y前进 -> Z前进 -> X后退 -> Y后退 -> Z后退
//     // step 0: X轴前进1微米 -> 采集第1张
//     // step 1: Y轴前进1微米 -> 采集第2张
//     // step 2: Z轴前进1微米 -> 采集第3张
//     // step 3: X轴后退1微米 -> 采集第4张
//     // step 4: Y轴后退1微米 -> 采集第5张
//     // step 5: Z轴后退1微米 -> 采集第6张
//     // step 6: X轴前进1微米 -> 采集第7张 (新周期开始)
//     // ...以此类推

//     int phaseInCycle = step % 6;  // 周期内的阶段 (0-5)
//     QString axis;
//     bool moveForward;

//     switch (phaseInCycle) {
//     case 0:
//         axis = "X";
//         moveForward = true;
//         break;
//     case 1:
//         axis = "Y";
//         moveForward = true;
//         break;
//     case 2:
//         axis = "Z";
//         moveForward = true;
//         break;
//     case 3:
//         axis = "X";
//         moveForward = false;
//         break;
//     case 4:
//         axis = "Y";
//         moveForward = false;
//         break;
//     case 5:
//         axis = "Z";
//         moveForward = false;
//         break;
//     default:
//         axis = "X";
//         moveForward = true;
//         break;
//     }

//     // 1微米 = 2048微步（根据 moveXYZBeforeMeasure 的换算: 1微米 = 2048 usteps）
//     int usteps = moveForward ? 2048 : -2048;

//     updateStatus(QString("移动%1轴 (%2/%3)...").arg(axis).arg(step + 1).arg(target));

//     // 发送单轴电机移动命令
//     moveSingleAxis(axis, usteps);

//     // 使用定时器等待电机移动完成后再开始采集图像
//     // 单轴移动时间约200ms，等待300ms确保到位
//     QTimer::singleShot(300, this, [this]() {
//         if (m_state == Capturing) {
//             m_waitingForImage = true;
//         }
//     });
// }

void MeasurementControlWidget::moveSingleAxis(const QString& axis, int usteps)
{
    if (!m_motorController || !m_motorController->isConnected()) {
        qDebug() << "[MeasurementControlWidget] 电机控制器未连接";
        return;
    }

    // 发送单轴移动命令
    m_motorController->movedX(usteps, axis);
}

void MeasurementControlWidget::onSettlingComplete()
{
    // 电机稳定等待完成，触发图像采集
    if (m_state != Capturing) {
        return;
    }

    // 设置等待标志，相机线程会发送图像
    m_waitingForImage = true;
}

void MeasurementControlWidget::onPostCaptureComplete()
{
    // 采集后等待完成，检查是否继续
    if (m_state != Capturing) {
        return;
    }

    int current = static_cast<int>(m_capturedImages.size());
    int target = m_targetCount;

    if (current >= target) {
        qDebug() << "[MeasurementControlWidget] 全部采集完成，开始处理";
        startProcessing();
        return;
    }

    // 发送下一步电机命令，等待稳定后采集
    sendMotorCommand(current);  // current即为下一帧的step索引
    QTimer::singleShot(SETTLE_TIME_MS, this, &MeasurementControlWidget::onSettlingComplete);
}

void MeasurementControlWidget::sendMotorCommand(int step)
{
    if (step < 0 || step >= m_targetCount) {
        return;
    }

    // 单轴分步进退逻辑（与原moveMotorAndCapture相同）
    int phaseInCycle = step % 6;
    QString axis;
    bool moveForward;

    switch (phaseInCycle) {
    case 0:
        axis = "X";
        moveForward = true;
        break;
    case 1:
        axis = "Y";
        moveForward = true;
        break;
    case 2:
        axis = "Z";
        moveForward = true;
        break;
    case 3:
        axis = "X";
        moveForward = false;
        break;
    case 4:
        axis = "Y";
        moveForward = false;
        break;
    case 5:
        axis = "Z";
        moveForward = false;
        break;
    default:
        axis = "X";
        moveForward = true;
        break;
    }

    int usteps = moveForward ? m_stepSizeUsteps : -m_stepSizeUsteps;

    moveSingleAxis(axis, usteps);
}

void MeasurementControlWidget::onBrowsePathClicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "选择保存路径",
        m_savePath.isEmpty() ? QDir::homePath() : m_savePath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty()) {
        m_savePath = dir;
        ui->lineEdit_savePath->setText(m_savePath);
        // 保存到配置文件
        writeIniConfig(m_savePath);

    }
}

void MeasurementControlWidget::initSavePath()
{
    // 从配置文件读取保存路径
    m_savePath = readIniConfig();
    if (!m_savePath.isEmpty()) {
        ui->lineEdit_savePath->setText(m_savePath);
    }
}

void MeasurementControlWidget::initHistoryPath()
{
    // 从配置文件读取历史图像路径
    QString ini_file_path = QApplication::applicationDirPath() + "/config.ini";
    QSettings settings(ini_file_path, QSettings::IniFormat);
    m_historyImagePath = settings.value("Settings/HistoryImagePath").toString();

    if (!m_historyImagePath.isEmpty()) {
        ui->lineEdit_historyPath->setText(m_historyImagePath);

        // 统计图像数量
        m_historyImageCount = countImagesInFolder(m_historyImagePath);
        ui->label_historyImageCountValue->setText(QString::number(m_historyImageCount));

        // 根据图像数量更新样式
        if (m_historyImageCount > 0) {
            ui->label_historyImageCountValue->setStyleSheet("color: #4CAF50; font-weight: bold;");
        } else {
            ui->label_historyImageCountValue->setStyleSheet("color: #F44336;");
        }
    }
}

void MeasurementControlWidget::onSelectROIClicked()
{
    showROISelectionDialog();
}

bool MeasurementControlWidget::showROISelectionDialog()
{
    // 检查相机是否连接
    if (!m_cameraThread) {
        QMessageBox::warning(this, "警告", "Basler相机未连接，无法框选区域");
        return false;
    }

    // 临时开启测量采集模式获取一帧全分辨率图像
    m_cameraThread->setMeasurementCapturing(true);

    // 等待获取图像
    QImage capturedImage;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    auto connection = connect(m_cameraThread, &BaslerCameraThread::fullResolutionImageReady,
                              [&capturedImage, &loop](const QImage& image) {
                                  capturedImage = image.copy();
                                  loop.quit();
                              });

    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(3000);  // 3秒超时
    loop.exec();

    disconnect(connection);

    // 关闭测量采集模式
    m_cameraThread->setMeasurementCapturing(false);

    if (capturedImage.isNull()) {
        QMessageBox::warning(this, "警告", "无法获取相机图像，请确保相机正常工作");
        return false;
    }

    // 显示ROI选择对话框
    ROISelectionDialog dialog(this);
    dialog.setImage(capturedImage);

    if (dialog.exec() == QDialog::Accepted && dialog.hasValidSelection()) {
        m_manualROI = dialog.getSelectedROI();
        m_hasManualROI = true;
        updateROIStatusLabel();

        qDebug() << "[MeasurementControlWidget] 已设置手动ROI:" << m_manualROI;
        return true;
    }

    return false;
}

void MeasurementControlWidget::updateROIStatusLabel()
{
    if (m_hasManualROI) {
        ui->label_roiStatus->setText(QString("已设置 (%1x%2)")
                                     .arg(m_manualROI.width())
                                     .arg(m_manualROI.height()));
        ui->label_roiStatus->setStyleSheet("color: #4CAF50; font-weight: bold;");
    } else {
        ui->label_roiStatus->setText("未设置");
        ui->label_roiStatus->setStyleSheet("color: #999;");
    }
}

// ==================== 历史图像处理相关 ====================

void MeasurementControlWidget::onBrowseHistoryPathClicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "选择历史图像文件夹",
        m_historyImagePath.isEmpty() ? QDir::homePath() : m_historyImagePath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty()) {
        m_historyImagePath = dir;
        ui->lineEdit_historyPath->setText(m_historyImagePath);

        // 保存到配置文件
        QString ini_file_path = QApplication::applicationDirPath() + "/config.ini";
        QSettings settings(ini_file_path, QSettings::IniFormat);
        settings.setValue("Settings/HistoryImagePath", m_historyImagePath);

        // 统计图像数量
        m_historyImageCount = countImagesInFolder(m_historyImagePath);
        ui->label_historyImageCountValue->setText(QString::number(m_historyImageCount));

        // 根据图像数量更新样式
        if (m_historyImageCount > 0) {
            ui->label_historyImageCountValue->setStyleSheet("color: #4CAF50; font-weight: bold;");
        } else {
            ui->label_historyImageCountValue->setStyleSheet("color: #F44336;");
        }
    }
}

void MeasurementControlWidget::onProcessHistoryClicked()
{
    // 检查状态
    if (m_state != Idle) {
        QMessageBox::information(this, "提示", "请等待当前操作完成...");
        return;
    }

    // 检查路径
    if (m_historyImagePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择历史图像文件夹");
        return;
    }

    // 检查图像数量
    if (m_historyImageCount <= 0) {
        QMessageBox::warning(this, "警告", "所选文件夹中没有找到图像文件");
        return;
    }

    startHistoryProcessing();
}

void MeasurementControlWidget::startHistoryProcessing()
{
    setState(Processing);
    updateStatus("准备处理历史图像...");
    updateProgress(0);

    // 设置历史图像路径
    m_worker->setHistoryPath(m_historyImagePath, m_historyImageCount);

    // 触发Worker处理（通过invokeMethod确保在Worker线程执行）
    QMetaObject::invokeMethod(m_worker, "processFromPath", Qt::QueuedConnection);


}

void MeasurementControlWidget::onWorkerFirstImageReady(const QImage& firstImage)
{
    // 发送测量开始信号，传递首张干涉图
    emit measurementStarted(firstImage);
}

void MeasurementControlWidget::resetMotorsToInitialPosition()
{
    // 检查电机控制器连接
    if (!m_motorController || !m_motorController->isConnected()) {
        qDebug() << "[MeasurementControlWidget] 无法复位：电机控制器未连接";
        return;
    }

    // 检查是否已记录初始位置
    if (!m_hasInitialPosition) {
        qDebug() << "[MeasurementControlWidget] 无法复位：未记录初始位置";
        return;
    }

    // 读取当前位置
    int currentX = m_motorController->readX("X");
    int currentY = m_motorController->readX("Y");
    int currentZ = m_motorController->readX("Z");

    qDebug() << "[MeasurementControlWidget] 当前位置: X=" << currentX
             << " Y=" << currentY << " Z=" << currentZ;
    qDebug() << "[MeasurementControlWidget] 目标位置: X=" << m_initialPositionX
             << " Y=" << m_initialPositionY << " Z=" << m_initialPositionZ;

    // 检查是否需要移动
    bool needMove = (currentX != m_initialPositionX) ||
                    (currentY != m_initialPositionY) ||
                    (currentZ != m_initialPositionZ);

    if (!needMove) {
        qDebug() << "[MeasurementControlWidget] 电机已在初始位置，无需复位";
        return;
    }

    // 移动到初始位置（使用绝对定位）
    updateStatus("电机复位中...");

    m_motorController->moveToX(m_initialPositionX, "X");
    QThread::msleep(200);  // 等待X轴完成

    m_motorController->moveToX(m_initialPositionY, "Y");
    QThread::msleep(200);  // 等待Y轴完成

    m_motorController->moveToX(m_initialPositionZ, "Z");
    QThread::msleep(200);  // 等待Z轴完成

    qDebug() << "[MeasurementControlWidget] 电机已复位到初始位置";
}

// ==================== Zernike 去除项相关 ====================

uint32_t MeasurementControlWidget::getZernikeRemoveMask() const
{
    uint32_t mask = 0;
    if (ui->btn_zPiston->isChecked())      mask |= PhaseProcessor::ZERNIKE_PISTON;
    if (ui->btn_zTilt->isChecked())        mask |= PhaseProcessor::ZERNIKE_TILT;
    if (ui->btn_zPower->isChecked())       mask |= PhaseProcessor::ZERNIKE_POWER;
    if (ui->btn_zAstigmatism->isChecked()) mask |= PhaseProcessor::ZERNIKE_ASTIGMATISM;
    if (ui->btn_zComa->isChecked())        mask |= PhaseProcessor::ZERNIKE_COMA;
    if (ui->btn_zSpherical->isChecked())   mask |= PhaseProcessor::ZERNIKE_SPHERICAL;
    return mask;
}

void MeasurementControlWidget::onZernikeToggled()
{
    // Only reprocess if we have a valid result with unwrapped phase
    if (!m_lastResult.is_valid || m_lastResult.unwrapped_phase.empty()) {
        return;
    }
    reprocessZernike();
}

void MeasurementControlWidget::reprocessZernike()
{
    uint32_t removeMask = getZernikeRemoveMask();

    PhaseProcessor processor;
    cv::Mat newWavefront = processor.removeZernikeTerms(
        m_lastResult.unwrapped_phase, m_lastResult.phase_mask, removeMask);

    // Build updated result
    APDAResult updated = m_lastResult.copy();
    updated.wavefront = newWavefront;

    // Recalculate PV/RMS
    double minVal, maxVal;
    cv::minMaxLoc(newWavefront, &minVal, &maxVal, nullptr, nullptr, m_lastResult.phase_mask);
    updated.wavefront_pv = maxVal - minVal;

    cv::Scalar meanVal = cv::mean(newWavefront, m_lastResult.phase_mask);
    cv::Mat diff = newWavefront - meanVal[0];
    cv::Mat squared;
    cv::multiply(diff, diff, squared);
    cv::Scalar sumSq = cv::mean(squared, m_lastResult.phase_mask);
    updated.wavefront_rms = std::sqrt(sumSq[0]);

    updated.save_path = "";  // Don't re-save images on Zernike toggle

    emit measurementCompleted(updated);
}
