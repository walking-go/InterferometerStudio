#include "ag_element.h"
#include "drawing_funs.h"
#include "qpainter.h"
#include "element_canvas.h"

// AG_Dot
// ------
AG_Dot::AG_Dot(const ElementType& type_, const Target* target_)
    :Element(type_, target_)
{
    m_N = 1;
    m_feature_points = QVector<QPointF>(1);
}

void AG_Dot::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);
    switch (m_points.size()) {
    case 1:
        m_feature_points[0] = m_points[0];
        break;
    }
}

void AG_Dot::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
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
    // 3. 绘图
    // -------
    switch(pts.size()){
    case 1:
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AG_Dot::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
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
    QPointF cursor_pos = canvas_->logicalToPhysical(cursor_pos_);
    // 3. 判断是否靠近
    // -------------
    if(aroundDot(pts[0], cursor_pos))
        return true;
    else
        return false;
}

// AG_LineSegment
// --------------
AG_LineSegment::AG_LineSegment(const ElementType &type_, const Target* target_)
    :Element(type_, target_)
{
    m_N = 2;
    m_feature_points = QVector<QPointF>(2);
}

void AG_LineSegment::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch (m_points.size()) {
    case 1:
        m_feature_points[0] = m_points[0];
        break;
    case 2:
        m_feature_points[1] = m_points[1];
        break;
    }
}

void AG_LineSegment::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
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
    // 3. 绘图
    // -------
    switch(pts.size()){
    case 1:
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
        break;
    case 2:
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
        drawDot(painter_, pts[1], tool_box_.main_pen, Type_CrossDot);
        drawLine(painter_, pts[0], pts[1], tool_box_.main_pen, Type_SimpleLine);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AG_LineSegment::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
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
    QPointF cursor_pos = canvas_->logicalToPhysical(cursor_pos_);
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(pts[0], pts[1]), cursor_pos))
        return true;
    else
        return false;
}

// AG_Circle
// ---------
AG_Circle::AG_Circle(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 3;
    // m_feature_points
    // 0 - 圆心
    m_feature_points = QVector<QPointF>(1);
    m_n_feature_exceptions = 0;
    m_feature_exceptions = nullptr;
}

void AG_Circle::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch (m_points.size()) {
    case 3:
        m_feature_points[0] = circleCenter(m_points[0], m_points[1], m_points[2]);
        break;
    }
}

void AG_Circle::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
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
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
        break;
    case 2:
        // 1
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.main_pen, Type_CrossDot);
        break;
    case 3:
        drawDot(painter_, feature_pts[0], tool_box_.main_pen, Type_CrossDot);
        drawCircle(painter_, feature_pts[0], pts[2], tool_box_.main_pen);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AG_Circle::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
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
    if(aroundCircle(feature_pts[0], dist(feature_pts[0], pts[2]), cursor_pos)
        || aroundDot(feature_pts[0], cursor_pos))
        return true;
    else
        return false;
}


// AG_Line
// -------
AG_Line::AG_Line(const ElementType &type_, const Target* target_)
    :Element(type_, target_)
{
    m_N = 2;
    // m_feature_points:
    // 0 - m_points[0]
    // 1 - m_points[1]
    // 2 - 线段延伸后的其中一点
    // 3 - 线段延伸后的其中一点
    m_feature_points = QVector<QPointF>(4);
    // 跳过检测的特征点
    m_n_feature_exceptions = 2;
    m_feature_exceptions = new int[2]{2, 3};
}

void AG_Line::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch (m_points.size()) {
    case 1:
        m_feature_points[0] = m_points[0];
        break;
    case 2:
        m_feature_points[1] = m_points[1];
        QLineF line = stretchLineSegment(QLineF(m_points[0], m_points[1]), m_target->topo.rect());
        m_feature_points[2] = line.p1();
        m_feature_points[3] = line.p2();
        break;
    }
}

void AG_Line::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
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
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
        break;
    case 2:
        drawLine(painter_, feature_pts[2], feature_pts[3], tool_box_.main_pen, Type_SimpleLine);
        drawLine(painter_, pts[0], pts[1], tool_box_.main_pen, Type_SimpleLine);
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
        drawDot(painter_, pts[1], tool_box_.main_pen, Type_CrossDot);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AG_Line::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
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
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(feature_pts[2], feature_pts[3]), cursor_pos))
        return true;
    else
        return false;
}

// AG_HorizontalLine
// -----------------
AG_HorizontalLine::AG_HorizontalLine(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 1;
    // m_feature_points:
    // 0 - 水平线与图像左边缘交点
    // 1 - 水平线与图像右边缘交点
    m_feature_points = QVector<QPointF>(2);
    // 跳过检测的特征点
    m_n_feature_exceptions = 2;
    m_feature_exceptions = new int[2]{0, 1};
}

void AG_HorizontalLine::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch (m_points.size()) {
    case 1:
        qreal y = m_points[0].y();
        m_feature_points[0] = QPointF(m_target->topo.rect().left(), y);
        m_feature_points[1] = QPointF(m_target->topo.rect().right(), y);
        break;
    }
}

void AG_HorizontalLine::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    // 3. 绘图
    // -------
    drawLine(painter_, feature_pts[0], feature_pts[1], tool_box_.main_pen, Type_SimpleLine);
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AG_HorizontalLine::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
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
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(feature_pts[0], feature_pts[1]), cursor_pos))
        return true;
    else
        return false;

}

// AG_VerticalLine
// ---------------
AG_VerticalLine::AG_VerticalLine(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 1;
    // m_feature_points:
    // 0 - 水平线与图像上边缘交点
    // 1 - 水平线与图像下边缘交点
    m_feature_points = QVector<QPointF>(2);
    // 跳过检测的特征点
    m_n_feature_exceptions = 2;
    m_feature_exceptions = new int[2]{0, 1};
}

void AG_VerticalLine::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch (m_points.size()) {
    case 1:
        qreal x = m_points[0].x();
        m_feature_points[0] = QPointF(x, m_target->topo.rect().top());
        m_feature_points[1] = QPointF(x, m_target->topo.rect().bottom());
        break;
    }
}

void AG_VerticalLine::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    // 3. 绘图
    // -------
    drawLine(painter_, feature_pts[0], feature_pts[1], tool_box_.main_pen, Type_SimpleLine);
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AG_VerticalLine::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
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
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(feature_pts[0], feature_pts[1]), cursor_pos))
        return true;
    else
        return false;

}

// AG_ClockwiseCircle
// ------------------
AG_ClockwiseCircle::AG_ClockwiseCircle(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 3;
    // m_feature_points
    // 0 - 圆心
    // 1 - 圆最右侧的点
    m_feature_points = QVector<QPointF>(2);
    m_n_feature_exceptions = 1;
    m_feature_exceptions = new int[1]{1};
}

void AG_ClockwiseCircle::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch (m_points.size()) {
    case 3:
        m_feature_points[0] = circleCenter(m_points[0], m_points[1], m_points[2]);
        m_feature_points[1] = m_feature_points[0] + QPointF(dist(m_points[0], m_feature_points[0]), 0);
        break;
    }
}

void AG_ClockwiseCircle::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
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
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
        break;
    case 2:
        // 1
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
        // 2
        drawDot(painter_, pts[1], tool_box_.main_pen, Type_CrossDot);
        break;
    case 3:
        // 草稿状态时
        if(this->m_status == ElementStatus::Draft){
            // 1
            drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
            // 2
            drawDot(painter_, pts[1], tool_box_.main_pen, Type_CrossDot);
            // 3
            drawDot(painter_, pts[1], tool_box_.main_pen, Type_CrossDot);
        }
        // 稳定状态时
        drawDot(painter_, feature_pts[0], tool_box_.main_pen, Type_CrossDot);
        drawCircle(painter_, feature_pts[0], pts[2], tool_box_.main_pen, QBrush(Qt::transparent), Type_ClockwiseCircle);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AG_ClockwiseCircle::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
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
    if(aroundCircle(feature_pts[0], dist(feature_pts[0], pts[2]), cursor_pos)
        || aroundDot(feature_pts[0], cursor_pos))
        return true;
    else
        return false;
}

// AG_ArrowedLineSegment
// ---------------------
AG_ArrowedLineSegment::AG_ArrowedLineSegment(const ElementType &type_, const Target* target_)
    :Element(type_, target_)
{
    m_N = 2;
    m_feature_points = QVector<QPointF>(2);
}

void AG_ArrowedLineSegment::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch (m_points.size()) {
    case 1:
        m_feature_points[0] = m_points[0];
        break;
    case 2:
        m_feature_points[1] = m_points[1];
        break;
    }
}

void AG_ArrowedLineSegment::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
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
    // 3. 绘图
    // -------
    switch(pts.size()){
    case 1:
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_FilledDot);
        break;
    case 2:
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_FilledDot);
        drawDot(painter_, pts[1], tool_box_.main_pen, Type_FilledDot);
        drawLine(painter_, pts[0], pts[1], tool_box_.main_pen, Type_SingleArrowedLine);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AG_ArrowedLineSegment::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
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
    QPointF cursor_pos = canvas_->logicalToPhysical(cursor_pos_);
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(pts[0], pts[1]), cursor_pos))
        return true;
    else
        return false;
}

// AG_ArrowedLine
// --------------
AG_ArrowedLine::AG_ArrowedLine(const ElementType &type_, const Target* target_)
    :Element(type_, target_)
{
    m_N = 2;
    // m_feature_points:
    // 0 - m_points[0]
    // 1 - m_points[1]
    // 2 - 线段延伸后的其中一点
    // 3 - 线段延伸后的其中一点
    m_feature_points = QVector<QPointF>(4);
    // 跳过检测的特征点
    m_n_feature_exceptions = 2;
    m_feature_exceptions = new int[2]{2, 3};
}

void AG_ArrowedLine::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch (m_points.size()) {
    case 1:
        m_feature_points[0] = m_points[0];
        break;
    case 2:
        m_feature_points[1] = m_points[1];
        QLineF line = stretchLineSegment(QLineF(m_points[0], m_points[1]), m_target->topo.rect());
        m_feature_points[2] = line.p1();
        m_feature_points[3] = line.p2();
        break;
    }
}

void AG_ArrowedLine::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
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
        drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
        break;
    case 2:
        if(this->m_status == ElementStatus::Draft){
            drawLine(painter_, pts[0], pts[1], tool_box_.main_pen, Type_SimpleLine);
            drawDot(painter_, pts[0], tool_box_.main_pen, Type_CrossDot);
            drawDot(painter_, pts[1], tool_box_.main_pen, Type_CrossDot);
        }
        drawLine(painter_, feature_pts[2], feature_pts[3], tool_box_.main_pen, Type_VectorLine);
        break;
    }
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AG_ArrowedLine::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
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
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(feature_pts[2], feature_pts[3]), cursor_pos))
        return true;
    else
        return false;
}

// AG_ArrowedHorizontalLine
// -----------------
AG_ArrowedHorizontalLine::AG_ArrowedHorizontalLine(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 1;
    // m_feature_points:
    // 0 - 水平线与图像左边缘交点
    // 1 - 水平线与图像右边缘交点
    m_feature_points = QVector<QPointF>(2);
    // 跳过检测的特征点
    m_n_feature_exceptions = 2;
    m_feature_exceptions = new int[2]{0, 1};
}

void AG_ArrowedHorizontalLine::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch (m_points.size()) {
    case 1:
        qreal y = m_points[0].y();
        m_feature_points[0] = QPointF(m_target->topo.rect().left(), y);
        m_feature_points[1] = QPointF(m_target->topo.rect().right(), y);
        break;
    }
}

void AG_ArrowedHorizontalLine::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    // 3. 绘图
    // -------
    drawLine(painter_, feature_pts[0], feature_pts[1], tool_box_.main_pen, Type_VectorLine);
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AG_ArrowedHorizontalLine::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
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
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(feature_pts[0], feature_pts[1]), cursor_pos))
        return true;
    else
        return false;

}

// AG_VerticalLine
// ---------------
AG_ArrowedVerticalLine::AG_ArrowedVerticalLine(const ElementType &type_, const Target *target_)
    :Element(type_, target_)
{
    m_N = 1;
    // m_feature_points:
    // 0 - 水平线与图像上边缘交点
    // 1 - 水平线与图像下边缘交点
    m_feature_points = QVector<QPointF>(2);
    // 跳过检测的特征点
    m_n_feature_exceptions = 2;
    m_feature_exceptions = new int[2]{0, 1};
}

void AG_ArrowedVerticalLine::setIntention(const QPointF &pos_)
{
    Element::setIntention(pos_);

    switch (m_points.size()) {
    case 1:
        qreal x = m_points[0].x();
        m_feature_points[0] = QPointF(x, m_target->topo.rect().top());
        m_feature_points[1] = QPointF(x, m_target->topo.rect().bottom());
        break;
    }
}

void AG_ArrowedVerticalLine::draw(ElementCanvas *canvas_, QPainter *painter_, const ToolBox &tool_box_, const QString &text_)
{
    // 1. 暂存原始QPainter::painter_
    // ----------------------------
    painter_->save();
    // 2. 将“逻辑坐标”转换为“物理坐标”
    // ----------------------------
    QVector<QPointF> feature_pts = m_feature_points;
    for(QVector<QPointF>::iterator p_feature_pts = feature_pts.begin();
         p_feature_pts != feature_pts.end();
         p_feature_pts++){
        *p_feature_pts = canvas_->logicalToPhysical(*p_feature_pts);
    }
    // 3. 绘图
    // -------
    drawLine(painter_, feature_pts[0], feature_pts[1], tool_box_.main_pen, Type_VectorLine);
    // 4. 恢复原始QPainter::painter_
    // ----------------------------
    painter_->restore();
}

bool AG_ArrowedVerticalLine::aroundMe(ElementCanvas *canvas_, const QPointF &cursor_pos_, const ToolBox &tool_box_)
{
    // 1.判断元素是否完成
    // ---------------
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
    // 3. 判断是否靠近
    // -------------
    if(aroundLine(QLineF(feature_pts[0], feature_pts[1]), cursor_pos))
        return true;
    else
        return false;

}
