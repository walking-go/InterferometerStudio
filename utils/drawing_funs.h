#ifndef DRAWING_FUNS_H
#define DRAWING_FUNS_H

#include "canvas_namespace.h"
#include "math.h"
#include <QPainter>
#include <QLine>
#include <QVector>

// 绘制效果相关
// ----------
void setAlpha(QPainter* painter_, QRect rect_, int alpha_);
void emphasizeToolBox(ToolBox& tool_box_);

// Draw sth.
// ---------
void drawDot(QPainter* painter_, const QPointF& pt_, const QPen& pen_, const DotType& type_ = DotType::Type_CrossDot);
void drawLine(QPainter* painter_, const QPointF& pt1_, const QPointF& pt2_, const QPen& pen_, const LineType& type_ = LineType::Type_SimpleLine);
void drawCircle(QPainter* painter_, const QPointF& pt1_, const QPointF& pt2_, const QPen& pen_, const QBrush &brush_ = QBrush(Qt::transparent), const CircleType& type_ = CircleType::Type_SimpleCircle);
void drawRectangle(QPainter* painter_, const QPointF& pt1_, const QPointF& pt2_, const QPen& pen_, const QBrush &brush_ = QBrush(Qt::transparent));
void drawPolygon(QPainter* painter_, const QVector<QPointF>& pts_, const QPen& pen_, const QBrush &brush_ = QBrush(Qt::transparent));
void drawText(QPainter* painter_, const QPointF& pt_, const QString& text_,
              int angle_, const QFont& font_, const QPen& pen_, const QBrush& bg_brush_ = QBrush(Qt::white));

// Around sth.
// -----------
const int AROUND_THRESH = 10;   /**< 检测某点是否临近某元素时用到的阈值，距离小于该阈值则判为临近*/
bool aroundDot(const QPointF& pt_, const QPointF& pos_, qreal thresh_ = AROUND_THRESH);
bool aroundLine(const QLineF& line_, const QPointF& pos_, qreal thresh_ = AROUND_THRESH);
bool aroundCircle(const QPointF& center_, qreal radius_, const QPointF& pos_, qreal thresh_ = AROUND_THRESH);
bool aroundRectangle(const QRectF& rect_, const QPointF& pos_, qreal thresh_ = AROUND_THRESH);
bool aroundPolygon(const QVector<QPointF>& pts_, const QPointF& pos_, qreal thresh_ = AROUND_THRESH);
bool aroundText(const QPointF& pt_, const QString& text_, int angle_, const QFont& font_, const QPointF &pos_);

// In sth.
bool inCircle(const QPointF& center_, qreal radius_, const QPointF& pos_);
bool inRectangle(const QRectF& rect_, const QPointF& pos_);
bool inPolygon(const QVector<QPointF>& pts_, const QPointF& pos_);

// 几何计算
// -------
qreal dist(const QPointF& pt1_, const QPointF& pt2_);
QLineF verticalLineSegment(const QPointF& dot_, const QLineF& line_);
QPointF verticalPoint(const QPointF& dot_, const QLineF& line_);
QPointF circleCenter(const QPointF& pt1_, const QPointF& pt2_, const QPointF& pt3_);
qreal polygonArea(const QVector<QPointF>& pts_);
QLineF stretchLineSegment(const QLineF& line_, const QRectF& rect_);

#endif // DRAWING_FUNS_H
