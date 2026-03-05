#ifndef CURVE_CHART_H
#define CURVE_CHART_H

#include "element.h"
#include "uddm_namespace.h"

#include <QWidget>
#include <QtCharts/QtCharts>
#include <QRandomGenerator>
#include <QtMath>

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QMouseEvent;
class QResizeEvent;
QT_END_NAMESPACE

QT_CHARTS_BEGIN_NAMESPACE
    class QChart;
QT_CHARTS_END_NAMESPACE
    class Callout;
QT_CHARTS_USE_NAMESPACE

class CurveChart : public QChartView
{
    Q_OBJECT
public:
    explicit CurveChart(QWidget  *parent = nullptr);
    ~CurveChart();

    void initAxis(QString name_x_, qreal min_x_, qreal max_x_, int tick_x_, \
                  QString name_y_, qreal min_y_, qreal max_y_, int tick_y_);
    void resetAxis();
    void buildChart();
    void loadHeigntData();

    void findCircleData(QPointF pos0_, qreal r_);
    void findStraightData(QPointF pos1_, QPointF pos2_);

    void findCircleHeight();
    void findStraightHeight();

    qreal calRealStrightDistance(QPointF pos1_, QPointF pos2_);
    qreal calRealCircumference(qreal r_);

    QVector<qreal> findRoundPointHeight(qreal xf_, qreal yf_);
    qreal quadraticSpline(qreal xf_, qreal yf_, qreal round_height_1_, qreal round_height_2_, qreal round_height_3_, qreal round_height_4_);

    qreal cubicSpline(qreal xf_, qreal yf_, qreal round_height_1_, qreal round_height_2_, qreal round_height_3_, qreal round_height_4_);//三次插值
public:
    QImage m_topo;                              /**< 地形图像*/

    QPointF* m_straight_data2D = nullptr;       /**< 直线数据点坐标(x,y)*/
    QPointF* m_circle_data2D = nullptr;         /**< 圆周数据点坐标(x,y)*/

    int m_straight_num;                         /**< 直线数据点个数*/
    int m_circle_num;                           /**< 圆周数据点个数*/

    qreal m_pix2real_ratio = 1.0;               /**< pix2real_ratio，一个像素间距对应的实际坐标间距，单位um*/
    qreal m_resolution = 1;                   /**< resolution，分辨率即对topo采样时相邻像素数据点的实际坐标间距，单位um*/
    qreal m_resolution_facotr = 200.0;           /**< 分辨率系数，Target.scope = resolution * resolution_facotr，单位um*/
    qreal m_max_sampling_pts = 5000;             /**< 最大采样点数*/

private:
    const Target* m_target = nullptr;           /**< Target指针*/
    const Element* m_element = nullptr;         /**< Element指针*/
    QChart* m_chart = nullptr;                  /**< QChart指针*/

    qreal m_height;                             /**< 数据点高度*/
    qreal m_max_height;                         /**< 数据点最大高度*/
    qreal m_max_length;                         /**< 轮廓线长度*/
    QList<QPointF> m_height_data;               /**< 所有数据点的高度坐标*/

    QLineSeries* m_series = nullptr;            /**< QLineSeries指针，用于存放series*/

    QValueAxis* m_axis_x = nullptr;             /**< 曲线X轴*/
    QValueAxis* m_axis_y = nullptr;             /**< 曲线Y轴*/
    QString m_chart_name;                       /**< 图像名称*/
    QString m_name_x;                           /**< X轴名称*/
    QString m_name_y;                           /**< Y轴名称*/
    qreal m_max_x;                              /**< X轴最大值*/
    qreal m_max_y;                              /**< Y轴最大值*/
    qreal m_min_x;                              /**< X轴最小值*/
    qreal m_min_y;                              /**< Y轴最小值*/

    int m_tick_x;                               /**< X轴刻度线个数*/
    int m_tick_y;                               /**< Y轴刻度线个数*/
    int m_min_scale_x;                          /**< X轴最小刻度值*/
    int m_min_scale_y;                          /**< Y轴最小刻度值*/

public slots:
    void setTarget(const Target&target_);
    void setElement(const Element* element_);
    void clearElement();
    void refresh();

};

#endif // CURVE_CHART_H
