#include "mg_element.h"
#include "element_canvas.h"
#include "drawing_funs.h"
#include <math.h>
#include <QPainter>

// MM_DeltaXY
// ----------
MM_DeltaXY::MM_DeltaXY(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 3;
    // m_feature_points:
    // 0 - m_points[0]
    // 1 - m_points[1]
    // 2 - 测量指示箭头一端,与m_points[0]连线与 (m_points[0], m_points[1])相垂直
    // 3 - 测量指示箭头一端,与m_points[1]连线与 (m_points[0], m_points[1])相垂直
    m_feature_points = QVector<QPointF>(4);
    // 跳过检测的特征点
    m_n_feature_exceptions = 2;
    m_feature_exceptions = new int[2]{2, 3};
    // 描述
    m_unit = QObject::tr("um");
    m_description = "平面距离";
}

void MM_DeltaXY::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch(m_points.size()){
    case 1:
        m_feature_points[0] = m_points[0];
        break;
    case 2:
        m_feature_points[1] = m_points[1];
        break;
    case 3:
        QLineF indicator_line_1 = verticalLineSegment(m_points[2], QLineF(m_points[1], m_points[0]));
        QLineF indicator_line_2 = verticalLineSegment(m_points[2], QLineF(m_points[0], m_points[1]));
        m_feature_points[2] = indicator_line_1.p2();
        m_feature_points[3] = indicator_line_2.p2();
        break;
    }

    calculateResult();
}

void MM_DeltaXY::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }

    // 3. 绘图
    // -------
    switch(pts.size()){
    case 1:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        break;
    case 2:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.basic_pen, Type_CrossDot);
        drawLine(painter_, pts[0], pts[1], tool_box_.measurement_pen, Type_DoubleArrowedLine);
        drawText(painter_, pts[1], m_label, QLineF(pts[0], pts[1]).angle(), tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    case 3:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.basic_pen, Type_CrossDot);
        // 3
        drawLine(painter_, pts[0], feature_pts[2], tool_box_.assist_pen, Type_SimpleLine);
        drawLine(painter_, pts[1], feature_pts[3], tool_box_.assist_pen, Type_SimpleLine);
        drawLine(painter_, feature_pts[2], feature_pts[3], tool_box_.measurement_pen, Type_DoubleArrowedLine);
        drawText(painter_, pts[2], m_label, QLineF(pts[0], pts[1]).angle(), tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool MM_DeltaXY::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox& tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
    if(isFinished()==false)
    {
        return-1;
    }
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    QPointF cursor_pos = canvas_->logicalToPhysical(cursor_pos_);
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(feature_pts[2], feature_pts[3]), cursor_pos)
        || aroundLine(QLineF(feature_pts[0], feature_pts[2]), cursor_pos)
        || aroundLine(QLineF(feature_pts[1], feature_pts[3]), cursor_pos)
        || aroundText(pts[2], m_label, QLineF(pts[0], pts[1]).angle(), tool_box_.basic_font, cursor_pos))
        return true;
    else
        return false;
}

qreal MM_DeltaXY::calculateResult(bool recalculate_)
{
    if((m_points.size() == 2) || (m_points.size() >= 2 && recalculate_)){
        QPointF vec = m_points[1] - m_points[0];
        qreal dx = abs(vec.x()) * m_target->x_scope / m_target->topo2d.width();
        qreal dy = abs(vec.y()) * m_target->y_scope / m_target->topo2d.height();
        m_result = sqrt(dx*dx + dy*dy);
        m_label = m_prefix + m_description + QString::number(m_result, 'g', DIST_PRECISION + numOfIntergerDigit(m_result)) + m_unit;
    }
    return m_result;
}

// MM_DeltaZ
// ---------
MM_DeltaZ::MM_DeltaZ(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 3;
    // m_feature_points:
    // 0 - m_points[0]
    // 1 - m_points[1]
    // 2 - 测量指示箭头一端,与m_points[0]连线与 (m_points[0], m_points[1])相垂直
    // 3 - 测量指示箭头一端,与m_points[1]连线与 (m_points[0], m_points[1])相垂直
    m_feature_points = QVector<QPointF>(4);
    // 跳过检测的特征点
    m_n_feature_exceptions = 2;
    m_feature_exceptions = new int[2]{2, 3};
    // 描述
    m_unit = QObject::tr("um");
    m_description = "垂直落差";
}

void MM_DeltaZ::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch(m_points.size()){
    case 1:
        // 1
        m_feature_points[0] = m_points[0];
        break;
    case 2:
        m_feature_points[1] = m_points[1];
        break;
    case 3:
        QLineF indicator_line_1 = verticalLineSegment(m_points[2], QLineF(m_points[1], m_points[0]));
        QLineF indicator_line_2 = verticalLineSegment(m_points[2], QLineF(m_points[0], m_points[1]));
        m_feature_points[2] = indicator_line_1.p2();
        m_feature_points[3] = indicator_line_2.p2();
        break;
    }
    calculateResult();
}

void MM_DeltaZ::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }

    // 3. 绘图
    // -------
    switch(pts.size()){
    case 1:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        break;
    case 2:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.basic_pen, Type_CrossDot);
        drawLine(painter_, pts[0], pts[1], tool_box_.measurement_pen, Type_DoubleArrowedLine);
        drawText(painter_, pts[1], m_label, QLineF(pts[0], pts[1]).angle(), tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    case 3:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.basic_pen, Type_CrossDot);
        // 3
        drawLine(painter_, pts[0], feature_pts[2], tool_box_.assist_pen, Type_SimpleLine);
        drawLine(painter_, pts[1], feature_pts[3], tool_box_.assist_pen, Type_SimpleLine);
        drawLine(painter_, feature_pts[2], feature_pts[3], tool_box_.measurement_pen, Type_DoubleArrowedLine);
        drawText(painter_, pts[2], m_label, QLineF(pts[0], pts[1]).angle(), tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();

}

bool MM_DeltaZ::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
    if(isFinished()==false)
    {
        return-1;
    }
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    QPointF cursor_pos = canvas_->logicalToPhysical(cursor_pos_);
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(feature_pts[2], feature_pts[3]), cursor_pos)
        || aroundLine(QLineF(feature_pts[0], feature_pts[2]), cursor_pos)
        || aroundLine(QLineF(feature_pts[1], feature_pts[3]), cursor_pos)
        || aroundText(pts[2], m_label, QLineF(pts[0], pts[1]).angle(), tool_box_.basic_font, cursor_pos))
        return true;
    else
        return false;
}

qreal MM_DeltaZ::calculateResult(bool recalculate_)
{
    if(m_points.size() == 2 || (m_points.size() >= 2 && recalculate_)){
        ushort* pixels = (ushort*)m_target->topo.bits();
        int width = m_target->topo.width();
        int x1 = m_points[0].toPoint().x();
        int y1 = m_points[0].toPoint().y();
        int x2 = m_points[1].toPoint().x();
        int y2 = m_points[1].toPoint().y();
        ushort z1 = pixels[y1 * width + x1];
        ushort z2 = pixels[y2 * width + x2];
        m_result = (double)(z1 - z2) * m_target->z_scope / 65535;
        m_result = abs(m_result);
        m_label = m_prefix + m_description + QString::number(m_result, 'g', DIST_PRECISION + numOfIntergerDigit(m_result)) + m_unit;
    }
    return m_result;
}

// MM_Radius
// ---------
MM_Radius::MM_Radius(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 4;
    // m_feature_points
    // 0 - 圆心
    // 1 - 测量指示线端点，也是Text的绘制点
    m_feature_points = QVector<QPointF>(2);
    m_n_feature_exceptions = 1;
    m_feature_exceptions = new int[1]{1};
    // 描述
    m_unit = QObject::tr("um");
    m_description = "半径";
}

void MM_Radius::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch(m_points.size()){
    case 3:
        m_feature_points[0] = circleCenter(m_points[0], m_points[1], m_points[2]);
        break;
    case 4:
        m_feature_points[1] = verticalPoint(m_points[3], QLineF(m_points[2], m_feature_points[0]));
        break;
    }
    calculateResult();
}

void MM_Radius::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }

    // 3. 绘图
    // -------
    switch(pts.size()){
    case 1:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        break;
    case 2:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.basic_pen, Type_CrossDot);
        break;
    case 3:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.basic_pen, Type_CrossDot);
        // 3
        drawDot(painter_, pts[2], tool_box_.basic_pen, Type_CrossDot);
        drawDot(painter_, feature_pts[0], tool_box_.basic_pen, Type_CrossDot);
        drawCircle(painter_, feature_pts[0], pts[2], tool_box_.main_pen);
        drawLine(painter_, pts[2], feature_pts[0], tool_box_.measurement_pen, Type_DoubleArrowedLine);
        drawText(painter_, pts[2], m_label, QLineF(pts[2], feature_pts[0]).angle(), tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    case 4:
        // 1
        // 2
        // 3
        drawDot(painter_, feature_pts[0], tool_box_.basic_pen, Type_CrossDot);
        drawCircle(painter_, feature_pts[0], pts[2], tool_box_.main_pen);
        // 4
        QPen pen_extended_line(tool_box_.measurement_pen);
        pen_extended_line.setWidth(max(pen_extended_line.width() - DELTA_LINE_WIDTH, 1));
        drawLine(painter_, feature_pts[0], feature_pts[1], pen_extended_line, Type_SimpleLine);
        drawLine(painter_, feature_pts[0], pts[2], tool_box_.measurement_pen, Type_DoubleArrowedLine);
        drawText(painter_, feature_pts[1], m_label, QLineF(pts[2], feature_pts[0]).angle(), tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool MM_Radius::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox& tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
    if(isFinished()==false)
    {
        return-1;
    }
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    QPointF cursor_pos = canvas_->logicalToPhysical(cursor_pos_);
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(feature_pts[0], feature_pts[1]), cursor_pos)
        || aroundCircle(feature_pts[0], dist(feature_pts[0], pts[2]), cursor_pos)
        || aroundDot(feature_pts[0], cursor_pos)
        || aroundText(feature_pts[1], m_label, QLineF(pts[2], feature_pts[0]).angle(), tool_box_.basic_font, cursor_pos))
        return true;
    else
        return false;
}

qreal MM_Radius::calculateResult(bool recalculate_)
{
    if(m_points.size() == 3){
        QPointF vec = m_points[2] - m_feature_points[0];
        qreal dx = abs(vec.x()) * m_target->x_scope / m_target->topo2d.width();
        qreal dy = abs(vec.y()) * m_target->y_scope / m_target->topo2d.height();
        m_result = sqrt(dx*dx + dy*dy);
        m_label = m_prefix + m_description + QString::number(m_result, 'g', DIST_PRECISION + numOfIntergerDigit(m_result)) + m_unit;
    }
    return m_result;
}

// MM_Diameter
// -----------
MM_Diameter::MM_Diameter(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 4;
    // m_feature_points
    // 0 - 圆心
    // 1 - 测量指示线端点，也是Text的绘制点
    // 2 - 测量指示线端点，点1的对策点
    m_feature_points = QVector<QPointF>(2);
    m_n_feature_exceptions = 1;
    m_feature_exceptions = new int[1]{1};
    // 描述
    m_unit = QObject::tr("um");
    m_description = "直径";
}

void MM_Diameter::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch(m_points.size()){
    case 3:
        m_feature_points[0] = circleCenter(m_points[0], m_points[1], m_points[2]);
        break;
    case 4:
        m_feature_points[1] = verticalPoint(m_points[3], QLineF(m_points[2], m_feature_points[0]));
        break;
    }

    calculateResult();
}

void MM_Diameter::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }

    // 3. 绘图
    // -------
    switch(pts.size()){
    case 1:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        break;
    case 2:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.basic_pen, Type_CrossDot);
        break;
    case 3:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.basic_pen, Type_CrossDot);
        // 3
        drawDot(painter_, pts[2], tool_box_.basic_pen, Type_CrossDot);
        drawDot(painter_, feature_pts[0], tool_box_.basic_pen, Type_CrossDot);
        drawCircle(painter_, feature_pts[0], pts[2], tool_box_.main_pen);
        drawLine(painter_, pts[2], 2 * feature_pts[0] - pts[2], tool_box_.measurement_pen, Type_DoubleArrowedLine);
        drawText(painter_, pts[2], m_label, QLineF(pts[2], feature_pts[0]).angle(), tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    case 4:
        // 1
        // 2
        // 3
        drawDot(painter_, feature_pts[0], tool_box_.basic_pen, Type_CrossDot);
        drawCircle(painter_, feature_pts[0], pts[2], tool_box_.main_pen);
        // 4
        QPen pen_extended_line(tool_box_.measurement_pen);
        pen_extended_line.setWidth(max(pen_extended_line.width() - DELTA_LINE_WIDTH, 1));
        drawLine(painter_, feature_pts[0], feature_pts[1], pen_extended_line, Type_SimpleLine);
        drawLine(painter_, pts[2], 2 * feature_pts[0] - pts[2], tool_box_.measurement_pen, Type_DoubleArrowedLine);
        drawText(painter_, feature_pts[1], m_label, QLineF(pts[2], feature_pts[0]).angle(), tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool MM_Diameter::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
    if(isFinished()==false)
    {
        return-1;
    }
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    QPointF cursor_pos = canvas_->logicalToPhysical(cursor_pos_);
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(2 * feature_pts[0] - pts[2], feature_pts[1]), cursor_pos)
        || aroundCircle(feature_pts[0], dist(feature_pts[0], pts[2]), cursor_pos)
        || aroundDot(feature_pts[0], cursor_pos)
        || aroundText(feature_pts[1], m_label, QLineF(pts[2], feature_pts[0]).angle(), tool_box_.basic_font, cursor_pos))
        return true;
    else
        return false;
}

qreal MM_Diameter::calculateResult(bool recalculate_)
{
    if(m_points.size() == 3 || (m_points.size() >= 3 && recalculate_)){
        QPointF vec = m_points[2] - m_feature_points[0];
        qreal dx = abs(vec.x()) * m_target->x_scope / m_target->topo2d.width();
        qreal dy = abs(vec.y()) * m_target->y_scope / m_target->topo2d.height();
        m_result = sqrt(dx*dx + dy*dy) * 2;
        m_label = m_prefix + m_description + QString::number(m_result, 'g', DIST_PRECISION + numOfIntergerDigit(m_result)) + m_unit;
    }
    return m_result;
}

// MM_Radian
// ---------
MM_Radian::MM_Radian(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 4;
    // m_feature_points
    // 0 - m_points[0]
    // 1 - m_points[1]
    // 2 - m_points[2]
    m_feature_points = QVector<QPointF>(3);
    m_n_feature_exceptions = 0;
    m_feature_exceptions = nullptr;
    // 描述
    m_unit = QObject::tr("um");
    m_description = "角度";
}

void MM_Radian::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch(m_points.size()){
    case 1:
        m_feature_points[0] = m_points[0];
        break;
    case 2:
        // 2
        m_feature_points[1] = m_points[1];
        break;
    case 3:
        m_feature_points[2] = m_points[2];
        break;
    }

    calculateResult();
}

void MM_Radian::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }

    // 3. 绘图
    // -------
    switch(pts.size()){
    case 1:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        break;
    case 2:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.basic_pen, Type_CrossDot);
        drawLine(painter_, pts[0], pts[1], tool_box_.main_pen, Type_SimpleLine);
        break;
    case 3:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.basic_pen, Type_CrossDot);
        // 3
        drawDot(painter_, pts[2], tool_box_.basic_pen, Type_CrossDot);
        drawLine(painter_, pts[0], pts[2], tool_box_.main_pen, Type_SimpleLine);
        drawLine(painter_, pts[1], pts[2], tool_box_.main_pen, Type_SimpleLine);
        drawText(painter_, pts[2], m_label, 0.0, tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    case 4:
        // 1
        drawDot(painter_, pts[0], tool_box_.basic_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.basic_pen, Type_CrossDot);
        // 3
        drawDot(painter_, pts[2], tool_box_.basic_pen, Type_CrossDot);
        drawLine(painter_, pts[0], pts[2], tool_box_.main_pen, Type_SimpleLine);
        drawLine(painter_, pts[1], pts[2], tool_box_.main_pen, Type_SimpleLine);
        drawText(painter_, pts[3], m_label, 0.0, tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool MM_Radian::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
    if(isFinished()==false)
    {
        return-1;
    }
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    QPointF cursor_pos = canvas_->logicalToPhysical(cursor_pos_);
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(pts[0], pts[2]), cursor_pos)
        || aroundLine(QLineF(pts[1], pts[2]), cursor_pos)
        || aroundText(pts[3], m_label, 0.0, tool_box_.basic_font, cursor_pos))
        return true;
    else
        return false;
}

qreal MM_Radian::calculateResult(bool recalculate_)
{
    if(m_points.size() == 3 || (m_points.size() >= 3 && recalculate_)){
        QLineF line1(m_points[0], m_points[2]);
        QLineF line2(m_points[1], m_points[2]);

        qreal angle = line1.angleTo(line2);
        if(angle >= 180)
            m_result = 360 - angle;
        else
            m_result = angle;
        m_label = m_prefix + m_description + QString::number(m_result, 'g', DIST_PRECISION + numOfIntergerDigit(m_result)) + m_unit;
    }

    return m_result;
}

// MM_Count
// --------
MM_Count::MM_Count(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = MAX_COUNT;
    // m_feature_points = m_points[0 : end-1]
    m_feature_points = QVector<QPointF>(0);
    m_n_feature_exceptions = 0;
    m_feature_exceptions = nullptr;
    // 描述
    m_unit = QObject::tr("");
    m_description = "计数";
}

void MM_Count::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);
    calculateResult();
}

void MM_Count::setEndPoint(const QPointF &pos_)
{
    // 设置最大数量
    m_N = m_points.size() + 1;
    // 特征点
    m_feature_points = m_points;
}

void MM_Count::stepBack()
{
    m_N = MAX_COUNT;
    Element::stepBack();
}

void MM_Count::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    // 3. 绘图
    // -------
    // 画点
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != (pts.size() == m_N ? pts.end() - 1 : pts.end());
         p_pts++){
        drawDot(painter_, (*p_pts), tool_box_.measurement_pen, Type_CrossDot);
    }

    if(pts.size() > 1)
        drawText(painter_, pts.back(), m_label, 0.0, tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);

    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool MM_Count::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ----------------
    if(isFinished()==false)
    {
        return-1;
    }
    // 2.判断是否靠近特征点
    // -----------------
    QPointF cursor_pos = cursor_pos_;
    if(aroundText(canvas_->logicalToPhysical(m_points.back()), m_label, 0, tool_box_.basic_font, canvas_->logicalToPhysical(cursor_pos_))
        || Element::aroundMyFeature(canvas_, cursor_pos))
        return true;
    else
        return false;
}

qreal MM_Count::calculateResult(bool recalculate_)
{
    m_result = (m_points.size() == m_N ? m_N - 1 :  m_points.size());
    m_label = m_prefix + m_description + QString::number(m_result, 'g', DIST_PRECISION + numOfIntergerDigit(m_result)) + m_unit;
    return m_result;
}

// AM_Circle
// ---------
AM_Circle::AM_Circle(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 4;
    // m_feature_points
    // 0 - 圆心
    m_feature_points = QVector<QPointF>(1);
    m_n_feature_exceptions = 1;
    m_feature_exceptions = new int[1]{0};
    // 描述
    m_unit = QObject::tr("um²");
    m_description = "圆面积";
}

void AM_Circle::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch(m_points.size()){
    case 3:
        m_feature_points[0] = circleCenter(m_points[0], m_points[1], m_points[2]);
        break;
    }

    calculateResult();
}

void AM_Circle::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    // 3. 绘图
    // -------
    switch(pts.size()){
    case 1:
        // 1
        drawDot(painter_, pts[0], tool_box_.measurement_pen, Type_CrossDot);
        break;
    case 2:
        // 1
        drawDot(painter_, pts[0], tool_box_.measurement_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.measurement_pen, Type_CrossDot);
        break;
    case 3:
        // 1
        drawDot(painter_, pts[0], tool_box_.measurement_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.measurement_pen, Type_CrossDot);
        // 3
        drawDot(painter_, pts[2], tool_box_.measurement_pen, Type_CrossDot);
        drawCircle(painter_, feature_pts[0], pts[2], tool_box_.measurement_pen, tool_box_.area_brush);
        drawText(painter_, pts[2], m_label, 0.0, tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    case 4:
        // 1
        // 2
        // 3
        drawCircle(painter_, feature_pts[0], pts[2], tool_box_.measurement_pen, tool_box_.area_brush);
        // 4
        drawText(painter_, pts[3], m_label, 0.0, tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();

}

bool AM_Circle::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
    if(isFinished()==false)
    {
        return-1;
    }
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    QPointF cursor_pos = canvas_->logicalToPhysical(cursor_pos_);
    // 3. 判断是否靠近
    // -------------
    if(inCircle(feature_pts[0], dist(feature_pts[0], pts[2]) + tool_box_.measurement_pen.width() / 2, cursor_pos)
        || aroundText(pts[3], m_label, 0.0, tool_box_.basic_font, cursor_pos))
        return true;
    else
        return false;
}

qreal AM_Circle::calculateResult(bool recalculate_)
{
    if(m_points.size() == 3 || (m_points.size() >= 3 && recalculate_)){
        // 椭圆面积公式: S = pi * a * b
        // 其中a, b分别为半长轴长,半短轴
        // ------------------------------
        // 先算半径
        QPointF vec = m_points[2] - m_feature_points[0];
        qreal dx = abs(vec.x());
        qreal dy = abs(vec.y());
        qreal r = sqrt(dx * dx + dy * dy);
        // 计算长短轴
        qreal a = r * m_target->x_scope / m_target->topo2d.width();
        qreal b = r * m_target->y_scope / m_target->topo2d.height();
        // 计算面积
        m_result = acos(-1) * a * b;
        m_label = m_prefix + m_description + QString::number(m_result, 'g', DIST_PRECISION + numOfIntergerDigit(m_result)) + m_unit;
    }

    return m_result;
}

// AM_Rect
// -------
AM_Rect::AM_Rect(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 3;
    // m_feature_points
    m_feature_points = QVector<QPointF>(0);
    m_n_feature_exceptions = 0;
    m_feature_exceptions = new int[0]{};
    // 描述
    m_unit = QObject::tr("um²");
    m_description = "矩形面积";
}

void AM_Rect::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    calculateResult();
}

void AM_Rect::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    // 3. 绘图
    // -------
    switch(pts.size()){
    case 1:
        // 1
        drawDot(painter_, pts[0], tool_box_.measurement_pen, Type_CrossDot);
        break;
    case 2:
        // 1
        drawDot(painter_, pts[0], tool_box_.measurement_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.measurement_pen, Type_CrossDot);
        drawRectangle(painter_, pts[0], pts[1], tool_box_.measurement_pen, tool_box_.area_brush);
        drawText(painter_, pts[1], m_label, 0.0, tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    case 3:
        // 1
        // 2
        // 3
        drawRectangle(painter_, pts[0], pts[1], tool_box_.measurement_pen, tool_box_.area_brush);
        drawText(painter_, pts[2], m_label, 0.0, tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AM_Rect::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
    if(isFinished()==false)
    {
        return-1;
    }
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    QPointF cursor_pos = canvas_->logicalToPhysical(cursor_pos_);
    // 3. 判断是否靠近
    // -------------
    if(inRectangle(QRectF(pts[0], pts[1]), cursor_pos)
        || aroundText(pts[2], m_label, 0.0, tool_box_.basic_font, cursor_pos))
        return true;
    else
        return false;
}

qreal AM_Rect::calculateResult(bool recalculate_)
{
    if(m_points.size() == 2 || (m_points.size() >= 2 && recalculate_)){
        QPointF vec = m_points[0] - m_points[1];
        qreal w = abs(vec.x()) * m_target->x_scope / m_target->topo2d.width();
        qreal h = abs(vec.y()) * m_target->y_scope / m_target->topo2d.height();
        m_result = w * h;
        m_label = m_prefix + m_description + QString::number(m_result, 'g', DIST_PRECISION + numOfIntergerDigit(m_result)) + m_unit;
    }
    return m_result;
}

// AM_Polygon
// ----------
AM_Polygon::AM_Polygon(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = MAX_N_EDGES;
    // m_feature_points = m_points[0 : end-1]
    m_feature_points = QVector<QPointF>(0);
    m_n_feature_exceptions = 0;
    m_feature_exceptions = nullptr;
    // 描述
    m_unit = QObject::tr("um²");
    m_description = "多边形面积";
}

void AM_Polygon::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    calculateResult();
}

void AM_Polygon::setEndPoint(const QPointF &pos_)
{
    // 设置最大数量
    m_N = m_points.size() + 1;
    // 特征点
    m_feature_points = m_points;
}

void AM_Polygon::stepBack()
{
    m_N = MAX_N_EDGES;
    Element::stepBack();
}

void AM_Polygon::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> pts = m_points;
    for(QVector<QPointF>::iterator p_pts = pts.begin();
         p_pts != pts.end();
         p_pts++){
        *p_pts = canvas_->logicalToPhysical(*p_pts);
    }
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    // 3. 绘图
    // -------
    if(pts.size() == 1)
        drawDot(painter_, pts[0], tool_box_.measurement_pen, Type_CrossDot);
    else if(pts.size() == 2)
        drawLine(painter_, pts[0], pts[1], tool_box_.measurement_pen, Type_SimpleLine);
    else if(pts.size() < m_N)
        drawPolygon(painter_, pts, tool_box_.measurement_pen, tool_box_.area_brush);
    else
        drawPolygon(painter_, feature_pts, tool_box_.measurement_pen, tool_box_.area_brush);

    if(pts.size() > 2){
        drawText(painter_, pts.back(), m_label, 0.0, tool_box_.basic_font, tool_box_.font_pen, tool_box_.font_bg_brush);
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();

}

bool AM_Polygon::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ----------------
    if(isFinished()==false)
    {
        return-1;
    }
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    QPointF cursor_pos = canvas_->logicalToPhysical(cursor_pos_);
    // 3.判断是否靠近特征点
    // -----------------
    if(aroundText(canvas_->logicalToPhysical(m_points.back()), m_label, 0, tool_box_.basic_font, canvas_->logicalToPhysical(cursor_pos_))
        || inPolygon(feature_pts, cursor_pos))
        return true;
    else
        return false;
}

qreal AM_Polygon::calculateResult(bool recalculate_)
{
    // 确定多边形端点
    QVector<QPointF> pts;
    if(m_points.size() == m_N)
        pts = m_feature_points;
    else
        pts = m_points;
    // factor
    qreal factor_x = m_target->x_scope / m_target->topo2d.width();
    qreal factor_y = m_target->y_scope / m_target->topo2d.height();
    // 计算实际空间坐标
    QVector<QPointF>::Iterator iter;
    for(iter = pts.begin(); iter != pts.end(); iter++){
        (*iter).setX((*iter).x() * factor_x);
        (*iter).setY((*iter).y() * factor_y);
    }
    // 计算
    m_result = polygonArea(pts);
    m_label = m_prefix + m_description + QString::number(m_result, 'g', DIST_PRECISION + numOfIntergerDigit(m_result)) + m_unit;
    return m_result;
}
