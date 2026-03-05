#include "img_viewer.h"
#include "qapplication.h"
#include "qmenu.h"
#include "uddm_namespace.h"
#include <cmath>
#include <QStyleOption>
#include <QFileDialog>

ImgViewer::ImgViewer(QWidget *parent)
    : QLabel{parent}
{
    // 图像
    m_img = QImage();
    // 位置相关参数
    m_center_offset = {0.0, 0.0};
    m_bounder_offset = {0.0, 0.0, 0.0, 0.0};
    m_last_pos = {0.0, 0.0};
    // 缩放因子
    m_scale_factor = 1;
    m_zoom_factor = 1;
    m_step2zoom_factor = 0.3;
    m_smooth_steps = 5;
    m_smooth_time = 80;
    m_zoom_min_thresh = 0.2;
    m_zoom_max_thresh = 40.0;
    // 贴图
    m_tag = QImage();
    m_tag_scale_factor = 1;

    // 开启鼠标跟踪
    this->setMouseTracking(true);

    // 设置背景色
    this->setStyleSheet("background-color: rgb(0, 0, 0);");
    // 几何/绘制标志位初始化
    m_never_painted = true;
    m_show_tag_synchronized_with_img = true;
}

/**
 * @brief 槽函数，接受一个QImage并在当前物理坐标系下自适应大小和位置
 * 进行显示，自适应位置为物理坐标系中央对其图像中央
 */
void ImgViewer::setImg(const QImage& img_, bool deep_copy_)
{
    // 图像为空时
    if(img_.isNull()){
        m_img = QImage();
        return;
    }
    // 图像
    // 深拷贝？
    if(deep_copy_)
        m_img = img_.copy();
    else
        m_img = img_;
    // 位置初始化
    geometoryAdaption();
    // 刷新
    update();
}

/**
 * @brief 槽函数，接受一个QImage并在当前物理坐标系下自适应其大小和位置
 * 进行显示，自适应位置为物理坐标系左侧对其图像左侧
 */
void ImgViewer::setTag(const QImage &tag_, bool deep_copy_)
{
    // 图像为空时
    if(tag_.isNull()){
        m_tag = QImage();
        return;
    }
    // 图像
    // 深拷贝？
    if(deep_copy_)
        m_tag = tag_.copy();
    else
        m_tag = tag_;
    // 位置初始化
    geometoryAdaption();
    // 刷新
    update();
}

/**
 * @brief 槽函数，接受一个QImage并在当前物理坐标系下按上一张图的位置和尺寸进行显示
 * 如果上一张图为空，则自适应调整大小和位置
 */
void ImgViewer::updateImg(const QImage &img_, bool deep_copy_, bool refresh_)
{
    if(m_img.isNull())
        setImg(img_, deep_copy_);
    else{
        // 深拷贝？
        if(deep_copy_)
            m_img = img_.copy();
        else
            m_img = img_;
        // 刷新界面？
        if(refresh_)
            update();
    }
}

/**
 * @brief 槽函数，接受一个QImage并在当前物理坐标系下按上一张Tag的位置和尺寸进行显示
 * 如果上一张图为空，则自适应调整大小和位置
 */
void ImgViewer::updateTag(const QImage &tag_, bool deep_copy_, bool refresh_)
{
    if(m_tag.isNull())
        setTag(tag_, deep_copy_);
    else{
        // 深拷贝？
        if(deep_copy_)
            m_tag = tag_.copy();
        else
            m_tag = tag_;
        // 刷新？
        if(refresh_)
            update();
    }
}

/** Debug begin*/
void ImgViewer::setImg()
{
    update();
    // 图像
    m_img = QImage(":/imgs/img1.png");
    // 位置
    m_center_offset = {0.0, 0.0};
    // 缩放
    qreal w_ratio = (qreal)this->width() / (qreal)m_img.width();
    qreal h_ratio = (qreal)this->height() / (qreal)m_img.height();
    m_scale_factor = qMin(w_ratio, h_ratio);
    m_zoom_factor = 1;
    // 刷新
    update();
}

void ImgViewer::setTag()
{
    setTag(prcoessLegendTag(Topo2DStyle::Grayscale, 25.222, 1));
}
/** Debug end*/

/**
 * @brief ImgViewer::showEvent
 * 当部件被显示时，将图像移植画面中央
 * 仅对内部事件做相应,不对最大最小化等系统事件做响应
 */
void ImgViewer::showEvent(QShowEvent *event)
{
    // 当该事件不是与窗口系统同时发生时
    if(!event->spontaneous())
        geometoryAdaption();
}

/**
 * @brief ImgViewer::paintEvent
 * 将当前pixmap进行坐标系变换后绘制在物理坐标系上
 */
void ImgViewer::paintEvent(QPaintEvent* event)
{
    // 如果没有图像，使用 QLabel 默认的绘制（显示文本）
    if (m_img.isNull()) {
        QLabel::paintEvent(event);
        return;
    }

    // 检测是否需要进行图像几何初始化
    if(m_never_painted){
        geometoryAdaption();
        m_never_painted = false;
    }
    // 自定义绘制
    m_n_nested_events += 1;
    updateBounderOffset();
    m_imgviewer_painter.begin(this);
    // 进行坐标转换 并 进行绘图
    // --------------------
    m_imgviewer_painter.setRenderHint(QPainter::SmoothPixmapTransform);
    // 世界坐标系（画笔坐标系）到逻辑坐标系（画布坐标系）的转换
    // Note: 以下转换通过矩阵右乘来实现， T1 × S1 × S2 × T2 × Coord_world = Coord_logical
    //       因此转换进行的顺序是先进行T2，再进行S2，再S1，T1，与书写顺序相反
    // -------------------------------------------------------------
    // 自定义移动
    m_imgviewer_painter.translate(m_center_offset);   // T1
    // 缩放
    m_imgviewer_painter.scale(m_scale_factor, m_scale_factor);  // S1
    m_imgviewer_painter.scale(m_zoom_factor, m_zoom_factor);    // S2
    // “图像中央” 移动到 “画布坐标系原点”
    m_imgviewer_painter.translate(QPointF(-m_img.width() / 2.0, -m_img.height() / 2.0));  // T2
    // 逻辑坐标系（画布坐标系）到物理坐标系（屏幕坐标系）的映射
    // “画布坐标系原点” 映射到 “屏幕中央”
    // -----------------------------
    m_imgviewer_painter.setWindow(QRect(0, 0, 100, 100));
    m_imgviewer_painter.setViewport(QRect((this->width()-1) / 2.0, (this->height()-1) / 2, 100, 100));
    // 图像以原大小画在 “画笔坐标系” 下
    m_imgviewer_painter.drawImage(m_img.rect(), m_img, m_img.rect());
    m_imgviewer_painter.end();

    // 绘制贴图
    // -------
    if(!m_show_tag_synchronized_with_img || !m_img.isNull()){
        m_tag_painter.begin(this);
        m_tag_painter.scale(m_tag_scale_factor, m_tag_scale_factor);
        m_tag_painter.drawImage(m_tag.rect(), m_tag, m_tag.rect());
        m_tag_painter.end();
    }

    m_n_nested_events -= 1;
}

/**
 * @brief ImgViewer::resizeEvent
 */
void ImgViewer::resizeEvent(QResizeEvent *event)
{
    // 更新边界
    updateBounderOffset();
    // 移至中心
    move(QPointF(0.0, 0.0));
    // 更新tag的缩放因子
    qreal w_ratio = (qreal)this->width() / (qreal)m_tag.width();
    qreal h_ratio = (qreal)this->height() / (qreal)m_tag.height();
    m_tag_scale_factor = qMin(w_ratio, h_ratio);
    // 刷新界面
    update();
}

/**
 * @brief ImgViewer::wheelEvent
 * 响应滚轮事件（支持触摸板）对图像进行缩放，以滚轮滚过15°为一个缩放单位
 */
void ImgViewer::wheelEvent(QWheelEvent *event)
{
    m_n_nested_events += 1;
    // 读取步长，15°为1步
    // 滚轮灵敏度一般为1/8°，但一般每滚过15°才触发一次wheelEvent
    int step = event->angleDelta().y() / 8 / 15;
    // 缩放
    smooth_zoom(step * m_step2zoom_factor * m_zoom_factor);
    update();
    m_n_nested_events -= 1;
}

/**
 * @brief ImgViewer::mouseMoveEvent
 * 响应鼠标移动事件，记录当前鼠标位置到成员变量last_pos
 * 当鼠标左键按下时，图像随鼠标移动，位移量与鼠标移动量相同
 */
void ImgViewer::mouseMoveEvent(QMouseEvent *event)
{
    m_n_nested_events += 1;
    QPointF pos = event->pos();

    Qt::MouseButtons btns = event->buttons();

    if (Qt::LeftButton == (btns & Qt::LeftButton))
        move(pos - m_last_pos);

    m_last_pos = pos;
    update();
    m_n_nested_events -= 1;
}

/**
 * @brief ImgViewer::mousePressEvent
 * 当鼠标右键按下时，弹出菜单
 */
void ImgViewer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
        showContextMenu(event->pos());
    else
        QLabel::mousePressEvent(event);
}

/**
 * @brief 缩放图像
 * 以光标所在位置为基准点进行图像缩放
 * 同时返回一个BounderCrossingType类型的量，用于指示此次更新是否使图像边界跨越屏幕边界
 *
 * @param factor_ 加性缩放因子，正值表放大，负值表缩小
 */
ImgViewer::BounderCrossingType ImgViewer::zoom(qreal factor_)
{
    // 缩放因子
    // -------
    qreal factor = 0.0;
    if(m_zoom_factor + factor_ < m_zoom_min_thresh)
        factor = m_zoom_min_thresh - m_zoom_factor;
    else if(m_zoom_factor + factor_ > m_zoom_max_thresh)
        factor = m_zoom_max_thresh - m_zoom_factor;
    else
        factor = factor_;
    m_zoom_factor += factor;

    // 位移，以使保持光标下的点在屏幕中的位置尽量不变
    // --------------------------------------
    // 读取缩放前的数据
    QPointF cursor_logcial = physicalToLogical(m_last_pos);
    QPointF cursor_offset_logical = cursor_logcial - QPointF(m_img.width() / 2, m_img.height() / 2);
    // 计算需要的位移量
    QPointF move_vec = -cursor_offset_logical * m_scale_factor * factor;
    // 将数据更新到缩放后,同时接收跨越边界的信息
    BounderCrossingType cross_bounder = updateBounderOffset();
    // 位移
    move(move_vec);

    return cross_bounder;
}

/**
 * @brief 平滑缩放
 * 通过添加中间图像来对图像缩放过程进行平滑
 * 平滑效果通过smooth_steps, smooth_time两个类中的成员变量控制
 * 同时考虑图像边界和屏幕边界的贴合
 *
 * @param factor_ 见zoom(qreal factor_)函数
 */
void ImgViewer::smooth_zoom(qreal factor_)
{
    m_is_outmost_layer = true;
    bool quit = false;

    QElapsedTimer t;
    qreal factor = factor_ / m_smooth_steps;

    for(int i = 0; i < m_smooth_steps; i++){
        // 判断是否跨越边界
        BounderCrossingType cross_bounder = zoom(factor);
        // 当图像两侧边缘刚好触碰到屏幕边界时，停止本次缩放
        if(cross_bounder == BounderCrossingType::HCross){
            moveToCenter(false, true);
            quit = true;
        }
        if(cross_bounder == BounderCrossingType::VCross){
            moveToCenter(true, false);
        quit = true;
        }

        // 平滑缩放
        t.start();
        while(t.elapsed() < m_smooth_time / m_smooth_steps)
            if(m_n_nested_events < 32)
                QCoreApplication::processEvents();
        // 当有新的缩放事件产生，本次缩放将会超时，停止本次缩放
        if(t.elapsed() > m_smooth_time + 200)
            quit = true;

        // 是否放弃
        if(quit | !m_is_outmost_layer)
            break;
    }
    m_is_outmost_layer = false;
}

/**
 * @brief 移动图像
 * 按矢量vector_移动图像，同时考虑图像边缘与控件边缘的位置，
 * 对位移矢量做必要的修正，以实现必要的贴合和对称效果。
 * 具体效果结合程序运行感受
 *
 * @param vector_ 物理坐标系下的位移矢量
 */
void ImgViewer::move(QPointF vector_)
{
    // 边界情况处理
    // ----------
    // 1.上下边界
    // ----------
    // 屏幕可完整容纳图像 - 移至中央
    if(compareWithZero(m_img.height() * m_scale_factor * m_zoom_factor - this->height()) != 1)
        vector_.setY(-m_center_offset.ry());
    // 屏幕不可完整容纳图像
    // 但移动后会图像边缘进入边界内 - 图像边缘移动到边界上
    else if(compareWithZero(m_bounder_offset.top - vector_.ry(), 1.0) == -1)
        vector_.setY(+m_bounder_offset.top);
    else if(compareWithZero(m_bounder_offset.bottom + vector_.ry(), 1.0) == -1)
        vector_.setY(-m_bounder_offset.bottom);
    // 2.左右边界
    // ----------
    // 屏幕可完整容纳图像 - 移至中央
    if(compareWithZero(m_img.width() * m_scale_factor * m_zoom_factor - this->width()) != 1)
        vector_.setX(-m_center_offset.rx());
    // 屏幕不可完整容纳图像
    // 但移动后会图像边缘进入边界内 - 图像边缘移动到边界上
    else if(compareWithZero(m_bounder_offset.left - vector_.rx(), 1.0) == -1)
        vector_.setX(+m_bounder_offset.left);
    else if(compareWithZero(m_bounder_offset.right + vector_.rx(), 1.0) == -1)
        vector_.setX(-m_bounder_offset.right);
    // 获得最终的移动量
    // -------------
    m_center_offset += vector_;
    update();
}

/**
 * @brief 将图像移动至物理坐标系中心
 */
void ImgViewer::moveToCenter(bool x_direction_, bool y_direction_)
{
    if(x_direction_)
        m_center_offset.setX(0);
    if(y_direction_)
        m_center_offset.setY(0);
}

/**
 * @brief 根据当前的位移、缩放相关参数，更新边界偏移量
 * 同时返回一个BounderCrossingType类型的量，用于指示此次更新是否使图像边界跨越屏幕边界
 */
ImgViewer::BounderCrossingType ImgViewer::updateBounderOffset()
{
    BounderOffset diff = m_bounder_offset;
    m_bounder_offset = {
        (m_scale_factor * m_zoom_factor * m_img.height() - this->height())/ 2 - m_center_offset.ry(),
        (m_scale_factor * m_zoom_factor * m_img.height() - this->height())/ 2 + m_center_offset.ry(),
        (m_scale_factor * m_zoom_factor * m_img.width() - this->width())/ 2 - m_center_offset.rx(),
        (m_scale_factor * m_zoom_factor * m_img.width() - this->width())/ 2 + m_center_offset.rx()
    };
    // 判断是否跨越边界
    diff = diff * m_bounder_offset;
    if(diff.top < 0 || diff.bottom < 0)
        return BounderCrossingType::VCross;
    if(diff.left < 0 || diff.right < 0)
        return BounderCrossingType::HCross;
    else
        return BounderCrossingType::NoCross;
}

/**
 * @brief 图像几何初始化
 * 将图像移至画面中央
 * 将图像/贴纸图像大小自适应
 */
void ImgViewer::geometoryAdaption()
{
    // 更新缩放比例
    qreal w_ratio_pixmap = (qreal)this->width() / (qreal)m_img.width();
    qreal h_ratio_pixmap = (qreal)this->height() / (qreal)m_img.height();
    m_scale_factor = qMin(w_ratio_pixmap, h_ratio_pixmap);
    qreal w_ratio_tag = (qreal)this->width() / (qreal)m_tag.width();
    qreal h_ratio_tag = (qreal)this->height() / (qreal)m_tag.height();
    m_tag_scale_factor = qMin(w_ratio_tag, h_ratio_tag);
    m_zoom_factor = 1.0;
    // 移动至中央
    moveToCenter();
    // 更新边界
    updateBounderOffset();
}

/**
 * @brief 物理坐标系位置到逻辑坐标系的转换
 */
QPointF ImgViewer::physicalToLogical(QPointF pos_)
{
    // 以浮点型运算
    QPointF physical_pos = pos_;
    QPointF physical_offset = physical_pos - QPointF(this->width() - 1, this->height() - 1) / 2.0;
    QPointF logical_offset = (-m_center_offset + physical_offset) / m_scale_factor / m_zoom_factor;
    QPointF logical_pos = QPointF(m_img.width() - 1, m_img.height() - 1) / 2.0 + logical_offset;
    return logical_pos;
}

/**
 * @brief 逻辑坐标系位置到物理坐标系的转换
 */
QPointF ImgViewer::logicalToPhysical(QPointF pos_)
{
    // 以浮点型运算
    QPointF logical_pos = pos_;
    QPointF logical_offset = logical_pos - QPointF(m_img.width() - 1, m_img.height() - 1) / 2.0;
    QPointF physical_offset = logical_offset * m_scale_factor * m_zoom_factor + m_center_offset;
    QPointF physical_pos = physical_offset + QPointF(this->width() - 1, this->height() - 1) / 2.0;
    return physical_pos;
}

/**
 * @brief ImgViewer::showContextMenu
 * 显示右键菜单，当前菜单中仅单一功能“保存”，用来保存当前图像
 */
void ImgViewer::showContextMenu(const QPoint &pos)
{
    // 右键菜单
    QMenu menu(tr("Menu"), this);
    menu.setStyleSheet("background-color: gray;");
    // 保存
    QPixmap pixmap = this->grab();
    QAction *save_action = menu.addAction(tr("保存"));
    connect(save_action, &QAction::triggered, this, [=]() {
        QString filename = QFileDialog::getSaveFileName(this, tr("保存文件"), QApplication::applicationDirPath(), tr("Images (*.png *.tiff *.jpg)"));
        if (!filename.isEmpty())
            pixmap.save(filename);
    });
    // 居中
    QAction *center_action = menu.addAction(tr("居中"));
    connect(center_action, &QAction::triggered, this, [=]() {
        this->geometoryAdaption();
    });
    // 运行
    menu.exec(mapToGlobal(pos));
}
