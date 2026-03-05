#include "drawing_funs.h"
#include "qdebug.h"
#include "qmath.h"
#include "uddm_namespace.h"

// 常量参数
// -------
// 绘制效果相关
const int DELTA_PEN_WIDTH = 2;  /**< 强调绘制时，画笔加粗量*/
const int DELTA_FONT_WIDTH = 2; /**< 强调绘制时，字体pointSize加粗量*/
const int DELTA_ALPHA = 50;     /**< 强调绘制时, 填充透明度增加量*/
// Draw sth.
const int CHAR_MARGIN = 2;      /**< 绘制文字时，文字边框margin, 单位为pointSize*/
const int CHAR_SPACING = 2;     /**< 绘制文字时，文字间隔, 单位为pointSize*/

// 绘制效果相关
// ----------
/**
 * @brief setAlpha
 * 将painter_中rect_区域设置为半透明，透明度为alpha_
 */
void setAlpha(QPainter* painter_, QRect rect_, int alpha_)
{
    QPainter::CompositionMode mode = painter_->compositionMode();
    painter_->setCompositionMode(QPainter::CompositionMode_DestinationIn);
    painter_->fillRect(rect_, QColor(0, 0, 0, alpha_));
    painter_->setCompositionMode(mode);
}

/**
 * @brief emphasizeToolBox
 * 对tool_box_做强调处理
 * 包括画笔加粗DELTA_PEN_WIDTH
 * 字体增大DELTA_FONT_WIDTH
 */
void emphasizeToolBox(ToolBox &tool_box_)
{
    // 线宽 + DELTA_PEN_WIDTH
    tool_box_.measurement_pen.setWidthF(tool_box_.measurement_pen.widthF() + DELTA_PEN_WIDTH);
    tool_box_.main_pen.setWidthF(tool_box_.main_pen.widthF() + DELTA_PEN_WIDTH);
    tool_box_.basic_pen.setWidthF(tool_box_.basic_pen.widthF() + DELTA_PEN_WIDTH);
    tool_box_.assist_pen.setWidthF(tool_box_.assist_pen.widthF() + DELTA_PEN_WIDTH);
    // 字体 + DELTA_FONT_WIDTH
    tool_box_.basic_font.setWeight(QFont::ExtraBold);
    tool_box_.basic_font.setPointSize(tool_box_.basic_font.pointSize() + DELTA_FONT_WIDTH);
    // 笔刷透明度 + DELTA_ALPHA
    QColor color = tool_box_.area_brush.color();
    color.setAlpha(color.alpha() + DELTA_ALPHA);
    tool_box_.area_brush.setColor(color);
}

// Draw sth.
// ---------
void drawDot(QPainter *painter_, const QPointF &pt_, const QPen &pen_, const DotType &type_)
{
    painter_->save();
    painter_->setPen(pen_);
    // 参数表
    // -----
    // Type_CrossDot
    qreal shift = pen_.widthF() + 5.0;             /**< 交叉点所在矩形的半边长*/
    // Type_FilledDot
    qreal r = pen_.widthF() + 1;    /**< 点所在圆的半径*/

    // 绘制
    // -----
    switch(type_){
    case Type_NoDot:
        break;
    case Type_CrossDot:{
        QPointF shift_x(shift, 0.0);
        QPointF shift_y(0.0, shift);
        painter_->drawLine(pt_ - shift_x - shift_y, pt_ + shift_x + shift_y);
        painter_->drawLine(pt_ - shift_x + shift_y, pt_ + shift_x - shift_y);
        break;
    }
    case Type_FilledDot:
        painter_->setBrush(pen_.color());
        painter_->drawEllipse(pt_.x() - r, pt_.y() - r, 2 * r, 2 * r);
        break;
    }
    painter_->restore();
}

void drawLine(QPainter *painter_, const QPointF &pt1_, const QPointF &pt2_, const QPen &pen_, const LineType &type_)
{
    painter_->save();
    painter_->setPen(pen_);

    // 目标属性
    // -------
    QLineF line(pt1_, pt2_);
    qreal angle = line.angle();

    // 参数表
    // -----
    // Type_VectorLine & Type_DoubleArrowedLine & Type_SingleArrowedLine
    qreal arrow_angle = 30;     /**< 箭头一边张开的角度*/
    qreal arrow_lenght = 20;    /**< 箭头线长*/

    // 绘制
    // -----
    switch(type_){
    case Type_NoLine:
        break;
    case Type_VectorLine:{
        QPointF indicator_pt = 0.5 * pt1_ + 0.5 * pt2_;
        QLineF arrow_line(indicator_pt.x(), indicator_pt.y(), indicator_pt.x() + arrow_lenght, indicator_pt.y());
        arrow_line.setAngle(angle + 180 - arrow_angle);
        painter_->drawLine(arrow_line);
        arrow_line.setAngle(angle + 180 + arrow_angle);
        painter_->drawLine(arrow_line);
        painter_->drawLine(line);
        break;
    }
    case Type_DoubleArrowedLine:{
        QLineF arrow_line(pt1_.x(), pt1_.y(), pt1_.x() + arrow_lenght, pt1_.y());
        arrow_line.setAngle(angle - arrow_angle);
        painter_->drawLine(arrow_line);
        arrow_line.setAngle(angle + arrow_angle);
        painter_->drawLine(arrow_line);
    }
    case Type_SingleArrowedLine:{
        QLineF arrow_line(pt2_.x(), pt2_.y(), pt2_.x() + arrow_lenght, pt2_.y());
        arrow_line.setAngle(angle + 180 - arrow_angle);
        painter_->drawLine(arrow_line);
        arrow_line.setAngle(angle + 180 + arrow_angle);
        painter_->drawLine(arrow_line);
    }
    case Type_SimpleLine:
        painter_->drawLine(line);
        break;
    }
    painter_->restore();
}

void drawCircle(QPainter *painter_, const QPointF &pt1_, const QPointF &pt2_, const QPen &pen_, const QBrush &brush_, const CircleType& type_)
{
    painter_->save();
    painter_->setPen(pen_);
    painter_->setBrush(brush_);

    // 目标属性
    // -----
    qreal radius = QLineF(pt1_, pt2_).length();
    qreal diam = 2 * radius ;
    QRectF rectangle(pt1_.x() - radius, pt1_.y() - radius, diam, diam);

    // 参数表
    // -----
    // Type_ClockwiseCircle
    // Type_AntiClockwiseCircle
    qreal indicator_angle = 30; /**< 指示箭头开始的角度*/

    // 绘制
    // -----
    switch(type_){
    case Type_NoCircle:
        break;
    case Type_SimpleCircle:
        painter_->drawEllipse(rectangle);
        break;
    case Type_ClockwiseCircle:{
        painter_->drawEllipse(rectangle);
        QPointF rightmost_pt = pt1_ + QPointF(radius, 0);
        // 绘制最右边点
        drawDot(painter_, rightmost_pt, pen_, Type_FilledDot);
        // 绘制箭头
        QLineF assist_line(pt1_, rightmost_pt);
        assist_line.setAngle(-indicator_angle);
        QPointF indicator_pt = assist_line.p2();
        QLineF indicator_line(indicator_pt, indicator_pt + QPointF(0, 0.1));
        indicator_line.setAngle(-90 - indicator_angle);
        drawLine(painter_, indicator_line.p1(), indicator_line.p2(), pen_, Type_SingleArrowedLine);
        break;
    }
    case Type_AntiClockwiseCircle:{
        painter_->drawEllipse(rectangle);
        QPointF rightmost_pt = pt1_ + QPointF(radius, 0);
        // 绘制最右边点
        drawDot(painter_, rightmost_pt, pen_, Type_FilledDot);
        // 绘制箭头
        QLineF assist_line(pt1_, rightmost_pt);
        assist_line.setAngle(indicator_angle);
        QPointF indicator_pt = assist_line.p2();
        QLineF indicator_line(indicator_pt, indicator_pt + QPointF(0, 0.1));
        indicator_line.setAngle(90 + indicator_angle);
        drawLine(painter_, indicator_line.p1(), indicator_line.p2(), pen_, Type_SingleArrowedLine);
    }
    break;
    };


    painter_->restore();
}

void drawRectangle(QPainter *painter_, const QPointF &pt1_, const QPointF &pt2_, const QPen &pen_, const QBrush &brush_)
{
    painter_->save();
    painter_->setPen(pen_);
    painter_->setBrush(brush_);
    //目标属性
    //-----
    qreal width=pt2_.x()-pt1_.x();
    qreal height=pt2_.y()-pt1_.y();
    // 绘制
    // -----
    painter_->drawRect(pt1_.x(), pt1_.y(),width,height);

    painter_->restore();
}

void drawPolygon(QPainter *painter_, const QVector<QPointF>& pts_, const QPen &pen_, const QBrush &brush_)
{
    if(pts_.size() < 3)
        return;

    painter_->save();

    painter_->setPen(pen_);
    painter_->setBrush(brush_);
    painter_->drawPolygon(QPolygonF(pts_));

    painter_->restore();
}

void drawText(QPainter *painter_, const QPointF& pt_, const QString& text_
              , int angle_, const QFont& font_, const QPen& pen_, const QBrush& bg_brush_)
{
    painter_->save();
    painter_->setPen(pen_);
    painter_->setFont(font_);

    QPointF pt(pt_);
    // 旋转
    if(angle_ != 0){
        angle_ %= 360;
        if(angle_ > 90 && angle_ < 270)
            angle_ -= 180;
        painter_->translate(pt_);
        painter_->rotate(-angle_);
        pt = QPointF(0.0, 0.0);
    }

    // 边界
    int n = text_.length();
    int char_size = font_.pointSize() * 1.3;
    QRectF rect = QRectF(pt, QSize(char_size * n + CHAR_MARGIN * 2 + CHAR_SPACING * (n - 1), char_size * 2));
    rect.moveBottomLeft(rect.topLeft());
    rect.moveBottomLeft(rect.bottomLeft() + (rect.topLeft() - rect.bottomLeft()) * 0.15);
    // 背景
    painter_->fillRect(rect, bg_brush_);
    // 文字
    painter_->drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, text_);

    painter_->restore();
}

// Around sth.
// -----------
bool aroundDot(const QPointF &pt_, const QPointF &pos_, qreal thresh_)
{
    qreal dx = pt_.x() - pos_.x();
    qreal dy = pt_.y() - pos_.y();
    qreal dist = sqrt(dx * dx + dy * dy);

    if(abs(dist) < thresh_)
        return true;
    else
        return false;
}

bool aroundLine(const QLineF& line_, const QPointF& pos_, qreal thresh_)
{
    QPointF pt1 = line_.p1();
    QPointF pt2 = line_.p2();

    // 计算线段垂点
    // ----------
    QPointF np;
    qreal x_se = pt1.x() - pt2.x();
    qreal y_se = pt1.y() - pt2.y();
    qreal x_se_2 = x_se * x_se; // x平方
    qreal y_se_2 = y_se * y_se; // y平方
    qreal x = (x_se_2 * pos_.x() + (pos_.y() - pt1.y()) * y_se * x_se + pt1.x() * y_se_2)
              / (x_se_2 + y_se_2);
    qreal y = y_se != 0 ? pos_.y() + x_se * (pos_.x() - x) / y_se : pt1.y();
    np.setX(x);
    np.setY(y);

    // 计算两点距离并判断鼠标是否靠近线元素
    // ------------------------------
    qreal dx = pos_.x() - np.x();
    qreal dy = pos_.y() - np.y();
    qreal dist = sqrt(dx * dx + dy * dy);

    if(abs(dist) < thresh_ && (np.x() - pt1.x()) * (np.x() - pt2.x()) <= 0)
        return true;
    else
        return false;
}

bool aroundCircle(const QPointF &center_, qreal radius_, const QPointF& pos_, qreal thresh_)
{
    qreal dx = center_.x() - pos_.x();
    qreal dy = center_.y() - pos_.y();
    qreal dist = sqrt(dx * dx + dy * dy) - radius_;
    if(abs(dist) < thresh_)
        return true;
    else
        return false;
}

bool aroundRectangle(const QRectF &rect_, const QPointF &pos_, qreal thresh_)
{
    qreal left   = qMin(rect_.left(), rect_.right());
    qreal right  = qMax(rect_.left(), rect_.right());
    qreal top    = qMin(rect_.top(), rect_.bottom());
    qreal bottom = qMax(rect_.top(), rect_.bottom());

    qreal dist_left   = -(pos_.x() - left);
    qreal dist_right  =   pos_.x() - right;
    qreal dist_top    = -(pos_.y() - top);
    qreal dist_bottom =   pos_.y() - bottom;

    // 是否矩形内
    bool in_rect = ((dist_left < thresh_) & (dist_right < thresh_) & (dist_top < thresh_) & (dist_bottom < thresh_))
                   | ((dist_left > -thresh_) & (dist_right > -thresh_) & (dist_top > -thresh_) & (dist_bottom > -thresh_));
    // 是否靠近四边
    bool close_to_edge = (abs(dist_left) < thresh_) || (abs(dist_right) < thresh_) || (abs(dist_top) < thresh_) || (abs(dist_bottom) < thresh_);
    if(in_rect & close_to_edge)
        return true;
    else
        return false;
}

bool aroundPolygon(const QVector<QPointF> &pts_, const QPointF &pos_, qreal thresh_)
{
    if(pts_.size() < 3)
        return false;

    for(int i = 1; i < (int)pts_.size(); i++)
        if(aroundLine(QLineF(pts_[i], pts_[i - 1]), pos_, thresh_))
            return true;
    if(aroundLine(QLineF(pts_[0], pts_.back()), pos_, thresh_))
        return true;

    return false;
}

bool aroundText(const QPointF &pt_, const QString &text_, int angle_, const QFont& font_, const QPointF &pos_)
{
    // 先做矩形
    int n = text_.length();
    int char_size = font_.pointSize();
    QRectF rect = QRectF(pt_, QSize(char_size * n + CHAR_MARGIN * 2 + CHAR_SPACING * (n - 1), char_size * 2));
    rect.moveBottomLeft(rect.topLeft());
    rect.moveBottomLeft(rect.bottomLeft() + (rect.topLeft() - rect.bottomLeft()) * 0.15);

    // 当没有旋转时
    if(angle_ == 0)
        return rect.contains(pos_);
    // 当存在旋转时
    else{
        if(angle_ != 0){
            angle_ %= 360;
            if(angle_ > 90 && angle_ < 270)
                angle_ -= 180;
        }

        // 以矩形左下角为原点建立坐标系
        // 将光标位置映射到该坐标系中
        QPointF pos = pos_ - rect.bottomLeft();
        // 将矩形四点映射到该坐标系中
        QTransform t;
        QPolygon poly;
        t.rotate(-angle_);
        t.translate(-rect.left(), -rect.bottom());
        poly = t.mapToPolygon(rect.toRect());
        // 判断光标是否在矩形中
        return poly.containsPoint(pos.toPoint(), Qt::OddEvenFill);
    }
}

// In sth.
// -------
bool inCircle(const QPointF &center_, qreal radius_, const QPointF &pos_)
{
    return dist(center_, pos_) < radius_;
}

bool inRectangle(const QRectF &rect_, const QPointF &pos_)
{
    return rect_.contains(pos_);
}

bool inPolygon(const QVector<QPointF> &pts_, const QPointF &pos_)
{
    return QPolygonF(pts_).containsPoint(pos_.toPoint(), Qt::OddEvenFill);
}

// 几何计算
// -------
/**
 * @brief dist
 * 计算pt1_、pt2_之间的欧氏距离
 */
qreal dist(const QPointF &pt1_, const QPointF &pt2_)
{
    QPointF vec = pt1_ - pt2_;
    return sqrt(vec.x() * vec.x() + vec.y() * vec.y());
}

/**
 * @brief verticalLineSegment
 * 返回一个垂直于line_的线段，
 * 该线段与line_.p2()的连线与line_垂直，
 * 该线段与dot_的连线与line_平行
 */
QLineF verticalLineSegment(const QPointF &dot_, const QLineF &line_)
{
    QPointF pt1 = line_.p1();
    QPointF pt2 = line_.p2();

    // 计算线段垂点
    // ----------
    QPointF np;
    qreal x_se = pt1.x() - pt2.x();
    qreal y_se = pt1.y() - pt2.y();
    qreal x_se_2 = x_se * x_se; // x平方
    qreal y_se_2 = y_se * y_se; // y平方
    qreal x = (x_se_2 * dot_.x() + (dot_.y() - pt1.y()) * y_se * x_se + pt1.x() * y_se_2)
              / (x_se_2 + y_se_2);
    qreal y = y_se != 0 ? dot_.y() + x_se * (dot_.x() - x) / y_se : pt1.y();
    np.setX(x);
    np.setY(y);

    // 计算两点距离
    // ----------
    qreal dx = dot_.x() - np.x();
    qreal dy = dot_.y() - np.y();
    qreal dist = sqrt(dx * dx + dy * dy);

    // 计算目标线段
    // ----------
    QPointF pt3 = pt2 + QPointF(dist, 0);
    QLineF result(pt2, pt3);
    result.setAngle(QLineF(0, 0, dx, dy).angle());

    return result;
}

/**
 * @brief verticalPoint
 * 返回点dot_到线段line_的垂点
 */
QPointF verticalPoint(const QPointF &dot_, const QLineF &line_)
{
    QPointF pt1 = line_.p1();
    QPointF pt2 = line_.p2();

    // 计算线段垂点
    // ----------
    QPointF np;
    qreal x_se = pt1.x() - pt2.x();
    qreal y_se = pt1.y() - pt2.y();
    qreal x_se_2 = x_se * x_se; // x平方
    qreal y_se_2 = y_se * y_se; // y平方
    qreal x = (x_se_2 * dot_.x() + (dot_.y() - pt1.y()) * y_se * x_se + pt1.x() * y_se_2)
              / (x_se_2 + y_se_2);
    qreal y = dot_.y() + x_se * (dot_.x() - x) / y_se;
    np.setX(x);
    np.setY(y);

    return np;
}

/**
 * @brief circleCenter
 * 计算三点圆中心，代码由chatGPT提供
 */
QPointF circleCenter(const QPointF &pt1_, const QPointF &pt2_, const QPointF &pt3_)
{
    qreal x1 = pt1_.x(), y1 = pt1_.y();
    qreal x2 = pt2_.x(), y2 = pt2_.y();
    qreal x3 = pt3_.x(), y3 = pt3_.y();

    qreal a = x2 - x1;
    qreal b = y2 - y1;
    qreal c = x3 - x1;
    qreal d = y3 - y1;

    qreal e = a * (x1 + x2) + b * (y1 + y2);
    qreal f = c * (x1 + x3) + d * (y1 + y3);

    qreal g = 2 * (a * (y3 - y2) - b * (x3 - x2));

    if (qFuzzyIsNull(g)) {
        // 三个点共线
        return QPointF(0, 0);
    }

    qreal center_x = (d * e - b * f) / g;
    qreal center_y = (a * f - c * e) / g;

    if (qFabs(center_x) > 1e4 || qFabs(center_y) > 1e4) {
        // 防止中心坐标过大
        return QPointF(0, 0);
    }

    QPointF center(center_x, center_y);
    return center;
}

/**
 * @brief polygonArea
 * 计算多边形面积
 * 计算方法参考:https://www.cnblogs.com/icmzn/p/10746561.html
 */
qreal polygonArea(const QVector<QPointF> &pts_)
{
    if(pts_.size() < 3)
        return 0.0;

    double s = pts_[0].y() * (pts_[pts_.size() - 1].x() - pts_[1].x());

    for(int i = 1; i < pts_.size(); i++)
        s += pts_[i].y() * (pts_[i - 1].x() - pts_[(i + 1) % pts_.size()].x());

    return fabs(s/2.0);
}

/**
 * @brief stretchLineSegment
 * 使用线段line_延长得到的直线,切割矩形rect_
 * 当切割得到的交点数等于2时,返回由两交点构成的新线段
 * 否则返回一个空的QLineF对象
 */
QLineF stretchLineSegment(const QLineF &line_, const QRectF& rect_)
{
    // 定义辅助点和辅助线段
    QPointF intersect_pt;
    QLineF  top_line(rect_.topLeft(), rect_.topRight() - QPointF(1.0, 0.0));
    QLineF  bottom_line(rect_.bottomLeft() - QPointF(0.0, 1.0), rect_.bottomRight() - QPointF(1.0, 1.0));
    QLineF  left_line(rect_.bottomLeft() - QPointF(0.0, 1.0), rect_.topLeft());
    QLineF  right_line(rect_.bottomRight() - QPointF(1.0, 1.0), rect_.topRight() - QPointF(1.0, 0.0));

    // 找交点
    QVector<QPointF> pts;
    if(line_.intersects(top_line, &intersect_pt) != 0)
        pts.push_back(intersect_pt);
    if(line_.intersects(bottom_line, &intersect_pt) != 0)
        pts.push_back(intersect_pt);
    if(line_.intersects(left_line, &intersect_pt) != 0)
        pts.push_back(intersect_pt);
    if(line_.intersects(right_line, &intersect_pt) != 0)
        pts.push_back(intersect_pt);
    assert(pts.size() == 4 || pts.size() == 2);

    // 筛选出在边界内的交点
    if(pts.size() == 4){
        std::sort(pts.begin(), pts.end(),
                  [&](QPointF pt1_, QPointF pt2_) -> bool{return pt1_.x() > pt2_.x();} );
        pts.pop_front();
        pts.pop_back();
    }

    //  确保pts在边界内
    double epsilon = 1e-6;
    for(int i = 0; i < 2; i++){
        if(closeToZero(pts[i].x(), epsilon))
            pts[i].setX(epsilon);
        if(closeToZero(pts[i].x() - (rect_.width() - 1), epsilon))
            pts[i].setX(rect_.width() - 1 - epsilon);
        if(closeToZero(pts[i].y(), epsilon))
            pts[i].setY(epsilon);
        if(closeToZero(pts[i].y() - (rect_.height() - 1), epsilon))
            pts[i].setY(rect_.height() - 1 - epsilon);
    }

    // 返回
    if(closeToZero<qreal>(QLineF(pts[0], pts[1]).angleTo(line_))
        || closeToZero<qreal>(QLineF(pts[0], pts[1]).angleTo(line_) - 360))
        return QLineF(pts[0], pts[1]);
    else
        return QLineF(pts[1], pts[0]);
}
