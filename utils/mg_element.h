#ifndef MG_ELEMENT_H
#define MG_ELEMENT_H

#include <vector>
#include <QString>
#include "uddm_namespace.h"
#include "canvas_namespace.h"
#include "element.h"

// MM
// --
// MM_DeltaXY
class MM_DeltaXY : public Element
{
public:
    explicit MM_DeltaXY(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
    virtual qreal calculateResult(bool recalculate_ = false) override;
};

// MM_DeltaZ
class MM_DeltaZ : public Element
{
public:
    explicit MM_DeltaZ(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
    virtual qreal calculateResult(bool recalculate_ = false) override;
};

// MM_Radius
class MM_Radius : public Element
{
public:
    explicit MM_Radius(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
    QPointF calculateCenter();
    virtual qreal calculateResult(bool recalculate_ = false) override;
};

// MM_Diameter
class MM_Diameter : public Element
{
public:
    explicit MM_Diameter(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
    virtual qreal calculateResult(bool recalculate_ = false) override;
};

// MM_Radian
class MM_Radian : public Element
{
public:
    explicit MM_Radian(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
    virtual qreal calculateResult(bool recalculate_ = false) override;
};

// MM_Count
class MM_Count : public Element
{
public:
    explicit MM_Count(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void setEndPoint(const QPointF& pos_) override;
    virtual void stepBack() override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
    virtual qreal calculateResult(bool recalculate_ = false) override;

protected:
    const int MAX_COUNT = 200;
};

// AM
// --
// AM_Circle
class AM_Circle : public Element
{
public:
    explicit AM_Circle(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
    virtual qreal calculateResult(bool recalculate_ = false) override;
};

// AM_Rect
class AM_Rect : public Element
{
public:
    explicit AM_Rect(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
    virtual qreal calculateResult(bool recalculate_ = false) override;
};

// AM_Polygon
class AM_Polygon : public Element
{
public:
    explicit AM_Polygon(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void setEndPoint(const QPointF& pos_) override;
    virtual void stepBack() override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
    virtual qreal calculateResult(bool recalculate_ = false) override;

protected:
    const int MAX_N_EDGES = 200;
};

#endif // MG_ELEMENT_H
