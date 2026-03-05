#ifndef ROI_SELECTION_DIALOG_H
#define ROI_SELECTION_DIALOG_H

#include <QDialog>
#include <QImage>
#include <QRect>
#include <QPoint>

class QLabel;

/**
 * @brief ROISelectionWidget 用于显示图像并允许用户框选ROI的控件
 */
class ROISelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ROISelectionWidget(QWidget *parent = nullptr);

    void setImage(const QImage& image);
    QRect getSelectedROI() const;
    bool hasSelection() const { return m_hasSelection; }
    void clearSelection();

signals:
    void selectionChanged(const QRect& roi);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QImage m_originalImage;      // 原始图像
    QImage m_scaledImage;        // 缩放后的图像
    double m_scaleFactor = 1.0;  // 缩放比例
    QPoint m_imageOffset;        // 图像在控件中的偏移

    bool m_isSelecting = false;  // 是否正在框选
    bool m_hasSelection = false; // 是否有有效选择
    QPoint m_startPoint;         // 框选起始点（控件坐标）
    QPoint m_endPoint;           // 框选结束点（控件坐标）
    QRect m_selectionRect;       // 选择矩形（控件坐标）

    // 拖拽相关
    bool m_isDragging = false;   // 是否正在拖拽选框
    QPoint m_dragStartPos;       // 拖拽开始时的鼠标位置（控件坐标）
    QRect m_dragStartRect;       // 拖拽开始时选框的原始位置（控件坐标）

    void updateScaledImage();
    QPoint widgetToImage(const QPoint& widgetPos) const;
    QPoint imageToWidget(const QPoint& imagePos) const;
    QRect widgetRectToImageRect(const QRect& widgetRect) const;
    bool isPointInSelection(const QPoint& pos) const;
};

/**
 * @brief ROISelectionDialog ROI框选对话框
 *
 * 显示相机当前画面，允许用户框选感兴趣区域
 */
class ROISelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ROISelectionDialog(QWidget *parent = nullptr);

    /**
     * @brief setImage 设置要显示的图像
     * @param image 相机捕获的图像
     */
    void setImage(const QImage& image);

    /**
     * @brief getSelectedROI 获取用户选择的ROI（原始图像坐标）
     * @return ROI矩形
     */
    QRect getSelectedROI() const;

    /**
     * @brief hasValidSelection 检查是否有有效的选择
     * @return true表示有有效选择
     */
    bool hasValidSelection() const;

private slots:
    void onSelectionChanged(const QRect& roi);
    void onConfirmClicked();
    void onCancelClicked();
    void onClearClicked();

private:
    ROISelectionWidget* m_selectionWidget;
    QLabel* m_infoLabel;
    QPushButton* m_confirmBtn;
    QPushButton* m_cancelBtn;
    QPushButton* m_clearBtn;

    void setupUI();
    void updateInfoLabel();
};

#endif // ROI_SELECTION_DIALOG_H
