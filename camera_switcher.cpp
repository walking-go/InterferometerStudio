#include "camera_switcher.h"
#include <QStyleOption>
#include <QPainter>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QDateTime>
#include "utils/img_viewer.h"
CameraSwitcher::CameraSwitcher(QWidget *parent) : QWidget(parent)
{
    setupUI();
    setStyleSheet("background-color: black;");
    setMinimumSize(0, 0);

}

CameraSwitcher::~CameraSwitcher()
{
    delete mainViewer;
    delete subLabel;
    delete switchButton;
}

// 接收相机1的帧
void CameraSwitcher::setMainCameraFrame(const QImage& frame) {
    m_MaincameraFrame = frame;
    updateFrames(); // 刷新画面
}

// 接收相机2的帧
void CameraSwitcher::setSubCameraFrame(const QImage& frame) {
    m_SubcameraFrame = frame;
    updateFrames(); // 刷新画面
}


// 切换主副相机
void CameraSwitcher::switchMainSub() {
    m_isMainCamera = !m_isMainCamera;
    updateFrames(); // 切换后立即刷新
}


// 实现公共接口4：显示错误
void CameraSwitcher::showError(const QString& msg)
{
    mainViewer->setImg(QImage()); // 清空主图
    subLabel->setText(msg);
}


// 刷新主副图（替代原updateMainFrame和updateSubFrame）
void CameraSwitcher::updateFrames() {

    // 若50ms内已刷新过，则跳过
    static qint64 lastRefresh = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastRefresh < 50) return;
    lastRefresh = now;

    // 主图显示逻辑 - 使用ImgViewer的updateImg保持缩放和位移状态
    QImage mainFrame = m_isMainCamera ? m_MaincameraFrame : m_SubcameraFrame;
    if (!mainFrame.isNull()) {
        mainViewer->updateImg(mainFrame);
    } else {
        mainViewer->setImg(QImage()); // 清空显示
    }

    // 副图显示逻辑（与主图相反）
    int subW = width() * 0.25;
    int subH = height() * 0.25;
    QImage subFrame = m_isMainCamera ? m_SubcameraFrame : m_MaincameraFrame;
    if (!subFrame.isNull()) {
        subLabel->setPixmap(QPixmap::fromImage(
            subFrame.scaled(subW, subH, Qt::KeepAspectRatio)
            ));
    } else {
        subLabel->setText(m_isMainCamera ? "相机2无帧" : "相机1无帧");
    }

    // 调整副图位置
    if (!subLabel->pixmap(Qt::ReturnByValue).isNull()) {
        subLabel->resize(subLabel->pixmap(Qt::ReturnByValue).size());
    }
    subLabel->move(width() - subLabel->width(), height() - subLabel->height());
}

// UI事件转发（仅发送信号，不处理）
bool CameraSwitcher::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == subLabel && event->type() == QEvent::MouseButtonPress) {
        if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
            emit subImageClicked(); // 向外发送“副图被点击”信号
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void CameraSwitcher::setupUI()
{
    mainViewer = new ImgViewer(this);
    mainViewer->setMinimumSize(0, 0);

    subLabel = new QLabel(this);
    subLabel->setAlignment(Qt::AlignCenter);
    subLabel->setStyleSheet("border: 2px solid gray;");
    subLabel->setMinimumSize(0, 0);
    subLabel->installEventFilter(this);

    switchButton = new QToolButton(this);
    switchButton->setText("切换画面");
    switchButton->setMinimumSize(80, 30);
    QString switchButtonStyle = R"(
    QToolButton {
        font: bold 10pt "微软雅黑";
        color: #457B9D;
        background-color: #FFFFFF;
        border: 2px solid #457B9D;
        border-radius: 4px;
        padding: 4px 15px;
    }
)";
    switchButton->setStyleSheet(switchButtonStyle);

    // 按钮点击→发送信号（不处理逻辑）
    connect(switchButton, &QToolButton::clicked, this, &CameraSwitcher::switchButtonClicked);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mainViewer);
    setLayout(layout);
    subLabel->setParent(this);
}

// 窗口缩放刷新
void CameraSwitcher::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    mainViewer->resize(size());
    updateFrames();
}

// 支持样式表
void CameraSwitcher::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
