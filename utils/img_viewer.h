#ifndef IMG_VIEWER_H
#define IMG_VIEWER_H

#include <math.h>

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QDebug>
#include <QElapsedTimer>
#include <QPushButton>
#include <QHBoxLayout>
#include <QCoreApplication>

/**
 * @brief The ImgViewer class
 *
 * 图像查看器，继承自QLabel类
 * 功能：
 * · setImg(QImage)函数设置图像，并将图片自适应地放置于屏幕中央
 * · 响应滚轮滚动事件，实现图像缩放，支持平滑缩放
 * · 响应鼠标点击并移动事件，实现图像移动
 * · setTag(QImage)函数设置贴纸图像，并将图片自适应地放置于屏幕左上角，该图像覆盖m_img显示，且无法缩放、移动
 * 特点：
 * · 多事件同时响应，互不冲突，例如可以在平滑缩放的同时移动图像
 * · 图像缩放时，自动贴合边缘，强迫症患者福音
 * · 当图像边缘未超出边界时，图像保持在屏幕正中，不允许移动
 * · 当图像边缘超出边界时，图像缩放保持光标位置下的点不动
 */
class ImgViewer : public QLabel
{
    Q_OBJECT

public:
    explicit ImgViewer(QWidget *parent = nullptr);
    /**
     *  @brief 图片显示时，其边缘超出屏幕的距离
     *  其值有如下三种情况：
     *      <0 未超出屏幕
     *      ≈0 刚好处于边缘
     *      >0 超出边缘
     *  数值单位为物理坐标系(physical)单位
     */
    struct BounderOffset{
        qreal top;      /**< 上边缘*/
        qreal bottom;   /**< 下边缘*/
        qreal left;     /**< 左边缘*/
        qreal right;    /**< 右边缘*/
        // 重写*方法，用于辨别缩放过程是否跨越边界
        BounderOffset operator*(const BounderOffset& other) const {
            BounderOffset result;
            result.top = top * other.top;
            result.bottom = bottom * other.bottom;
            result.left = left * other.left;
            result.right = right * other.right;
            return result;
        }
    };

    /**
     * @brief 图像缩放过程中，图像边界可能会跨越屏幕边界
     * 该枚举类指示跨越的类型
     */
    enum BounderCrossingType{
        NoCross,    /**< 未跨越*/
        HCross,     /**< 水平跨越*/
        VCross      /**< 垂直跨越*/
    };

signals:

public slots:
    void setImg(const QImage& img_, bool deep_copy_ = false);
    void setTag(const QImage& tag_, bool deep_copy_ = false);
    void updateImg(const QImage& img_, bool deep_copy_ = false, bool refresh_ = true);
    void updateTag(const QImage& tag_, bool deep_copy_ = false, bool refresh_ = true);
    void setImg();
    void setTag();
    QImage getImg(){return m_img; };
    QImage getTag(){return m_tag; };
    // 逻辑 <-> 物理 坐标系转换
    QPointF physicalToLogical(QPointF pos_);
    QPointF logicalToPhysical(QPointF pos_);
    // 菜单
    void showContextMenu(const QPoint &pos);

protected:
    QPainter m_imgviewer_painter;
    // 图像
    QImage m_img;
    // 位置相关参数
    QPointF m_center_offset;          /**< 图像中心相对屏幕中心偏移量， 单位：物理坐标系单位*/
    BounderOffset m_bounder_offset;   /**< 图片显示时，其边缘超出屏幕的距离, 单位：物理坐标系单位*/
    QPointF m_last_pos;               /**< 鼠标上一次所在位置，坐标系：物理坐标系*/
    // 缩放因子 pixmap.size = this.size / scale_factor / zoom_factor
    qreal m_scale_factor;             /**< 图片第一次展示时用到的缩放因子，以自适应地调整到贴合窗口的尺寸*/
    qreal m_zoom_factor;              /**< 在自适应尺寸的基础上，进行二次缩放的因子*/
    qreal m_step2zoom_factor;         /**< 鼠标滚轮每滚动15°对应zoom_factor的变化量*/
    int m_smooth_steps;               /**< 平滑缩放的步数*/
    qreal m_smooth_time;              /**< 平滑缩放的时间, 单位是ms*/
    qreal m_zoom_min_thresh;          /**< zoom_factor最小值*/
    qreal m_zoom_max_thresh;          /**< zoom_factor最大值*/
    // 贴图 tag
    QPainter m_tag_painter;
    QImage m_tag;                    /**< 贴纸图像，该图像在m_img上覆盖显示，在屏幕坐标系上的位置固定，不可缩放，用来显示图例等内容*/
    qreal m_tag_scale_factor;        /**< 用来自适应贴纸大小的缩放因子*/
    // 标志位
    // -----
    // 用来解决事件嵌套造成的麻烦
    bool m_is_outmost_layer = true;       /**< 指示当前平滑缩放是否是最外层，如果当前平滑缩放处理过程中，有新的平滑缩放产生，则该标志位被置为false*/
    int m_n_nested_events = 0;            /**< 嵌套的事件层数*/
    // 几何/绘制相关
    bool m_never_painted;                 /**< 指示是否执行过paintEvent, 初次执行时进行几何初始化(initGeometory)
                                            , 用来解决窗口初次显示时showEvent给出的this->width/height不准确的问题*/
    bool m_show_tag_synchronized_with_img;/**< 指示tag是否只在img不为空时显示*/

    // 重写函数
    virtual void showEvent(QShowEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;

    // 自定义函数
    BounderCrossingType zoom(qreal factor_);
    void smooth_zoom(qreal factor_);
    void move(QPointF vec_);
    void moveToCenter(bool x_direction_ = true, bool y_direction_ = true);
    BounderCrossingType updateBounderOffset();
    void geometoryAdaption();
};

#endif // IMG_VIEWER_H
