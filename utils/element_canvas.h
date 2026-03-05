#ifndef ELEMENT_CANVAS_H
#define ELEMENT_CANVAS_H

#include "uddm_namespace.h"
#include "canvas_namespace.h"
#include "img_viewer.h"

#include <QWidget>
#include <QPaintEvent>

using namespace std;

/**
 * @brief The ElementCanvas class
 * 元素（Element）画布类，用于绘制各种项目中自定义的元素
 * 该类主要用于存储已绘制完成的元素 和 响应系统上事件
 * 具体的元素管理、绘制等由成员m_element_drawer管理
 * 功能：
 * · 鼠标左键点击：添加新点
 * · 鼠标右键点击：撤销上一点
 * 特点：
 * · 随着鼠标移动，显示当前鼠标位置作为新点的预览效果
 * · 图像缩放时，绘制内容的大小在屏幕上的尺寸不变
 * · 绘制新元素时，已有元素透明处理
 * · 能够检测光标是否靠近某一元素，并在靠近时对该元素进行加粗显示
 */
class ElementCanvas : public ImgViewer
{
    Q_OBJECT
public:
    explicit ElementCanvas(QWidget *parent = nullptr);
    ~ElementCanvas();

protected:
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QPainter* m_canvas_painter;
    Drawer* m_MG_element_drawer;
    Drawer* m_AG_element_drawer;
    Drawer* m_current_drawer;

    QVector<Element*> m_MG_elements;
    QVector<Element*> m_AG_elements;
    ElementType m_current_element_type;   /**< 当前元素类型*/
    QString m_measurement_results;

    const Target* m_target;

    void setIntention(QPointF pos_);
    void adsorbCursorToFeaturePoint(QPointF& pt_);

signals:
    void elementUpdated(const Element* element_);
    void elementSelected(const Element* element_);
    void elementErased();
    void measurementResultsUpdated(QString results_);

public slots:
    // Set Get类函数
    // ------------
    // Type & Target
    void setType(ElementType type_);
    void setTarget(const Target& target_);
    void updateTarget(const Target& target_);
    void deleteElements(const QString& category_);
    const Target* getTarget(){return m_target; };
    // Color & LineWidth
    void setMGToolBox(const ToolBox& tool_box_);
    void setAGToolBox(const ToolBox& tool_box_);
    // Measurement Results
    QString getMeasurementResults();
    void refreshMeasurementResults();
};
#endif // ELEMENT_CANVAS_H
