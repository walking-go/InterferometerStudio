#ifndef SURFACEAREA_H
#define SURFACEAREA_H

#include "element.h"
#include "uddm_namespace.h"

#include <QWidget>
#include <QMainWindow>
#include <QtDataVisualization>
#include <QBoxLayout>
#include <Qt3DCore/QEntity>
#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/QSurfaceDataProxy>
#include <QtDataVisualization/QSurface3DSeries>
#include <QtDataVisualization/QSurfaceDataArray>
#include <QtDataVisualization/QSurfaceDataRow>

#include <time.h>
#include <QDebug>
#include <QtGlobal>
#include <algorithm>

QT_BEGIN_NAMESPACE
using namespace QtDataVisualization;
namespace Ui { class Widget; }
QT_END_NAMESPACE

/** @brief ScaleRenderType
 *  枚举了标尺刻度绘制的依据类型
 *  表明刻度是根据地形图、平面或是圆柱面绘制的
 */
enum ScaleRenderType
{
    TopoType,
    PlaneType,
    CylinderType
};

class SurfaceArea : public QWidget
{
    Q_OBJECT

public:
    SurfaceArea(QWidget *parent = nullptr);
    ~SurfaceArea();

    // 初始化函数 主要设置Q3DSurface相关参数
    void initTopo();
    void initCylinder();
    void initPlane();

    // 更新topo数据
    void updateTopo();

    // 绘制函数
    void renderTopo();
    void renderCylinder(QPointF pos0_, qreal R_);
    void renderPlane(QPointF pos1_, QPointF pos2_);

    // 更新坐标轴刻度值
    void updateScale();

    // 计算坐标轴最大最小刻度值
    float calMaxScaleXZ(float num_);
    float calMaxScaleY(float num_);
    float calMinScaleXZ(float num_);

public:
    QWidget* m_widget;
    Q3DSurface* m_graph;
    ScaleRenderType m_scale_render_type = TopoType;
    const Target* m_target = nullptr;
    const Element* m_element = nullptr;

    // 地形图相关变量
    QSurface3DSeries* m_series = nullptr;               /**< topo的series指针*/
    QSurfaceDataProxy* m_proxy = nullptr;               /**< topo的proxy指针*/
    QImage m_topo;                                      /**< 地形图像*/
    QImage m_topo_texture;                              /**< 地形纹理*/

    int m_img_width;                                    /**< topo图像宽度*/
    int m_img_height;                                   /**< topo图像高度*/
    float m_img_x;                                      /**< topo图像对应到x轴上的长度，单位：um*/
    float m_img_y;                                      /**< topo图像对应到y轴上的长度，单位：um*/
    float m_img_z;                                      /**< topo图像对应到z轴上的长度，单位：um*/

    // 圆柱面相关变量
    QSurface3DSeries* m_cylinder_series = nullptr;      /**< 圆柱面的series指针*/
    QSurfaceDataProxy* m_cyliner_proxy;                 /**< 圆柱面的proxy指针*/
    QVector<qreal> m_cylinder_range_x;                  /**< 圆柱面的对应到x轴上的范围，单位：um*/
    QVector<qreal> m_cylinder_range_z;                  /**< 圆柱面的对应到z轴上的范围，单位：um*/

    float m_cylinder_radius;                            /**< 圆柱面半径*/
    float m_cylinder_height;                            /**< 圆柱面高度*/
    int m_cylinder_points_num;                          /**< 圆柱面绘制采样点个数*/

    // 平面相关变量
    QSurface3DSeries* m_plane_series = nullptr;         /**< 平面的series指针*/
    QSurfaceDataProxy* m_plane_proxy = nullptr;         /**< 平面的proxy指针*/
    QVector<qreal> m_plane_range_x;                     /**< 平面的对应到x轴上的范围，单位：um*/
    QVector<qreal> m_plane_range_z;                     /**< 平面的对应到z轴上的范围，单位：um*/

    float m_plane_length;                               /**< 平面长度*/
    float m_plane_height;                               /**< 平面高度*/
    int m_plane_num;                                    /**< 平面绘制采样点个数*/

    // 比例系数
    qreal m_real_per_pix = 1;                           /**< 单位像素长度对应的实际长度值，单位：um*/
    qreal m_real_per_gray_level = 2;                    /**< 单位灰度等级对应的实际高度值，单位：um*/

    // 刻度范围
    float m_max_x;
    float m_max_y;
    float m_max_z;
    float m_min_x;
    float m_min_y;
    float m_min_z;

signals:
    void zoomLevelChanged(float zoom_level_);
    void cameraPositionChanged(const QVector3D& position_);
    void xRotationChanged(float xRotation);
    void yRotationChanged(float yRotation);

public slots:
    void setTarget(const Target& target_);
    void setElement(const Element* element_);
    void clearElement();

    void syncZoomLevel(float zoom_level_)
    {
        m_graph->scene()->activeCamera()->setZoomLevel(zoom_level_);
    }
    void syncXRotation(float x_rotation_)
    {
        m_graph->scene()->activeCamera()->setXRotation(x_rotation_);
    }
    void syncYRotation(float y_rotation_)
    {
        m_graph->scene()->activeCamera()->setYRotation(y_rotation_);
    }
};
#endif // SURFACEAREA_H
