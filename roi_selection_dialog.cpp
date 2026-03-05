#include "roi_selection_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QDebug>

// ==================== ROISelectionWidget ====================

ROISelectionWidget::ROISelectionWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(400, 400);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void ROISelectionWidget::setImage(const QImage& image)
{
    m_originalImage = image;
    clearSelection();
    updateScaledImage();
    update();
}

void ROISelectionWidget::clearSelection()
{
    m_isSelecting = false;
    m_hasSelection = false;
    m_selectionRect = QRect();
    update();
}

QRect ROISelectionWidget::getSelectedROI() const
{
    if (!m_hasSelection) {
        return QRect();
    }
    return widgetRectToImageRect(m_selectionRect);
}

void ROISelectionWidget::updateScaledImage()
{
    if (m_originalImage.isNull()) {
        m_scaledImage = QImage();
        m_scaleFactor = 1.0;
        m_imageOffset = QPoint(0, 0);
        return;
    }

    // 计算缩放比例，保持宽高比
    double scaleX = static_cast<double>(width()) / m_originalImage.width();
    double scaleY = static_cast<double>(height()) / m_originalImage.height();
    m_scaleFactor = qMin(scaleX, scaleY);

    // 缩放图像
    int scaledWidth = static_cast<int>(m_originalImage.width() * m_scaleFactor);
    int scaledHeight = static_cast<int>(m_originalImage.height() * m_scaleFactor);
    m_scaledImage = m_originalImage.scaled(scaledWidth, scaledHeight,
                                            Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation);

    // 计算居中偏移
    m_imageOffset = QPoint((width() - scaledWidth) / 2,
                           (height() - scaledHeight) / 2);
}

QPoint ROISelectionWidget::widgetToImage(const QPoint& widgetPos) const
{
    if (m_scaleFactor <= 0) return QPoint();

    int imgX = static_cast<int>((widgetPos.x() - m_imageOffset.x()) / m_scaleFactor);
    int imgY = static_cast<int>((widgetPos.y() - m_imageOffset.y()) / m_scaleFactor);

    // 限制在图像范围内
    imgX = qBound(0, imgX, m_originalImage.width() - 1);
    imgY = qBound(0, imgY, m_originalImage.height() - 1);

    return QPoint(imgX, imgY);
}

QPoint ROISelectionWidget::imageToWidget(const QPoint& imagePos) const
{
    int widgetX = static_cast<int>(imagePos.x() * m_scaleFactor) + m_imageOffset.x();
    int widgetY = static_cast<int>(imagePos.y() * m_scaleFactor) + m_imageOffset.y();
    return QPoint(widgetX, widgetY);
}

QRect ROISelectionWidget::widgetRectToImageRect(const QRect& widgetRect) const
{
    QPoint topLeft = widgetToImage(widgetRect.topLeft());
    QPoint bottomRight = widgetToImage(widgetRect.bottomRight());

    // 确保矩形有效
    int x = qMin(topLeft.x(), bottomRight.x());
    int y = qMin(topLeft.y(), bottomRight.y());
    int w = qAbs(bottomRight.x() - topLeft.x()) + 1;
    int h = qAbs(bottomRight.y() - topLeft.y()) + 1;

    // 强制正方形：取较小边长，确保宽高完全相等
    int size = qMin(w, h);
    w = size;
    h = size;

    // 限制在图像范围内
    x = qMax(0, x);
    y = qMax(0, y);
    w = qMin(w, m_originalImage.width() - x);
    h = qMin(h, m_originalImage.height() - y);

    // 再次确保正方形（防止边界限制导致不对称）
    size = qMin(w, h);
    w = size;
    h = size;

    return QRect(x, y, w, h);
}

bool ROISelectionWidget::isPointInSelection(const QPoint& pos) const
{
    if (!m_hasSelection || m_selectionRect.isEmpty()) {
        return false;
    }
    return m_selectionRect.contains(pos);
}

void ROISelectionWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 填充背景
    painter.fillRect(rect(), QColor(50, 50, 50));

    // 绘制图像
    if (!m_scaledImage.isNull()) {
        painter.drawImage(m_imageOffset, m_scaledImage);
    }

    // 绘制选择框
    if (m_hasSelection || m_isSelecting) {
        // 半透明遮罩
        QColor maskColor(0, 0, 0, 100);
        QRect imageRect(m_imageOffset, m_scaledImage.size());

        // 绘制四周遮罩
        if (!m_selectionRect.isEmpty()) {
            // 上
            painter.fillRect(QRect(imageRect.left(), imageRect.top(),
                                   imageRect.width(), m_selectionRect.top() - imageRect.top()),
                             maskColor);
            // 下
            painter.fillRect(QRect(imageRect.left(), m_selectionRect.bottom() + 1,
                                   imageRect.width(), imageRect.bottom() - m_selectionRect.bottom()),
                             maskColor);
            // 左
            painter.fillRect(QRect(imageRect.left(), m_selectionRect.top(),
                                   m_selectionRect.left() - imageRect.left(), m_selectionRect.height()),
                             maskColor);
            // 右
            painter.fillRect(QRect(m_selectionRect.right() + 1, m_selectionRect.top(),
                                   imageRect.right() - m_selectionRect.right(), m_selectionRect.height()),
                             maskColor);
        }

        // 绘制选择框边框
        QPen pen(QColor(0, 255, 0), 2, Qt::SolidLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(m_selectionRect);

        // 绘制角点
        int cornerSize = 8;
        painter.setBrush(QColor(0, 255, 0));
        QRect corners[] = {
            QRect(m_selectionRect.topLeft() - QPoint(cornerSize/2, cornerSize/2), QSize(cornerSize, cornerSize)),
            QRect(m_selectionRect.topRight() - QPoint(cornerSize/2, cornerSize/2), QSize(cornerSize, cornerSize)),
            QRect(m_selectionRect.bottomLeft() - QPoint(cornerSize/2, cornerSize/2), QSize(cornerSize, cornerSize)),
            QRect(m_selectionRect.bottomRight() - QPoint(cornerSize/2, cornerSize/2), QSize(cornerSize, cornerSize))
        };
        for (const auto& corner : corners) {
            painter.drawRect(corner);
        }
    }

    // 绘制提示文字
    if (!m_hasSelection && !m_isSelecting) {
        painter.setPen(Qt::white);
        painter.setFont(QFont("Microsoft YaHei", 14));
        painter.drawText(rect(), Qt::AlignCenter, "请在图像上框选裁剪区域");
    }
}

void ROISelectionWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_originalImage.isNull()) {
        // 检查是否点击在已有选框内，如果是则开始拖拽
        if (m_hasSelection && isPointInSelection(event->pos())) {
            m_isDragging = true;
            m_dragStartPos = event->pos();
            m_dragStartRect = m_selectionRect;
            setCursor(Qt::ClosedHandCursor);
            qDebug() << "[ROISelectionWidget] 开始拖拽选框";
        }
        // 否则开始新的框选
        else {
            m_isSelecting = true;
            m_hasSelection = false;  // 重新框选时清除之前的选择
            m_startPoint = event->pos();
            m_endPoint = event->pos();
            m_selectionRect = QRect(m_startPoint, m_endPoint).normalized();
        }
        update();
    }
    else if (event->button() == Qt::RightButton && (m_isSelecting || m_hasSelection)) {
        // 右键重新框选：取消当前框选状态或已有选择
        m_isSelecting = false;
        m_hasSelection = false;
        m_isDragging = false;
        m_selectionRect = QRect();
        setCursor(Qt::CrossCursor);
        update();
        qDebug() << "[ROISelectionWidget] 右键取消框选，可以重新框选";
    }
}

void ROISelectionWidget::mouseMoveEvent(QMouseEvent *event)
{
    // 拖拽模式：移动已有选框
    if (m_isDragging) {
        // 计算鼠标移动的偏移量
        QPoint offset = event->pos() - m_dragStartPos;

        // 计算新的选框位置
        QRect newRect = m_dragStartRect.translated(offset);

        // 确保选框不超出图像范围
        QRect imageRect(m_imageOffset, m_scaledImage.size());

        // 调整X方向位置
        if (newRect.left() < imageRect.left()) {
            newRect.moveLeft(imageRect.left());
        }
        if (newRect.right() > imageRect.right()) {
            newRect.moveRight(imageRect.right());
        }

        // 调整Y方向位置
        if (newRect.top() < imageRect.top()) {
            newRect.moveTop(imageRect.top());
        }
        if (newRect.bottom() > imageRect.bottom()) {
            newRect.moveBottom(imageRect.bottom());
        }

        m_selectionRect = newRect;
        update();

        emit selectionChanged(widgetRectToImageRect(m_selectionRect));
    }
    // 框选模式：绘制正方形选框
    else if (m_isSelecting) {
        m_endPoint = event->pos();

        // 限制在图像范围内
        QRect imageRect(m_imageOffset, m_scaledImage.size());
        m_endPoint.setX(qBound(imageRect.left(), m_endPoint.x(), imageRect.right()));
        m_endPoint.setY(qBound(imageRect.top(), m_endPoint.y(), imageRect.bottom()));

        // 计算正方形框选区域
        int dx = m_endPoint.x() - m_startPoint.x();
        int dy = m_endPoint.y() - m_startPoint.y();

        // 取较小的边长作为正方形边长
        int size = qMin(qAbs(dx), qAbs(dy));

        // 根据鼠标移动方向确定正方形的终点
        int endX = m_startPoint.x() + (dx >= 0 ? size : -size);
        int endY = m_startPoint.y() + (dy >= 0 ? size : -size);

        // 确保正方形不超出图像范围
        endX = qBound(imageRect.left(), endX, imageRect.right());
        endY = qBound(imageRect.top(), endY, imageRect.bottom());

        // 重新计算实际可用的正方形大小（考虑边界限制）
        int actualDx = endX - m_startPoint.x();
        int actualDy = endY - m_startPoint.y();
        int actualSize = qMin(qAbs(actualDx), qAbs(actualDy));

        // 最终确定正方形终点
        m_endPoint.setX(m_startPoint.x() + (actualDx >= 0 ? actualSize : -actualSize));
        m_endPoint.setY(m_startPoint.y() + (actualDy >= 0 ? actualSize : -actualSize));

        m_selectionRect = QRect(m_startPoint, m_endPoint).normalized();
        update();

        emit selectionChanged(widgetRectToImageRect(m_selectionRect));
    }
    // 悬停模式：更新光标
    else if (m_hasSelection) {
        if (isPointInSelection(event->pos())) {
            setCursor(Qt::OpenHandCursor);
        } else {
            setCursor(Qt::CrossCursor);
        }
    }
}

void ROISelectionWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 拖拽模式：结束拖拽
        if (m_isDragging) {
            m_isDragging = false;

            // 恢复光标
            if (isPointInSelection(event->pos())) {
                setCursor(Qt::OpenHandCursor);
            } else {
                setCursor(Qt::CrossCursor);
            }

            qDebug() << "[ROISelectionWidget] 拖拽结束，新位置:" << widgetRectToImageRect(m_selectionRect);
            update();
            emit selectionChanged(widgetRectToImageRect(m_selectionRect));
        }
        // 框选模式：完成框选
        else if (m_isSelecting) {
            m_isSelecting = false;

            // 检查选择是否有效（最小尺寸）
            QRect imageROI = widgetRectToImageRect(m_selectionRect);
            if (imageROI.width() >= 64 && imageROI.height() >= 64) {
                m_hasSelection = true;
                qDebug() << "[ROISelectionWidget] 选择完成，ROI:" << imageROI;
            } else {
                m_hasSelection = false;
                m_selectionRect = QRect();
                qDebug() << "[ROISelectionWidget] 选择区域过小，已取消";
            }

            update();
            emit selectionChanged(imageROI);
        }
    }
}

void ROISelectionWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updateScaledImage();

    // 如果有选择，需要重新计算选择框的显示位置
    // 这里简化处理：调整窗口大小时清除选择
    if (m_hasSelection) {
        // 保留选择，但需要重新绘制
        // 由于我们存储的是控件坐标，需要转换
        // 简化：清除选择
        // clearSelection();
    }
}

// ==================== ROISelectionDialog ====================

ROISelectionDialog::ROISelectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

void ROISelectionDialog::setupUI()
{
    setWindowTitle("选择裁剪区域");
    setMinimumSize(800, 700);
    resize(900, 750);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 提示标签
    QLabel* tipLabel = new QLabel("请在图像上拖动鼠标框选需要保留的干涉图区域：", this);
    tipLabel->setStyleSheet("font-size: 14px; color: #333;");
    mainLayout->addWidget(tipLabel);

    // 图像选择控件
    m_selectionWidget = new ROISelectionWidget(this);
    m_selectionWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(m_selectionWidget, 1);

    connect(m_selectionWidget, &ROISelectionWidget::selectionChanged,
            this, &ROISelectionDialog::onSelectionChanged);

    // 信息标签
    m_infoLabel = new QLabel("未选择区域", this);
    m_infoLabel->setStyleSheet("font-size: 12px; color: #666; padding: 5px;");
    m_infoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_infoLabel);

    // 按钮行
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);

    m_clearBtn = new QPushButton("清除选择", this);
    m_clearBtn->setMinimumHeight(35);
    connect(m_clearBtn, &QPushButton::clicked, this, &ROISelectionDialog::onClearClicked);
    buttonLayout->addWidget(m_clearBtn);

    buttonLayout->addStretch();

    m_cancelBtn = new QPushButton("取消", this);
    m_cancelBtn->setMinimumHeight(35);
    m_cancelBtn->setMinimumWidth(100);
    connect(m_cancelBtn, &QPushButton::clicked, this, &ROISelectionDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelBtn);

    m_confirmBtn = new QPushButton("确认", this);
    m_confirmBtn->setMinimumHeight(35);
    m_confirmBtn->setMinimumWidth(100);
    m_confirmBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }"
                                 "QPushButton:hover { background-color: #45a049; }"
                                 "QPushButton:disabled { background-color: #cccccc; }");
    m_confirmBtn->setEnabled(false);
    connect(m_confirmBtn, &QPushButton::clicked, this, &ROISelectionDialog::onConfirmClicked);
    buttonLayout->addWidget(m_confirmBtn);

    mainLayout->addLayout(buttonLayout);
}

void ROISelectionDialog::setImage(const QImage& image)
{
    m_selectionWidget->setImage(image);
    updateInfoLabel();
}

QRect ROISelectionDialog::getSelectedROI() const
{
    return m_selectionWidget->getSelectedROI();
}

bool ROISelectionDialog::hasValidSelection() const
{
    return m_selectionWidget->hasSelection();
}

void ROISelectionDialog::onSelectionChanged(const QRect& roi)
{
    Q_UNUSED(roi);
    updateInfoLabel();
}

void ROISelectionDialog::updateInfoLabel()
{
    if (m_selectionWidget->hasSelection()) {
        QRect roi = m_selectionWidget->getSelectedROI();
        m_infoLabel->setText(QString("已选择区域: 位置(%1, %2), 尺寸 %3 x %4 像素")
                             .arg(roi.x()).arg(roi.y())
                             .arg(roi.width()).arg(roi.height()));
        m_infoLabel->setStyleSheet("font-size: 12px; color: #4CAF50; padding: 5px; font-weight: bold;");
        m_confirmBtn->setEnabled(true);
    } else {
        m_infoLabel->setText("未选择区域（最小选择尺寸: 64x64像素）");
        m_infoLabel->setStyleSheet("font-size: 12px; color: #666; padding: 5px;");
        m_confirmBtn->setEnabled(false);
    }
}

void ROISelectionDialog::onConfirmClicked()
{
    if (m_selectionWidget->hasSelection()) {
        accept();
    }
}

void ROISelectionDialog::onCancelClicked()
{
    reject();
}

void ROISelectionDialog::onClearClicked()
{
    m_selectionWidget->clearSelection();
    updateInfoLabel();
}
