#ifndef AG_ELEMENT_H
#define AG_ELEMENT_H

#include <QWidget>
#include "element.h"

// AG_Dot
class AG_Dot : public Element
{
public:
    explicit AG_Dot(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
};

// AG_LineSegment
class AG_LineSegment : public Element
{
public:
    explicit AG_LineSegment(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
};

// AG_Circle
class AG_Circle : public Element
{
public:
    explicit AG_Circle(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
};

// AG_Line
class AG_Line : public Element
{
public:
    explicit AG_Line(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
};

// AG_HorizontalLine
class AG_HorizontalLine : public Element
{
public:
    explicit AG_HorizontalLine(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
};

// AG_VerticalLine
class AG_VerticalLine : public Element
{
public:
    explicit AG_VerticalLine(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
};

// AG_ClockwiseCircle
class AG_ClockwiseCircle : public Element
{
public:
    explicit AG_ClockwiseCircle(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
};


// AG_LineSegment
class AG_ArrowedLineSegment : public Element
{
public:
    explicit AG_ArrowedLineSegment(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
};

// AG_ArrowedLine
class AG_ArrowedLine : public Element
{
public:
    explicit AG_ArrowedLine(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
};

// AG_ArrowedHorizontalLine
class AG_ArrowedHorizontalLine : public Element
{
public:
    explicit AG_ArrowedHorizontalLine(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
};

// AG_ArrowedVerticalLine
class AG_ArrowedVerticalLine : public Element
{
public:
    explicit AG_ArrowedVerticalLine(const ElementType& type_, const Target* target_);
    virtual void setIntention(const QPointF& pos_) override;
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) override;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) override;
};

#endif // AG_ELEMENT_H
