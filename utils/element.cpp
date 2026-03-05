#include "element.h"
#include "element_canvas.h"
#include "drawing_funs.h"
#include <QPainter>

// Element
// --------
/**
 * @brief 设置意向点
 */
void Element::setIntention(const QPointF& pos_)
{
    switch(m_status){
    case Empty:
        m_points.push_back(pos_);
        break;
    case Draft:
        m_points.back() = pos_;
        break;
    case Settled:
        m_points.push_back(pos_);
        break;
    }
    m_status = Draft;
}

/**
 * @brief 设置下一点
 */
void Element::setPoint(const QPointF& pos_)
{
    switch(m_status){
    case Empty:
        m_points.push_back(pos_);
        break;
    case Draft:
        break;
    case Settled:
        break;
    }
    m_status = Settled;

    if((int)m_points.size() == m_N)
        m_finished = true;
}

/**
 * @brief 撤销上一点
 */
void Element::stepBack()
{
    switch(m_status){
    case Empty:
        break;
    case Draft:
        if(m_points.size() > 1)
            // 删除倒数第二个元素
            m_points.erase(m_points.end() - 2);
        break;
    case Settled:
        // 将最后一个点设置为草稿状态
        m_status = Draft;
        break;
    }
}

/**
 * @brief Element::aroundMyFeature
 * 检测物理坐标系下的点feature_pt_是否临近该元素的特征点
 * 如果临近，则返回true, 并将feature_pt_设置为该点坐标
 * exception_用来指定跳过检测的特征点的索引
 * n_exception_用来指定跳过检测的特征点的个数
 */
bool Element::aroundMyFeature(ElementCanvas *canvas_, QPointF &feature_pt_, int* exception_, int n_exception_)
{
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        // 跳过exception
        bool skip = false;
        for(int i = 0; i < n_exception_; i++)
            if(p_feature_pts - feature_pts.begin() == *(exception_ + i)){
                skip = true;
                break;
            }
        if(skip)
            continue;
        // 逻辑坐标系转物理坐标系
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
        QPointF cursor_pt = canvas_->logicalToPhysical(feature_pt_);
        // 判断是否临近
        if(aroundDot(*p_feature_pts, cursor_pt)){
            feature_pt_ = canvas_->physicalToLogical(*p_feature_pts);
            return true;
        }
    }
    return false;
}

/**
 * @brief Element::aroundMyFeature
 * 上一个函数的包装
 */
bool Element::aroundMyFeature(ElementCanvas *canvas_, QPointF &feature_pt_)
{
    return aroundMyFeature(canvas_, feature_pt_, m_feature_exceptions, m_n_feature_exceptions);
}
