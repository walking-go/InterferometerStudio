#ifndef CANVAS_NAMESPACE_H
#define CANVAS_NAMESPACE_H

#include <QPen>
#include <QFont>

class Drawer;
class Element;
class M2dElement;
class ImgViewer;
class ElementCanvas;

// 测量相关
// ------
const int DIST_PRECISION = 2;   /**< 距离显示精度*/
const int AREA_PRECISION = 2;   /**< 面积显示精度*/
const int DEGREE_PRECISION = 1; /**< 角度显示精度*/

// 绘制相关
// ------
// 图元是绘图时的最小单位
// 包括：Dot, Line, Circle, Rectangle, Polygon, Text
// 其中Dot和Line有不同的子类型
enum DotType
{
    Type_NoDot = 0,
    Type_CrossDot,      /**< 叉号*/
    Type_FilledDot      /**< 实心圆点*/
};

enum LineType
{
    Type_NoLine = 0,
    Type_SimpleLine,
    Type_SingleArrowedLine, /**< 一端带箭头*/
    Type_DoubleArrowedLine, /**< 两端带箭头*/
    Type_VectorLine         /**< 中间带箭头*/
};

enum CircleType
{
    Type_NoCircle = 0,
    Type_SimpleCircle,
    Type_ClockwiseCircle,
    Type_AntiClockwiseCircle
};

// Element是图元的组合
/**
 * @brief The ElementMode enum 元素的绘制模式
 */
enum ElementDrawingMode
{
    Normal,     /**< 正常*/
    Emphasize   /**< 强调(线条加粗，颜色加深)*/
};

/**
 * @brief The ToolBox class 绘画工具箱
 */
struct ToolBox
{
    // 画笔
    QPen basic_pen = QPen(Qt::black);       /**< 基础画笔*/
    QPen measurement_pen = QPen(Qt::red);   /**< 测量指示画笔*/
    QPen main_pen = QPen(Qt::blue);         /**< 主要图形画笔*/
    QPen assist_pen = QPen(Qt::yellow);     /**< 辅助画笔*/
    // 字体
    QFont basic_font = QFont("Times", 20, QFont::Bold); /**< 基础字体*/
    QPen font_pen = QPen(Qt::red);                      /**< 文字画笔*/
    QBrush font_bg_brush = QBrush(Qt::white);           /**< 文字背景颜色*/
    // 面积
    QBrush area_brush = QBrush(Qt::transparent);
};

const int MIN_LINE_WIDTH = 1;   /**< 允许的最小线宽*/
const int MAX_LINE_WIDTH = 20;  /**< 允许的最大线宽*/
const int DELTA_LINE_WIDTH = 3; /**< 变化线宽对某线条进行弱化 or 强调时, 线宽的变动量*/

// 状态
// ---
/**
 * @brief The ElementStatus enum 元素状态
 *
 * 元素点集中新点的添加需要两步：
 * 1. 画家提笔并移动，过程中笔经过的点被设为意向点
 * 2. 画家落笔
 * 画家落笔前元素为草稿状态Draft
 * 画家落笔后元素进入稳定状态Settled，直到画家再次提笔产生新的意向点
 */
enum ElementStatus
{
    Empty,  /**< 空*/
    Draft,  /**< 草稿状态*/
    Settled /**< 稳定状态*/
};

/**
 * @brief The DrawerStatus enum 画家状态
 *
 * 当没有新元素要构建时，画家为空闲状态Leisure
 * 当有新元素正在构建时，画家为创作状态Composing
 */
enum DrawerStatus
{
    Leisure,    /**< 空闲状态*/
    Composing   /**< 创作状态*/
};

/**
 * @brief The ElementType enum
 */
enum ElementType
{
    Type_NoElement,
    Type_DeleteElement,
    Type_TestRectangle, /**< 用于测试的矩形元素*/

    // MG(MeasurementGraphic),包括MM,AM两类
    // -----------------------------------
    Type_MG_Bottom,
    // MM（MainMeasurement）类
    Type_MM_Bottom,
    Type_MM_DeltaXY,
    Type_MM_DeltaZ,
    Type_MM_Radius,
    Type_MM_Diameter,
    Type_MM_Radian,
    Type_MM_Count,
    Type_MM_Top,
    // AM（AreaMeasurement）类
    Type_AM_Bottom,
    Type_AM_Circle,
    Type_AM_Rect,
    Type_AM_Polygon,
    Type_AM_Top,
    Type_MG_Top,
    // AG（AuxiliaryGraphic）类
    // -----------------------
    Type_AG_Bottom,
    Type_AG_Dot,
    Type_AG_LineSegment,
    Type_AG_Circle,
    Type_AG_Line,
    Type_AG_HorizontalLine,
    Type_AG_VerticalLine,
    Type_AG_ClockwiseCircle,
    Type_AG_ArrowedLineSegment,
    Type_AG_ArrowedLine,
    Type_AG_ArrowedHorizontalLine,
    Type_AG_ArrowedVerticalLine,
    Type_AG_Top
};
#endif // CANVAS_NAMESPACE_H
