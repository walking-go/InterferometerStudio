#ifndef DRAWER_H
#define DRAWER_H

#include "canvas_namespace.h"
#include <QVector>

using namespace std;

/**
 * @brief The Drawer class 画家类
 */
class Drawer : public QObject
{
    Q_OBJECT
public:
    Drawer(ElementCanvas* canvas_, QVector<Element*>* elements_);
    ~Drawer();

    // 元素的设置
    // --------
    // 创建新元素
    void create(const ElementType& type_);
    // 设置光标位置
    void setCursor(const QPointF& pos_);
    // 更新光标位置，设置当前元素意向点
    void setIntention(const QPointF& pos_);
    // 设置当前元素下一点
    void setPoint(const QPointF& pos_);
    // 设置当前元素终止点
    void setEndPoint(const QPointF& pos_);
    // 撤销当前元素上一点
    void stepBack();
    // 清空当前正在绘制的元素
    void eraseCurrentElement();
    // 清除坐标pt_下的元素
    void eraseElementUnderPos(const QPointF& pt_);
    // 查找临近特征点
    bool ajacentFeaturePoint(QPointF& pt_);
    // 重新计算测量结果
    void onTargetUpdated();

    // 绘制
    // ---
    void draw(QPainter* painter_);

    // Get、Set类函数
    // -------------
    DrawerStatus getStatus() {return m_status; };
    int getAdjcentElementIdx();
    void setToolBox(const ToolBox& tool_box_){m_tool_box = tool_box_; };
    ToolBox getToolBox(){return m_tool_box; };

private:
    // 画布及画笔
    ElementCanvas* m_canvas;              /**< 画布*/
    ToolBox m_tool_box;                   /**< 画图工具箱*/
    QPixmap m_element_pix;                /**< 用来绘制元素的透明底片*/
    // 标志位及参数
    int m_max_n_element = 64;             /**< 允许的最多元素数量*/
    bool m_emphasizeClosest = true;       /**< 是否在显示时对光标靠近元素进行强调*/
    bool m_semitransFinished = true;      /**< 是否在绘制新元素时对已完成元素做半透明处理*/
    int m_alpha = 150;                    /**< 半透明的ɑ值*/
    // 元素
    Element* m_current_element = nullptr; /**< 当前未完成的新元素*/
    QVector<Element*>* m_elements = nullptr;     /**< 当前已完成的元素集合*/
    // 状态
    DrawerStatus m_status = Leisure;      /**< 状态*/
    QPointF m_cursor_pos = {0.0, 0.0};    /**< 当前光标位置, 物理坐标系*/
    int m_adjcent_element_idx = -1;       /**< 当前光标所临近的元素的序号*/

    // 更新状态，包括Drawer状态、current_element和adjcent_element_idx
    void refresh();

signals:
    /**
     * @brief currentElementUpdated
     * 当存在正在绘制的Element(即m_current_element)时，
     * 该Element的下一个意向点被修改是发出
     */
    void currentElementUpdated(const Element*);
    /**
     * @brief newElementSettled
     * 新元素构建完成
     */
    void newElementSettled();
};
#endif // DRAWER_H
