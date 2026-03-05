#include "surface_area.h"

/**
 * @brief SurfaceArea::SurfaceArea
 * 构造函数
 * 初始化Q3DSurface类成员变量后需要将其放入QWidget中并显示
 * 设置了m_graph的坐标轴、主题、Layout，
 * 执行了初始化函数
 */
SurfaceArea::SurfaceArea(QWidget *parent)
    : QWidget(parent)
{
    m_graph = new Q3DSurface;
    // 在Q3DSurface中，直角坐标系为左手系，且y轴竖直向上表示高度
    m_graph->setAxisX(new QValue3DAxis);
    m_graph->setAxisY(new QValue3DAxis);
    m_graph->setAxisZ(new QValue3DAxis);
    // 设置z轴反向，变为右手系
    m_graph->axisZ()->setReversed(true);

    // 设置坐标轴标题，y z轴标题互换是为了使软件使用符合习惯，将z轴作为高度轴
    m_graph->axisX()->setTitle("x");
    m_graph->axisY()->setTitle("z");
    m_graph->axisZ()->setTitle("y");
    m_graph->axisX()->setTitleVisible(true);
    m_graph->axisY()->setTitleVisible(true);
    m_graph->axisZ()->setTitleVisible(true);
    m_graph->axisX()->setRange(0,1);
    m_graph->axisY()->setRange(0,1);
    m_graph->axisZ()->setRange(0,1);

    // 设置坐标轴标签自动旋转
    m_graph->axisX()->setLabelAutoRotation(30);
    m_graph->axisY()->setLabelAutoRotation(90);
    m_graph->axisZ()->setLabelAutoRotation(30);

    // 设置主题
    m_graph->activeTheme()->setType(Q3DTheme::ThemePrimaryColors);
    m_graph->activeTheme()->setLightStrength(0.5f);
    m_graph->activeTheme()->setAmbientLightStrength(0.8f);
    m_graph->activeTheme()->setSingleHighlightColor(QColor(255, 0, 0));
    m_graph->setShadowQuality(QAbstract3DGraph::ShadowQualityNone);
    m_graph->setMargin(0.2);

    // 设置坐标轴字体
    QFont font = m_graph->activeTheme()->font();
    font.setPointSize(font.pointSize() + 15); // 加大字体
    font.setBold(true); // 加粗字体
    m_graph->activeTheme()->setFont(font);

    // 设置Ui
    QVBoxLayout* vlayout = new QVBoxLayout(this);
    QSizePolicy size_policy = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_widget = new QWidget(this);
    m_widget = createWindowContainer(m_graph);
    m_widget->setSizePolicy(size_policy);
    vlayout->addWidget(m_widget);
    vlayout->setMargin(0);
    this->setLayout(vlayout);

    // 信号槽
    connect(m_graph->scene()->activeCamera(), &Q3DCamera::zoomLevelChanged, this, &SurfaceArea::zoomLevelChanged);
    connect(m_graph->scene()->activeCamera(), &Q3DCamera::xRotationChanged, this, &SurfaceArea::xRotationChanged);
    connect(m_graph->scene()->activeCamera(), &Q3DCamera::yRotationChanged, this, &SurfaceArea::yRotationChanged);

    // 初始化
    initTopo();
    initCylinder();
    initPlane();
}

/**
 * @brief SurfaceArea::~SurfaceArea
 * 析构函数
 */
SurfaceArea::~SurfaceArea()
{
    // 根据Qt示例，只需delete指针m_graph
    delete m_graph;
}

/**
 * @brief SurfaceArea::initTopo
 * 初始化地形图参数，创建相关变量并且设置m_series的标签格式、绘制模式和阴影模式
 */
void SurfaceArea::initTopo()
{
    // 创建一个QSurfaceDataProxy对象
    m_proxy = new QSurfaceDataProxy;
    // 创建一个QSurface3DSeries对象，并将这个QSurfaceDataProxy对象添加到它中
    m_series = new QSurface3DSeries(m_proxy);
    m_series->setItemLabelFormat(QStringLiteral("@xLabel, @zLabel, @yLabel"));
    m_series->setDrawMode(QSurface3DSeries::DrawSurface);
    m_series->setFlatShadingEnabled(false);
}

/**
 * @brief SurfaceArea::updateTopo
 * 更新topo图像数据
 * 仅当m_target->topo不为空时更新数据
 */
void SurfaceArea::updateTopo()
{
    // 读取图像数据
    if(!m_target->topo.isNull()){
        // 读图
        m_topo = m_target->topo;
        m_topo_texture = m_target->topo2d;
        m_series->setTexture(m_topo_texture);

        // 获取图像宽高
        m_img_width = m_topo.width();
        m_img_height = m_topo.height();

        // 比例系数
        m_real_per_pix = m_target->real_per_pix;
        m_real_per_gray_level = m_target->real_per_gray_level;

        QSurfaceDataArray* data_array = new QSurfaceDataArray;
        // 预分配QSurfaceDataArray中的元素数量
        data_array->reserve(m_img_height);

        // 通过遍历灰度图像素点读取高度数据 可以修改高度值
        // xoz坐标作为底面，y轴表示高度
        for(int i = 0; i < m_img_height; i++){
            float z = float(i) * m_real_per_pix;
            QSurfaceDataRow* new_row = new QSurfaceDataRow(m_img_width);
            for(int j = 0; j < m_img_width; j++){
                float x = float(j) * m_real_per_pix;
                uchar* uchar_ptr = m_topo.scanLine(i);
                ushort* ushort_ptr = reinterpret_cast<ushort*>(uchar_ptr);
                float color = *(ushort_ptr + j);
                float y = color * m_real_per_gray_level;
                (*new_row)[j].setPosition(QVector3D(x, y, z));
            }
            *data_array << new_row;
        }
        m_proxy->resetArray(data_array);

        // 计算x y z轴范围
        m_img_x = (float)m_img_width * m_real_per_pix;
        m_img_z = (float)m_img_height * m_real_per_pix;
        m_img_y = 65536.0f * m_real_per_gray_level;
    }
}

/**
 * @brief SurfaceArea::initCylinder
 * 初始化圆柱面参数，创建相关变量并且设置m_cylinder_series的颜色、标签格式、绘制模式和阴影模式
 */
void SurfaceArea::initCylinder()
{
    // 圆柱面参数
    m_cylinder_radius = 1.0f;
    m_cylinder_height = 1.0f;
    m_cylinder_points_num = 360;

    // 创建一个QSurfaceDataProxy对象
    m_cyliner_proxy = new QSurfaceDataProxy;
    // 创建一个QSurface3DSeries对象，并将这个QSurfaceDataProxy对象添加到它中
    m_cylinder_series = new QSurface3DSeries(m_cyliner_proxy);
    m_cylinder_series->setItemLabelFormat(QStringLiteral("@xLabel, @zLabel, @yLabel"));
    m_cylinder_series->setDrawMode(QSurface3DSeries::DrawSurface);
    m_cylinder_series->setFlatShadingEnabled(false);
    m_cylinder_series->setBaseColor(QColor(255, 0, 0));
}

/**
 * @brief SurfaceArea::initPlane
 * 初始化平面参数，创建相关变量并且设置m_plane_series的颜色、标签格式、绘制模式和阴影模式
 */
void SurfaceArea::initPlane()
{
    // 平面参数
    m_plane_length = 1.0f;
    m_plane_height = 1.0f;
    m_plane_num = 200;

    // 创建一个QSurfaceDataProxy对象.
    m_plane_proxy = new QSurfaceDataProxy;
    // 创建一个QSurface3DSeries对象，并将这个QSurfaceDataProxy对象添加到它中
    m_plane_series = new QSurface3DSeries(m_plane_proxy);
    m_plane_series->setItemLabelFormat(QStringLiteral("@xLabel, @zLabel, @yLabel"));
    m_plane_series->setDrawMode(QSurface3DSeries::DrawSurface);
    m_plane_series->setFlatShadingEnabled(false);
    m_plane_series->setBaseColor(QColor(255, 0, 0));
}

/**
 * @brief SurfaceArea::renderTopo
 * 绘制地形图
 * 当m_target->topo不为空时，添加m_series并更新坐标轴
 * 反之则将m_series移除不绘制
 */
void SurfaceArea::renderTopo()
{
    if(!m_target->topo.isNull()){
        m_graph->addSeries(m_series);
        updateScale();
        update();
    }
    else{
        m_graph->removeSeries(m_series);
        update();
    }
}

/**
 * @brief SurfaceArea::renderCylinder
 * 绘制圆柱面
 * 根据输入的圆心pos0_和半径R_(单位：像素)绘制圆柱面
 */
void SurfaceArea::renderCylinder(QPointF pos0_, qreal R_)
{
    m_graph->addSeries(m_cylinder_series);
    m_graph->removeSeries(m_plane_series);

    // 将pos0_和R_单位转换为实际坐标系um
    pos0_ = m_real_per_pix * pos0_;
    R_ = R_ * m_real_per_pix;

    // 计算三维坐标变换矩阵m_cylinder_matrix，对圆柱面点进行坐标变换
    float scale = R_;
    QVector3D translate = QVector3D(pos0_.x(), 0, pos0_.y());
    QMatrix4x4 m_cylinder_matrix;
    m_cylinder_matrix.setToIdentity();
    m_cylinder_matrix.translate(translate);
    m_cylinder_matrix.scale(scale, m_img_y, scale);

    QSurfaceDataArray* data_array = new QSurfaceDataArray;
    // 预分配QSurfaceDataArray中的元素数量
    data_array->reserve(m_cylinder_points_num);
    for(int i = 0; i < m_cylinder_points_num; i++){
        // xoz坐标作为底面，y轴表示高度
        float angle = float(i) / float(m_cylinder_points_num - 1) * PI * 2.0f;
        float x = m_cylinder_radius * cos(angle);
        float z = m_cylinder_radius * sin(angle);
        QSurfaceDataRow* new_row = new QSurfaceDataRow(2);
        // 进行矩阵变换
        QVector3D point1 = m_cylinder_matrix * QVector3D(x, 0, z);
        QVector3D point2 = m_cylinder_matrix * QVector3D(x, m_cylinder_height, z);
        (*new_row)[0].setPosition(point1);
        (*new_row)[1].setPosition(point2);
        *data_array << new_row;
    }
    m_cyliner_proxy->resetArray(data_array);

    // 设置圆柱面xoz坐标范围
    m_cylinder_range_x.clear();
    m_cylinder_range_x.append(pos0_.x() - R_);
    m_cylinder_range_x.append(pos0_.x() + R_);
    m_cylinder_range_z.clear();
    m_cylinder_range_z.append(pos0_.y() - R_);
    m_cylinder_range_z.append(pos0_.y() + R_);

    // 更新坐标轴刻度
    m_scale_render_type = CylinderType;
    updateScale();
    update();
}

/**
 * @brief SurfaceArea::renderPlane
 * 绘制平面
 * 根据输入的pos1_和pos2_(单位：像素)绘制平面
 */
void SurfaceArea::renderPlane(QPointF pos1_, QPointF pos2_)
{
    m_graph->addSeries(m_plane_series);
    m_graph->removeSeries(m_cylinder_series);

    // 将pos1_和pos2_单位转换为实际坐标系um
    pos1_ = m_real_per_pix * pos1_;
    pos2_ = m_real_per_pix * pos2_;

    // 计算三维坐标变换矩阵m_plane_matrix，对圆柱面点进行坐标变换
    QVector2D vec = QVector2D(pos2_ - pos1_);
    qreal scale = vec.length();
    float theta;
    if(vec.x() >= 0 && vec.y() >= 0)
        theta = atan(vec.y() / vec.x());
    else if(vec.x() < 0 && vec.y() >= 0)
        theta = atan(vec.y() / vec.x()) + PI;
    else if(vec.x() < 0 && vec.y() < 0)
        theta = atan(vec.y() / vec.x()) + PI;
    else
        theta = atan(vec.y() / vec.x());
    float rotateAngle = theta * 180 / PI;
    QVector3D translate = QVector3D(pos1_.x(), 0, pos1_.y());
    QMatrix4x4 m_plane_matrix;
    m_plane_matrix.setToIdentity();
    m_plane_matrix.translate(translate);
    m_plane_matrix.rotate(-rotateAngle, QVector3D(0.0f, 1.0f, 0.0f));
    m_plane_matrix.scale(scale, m_img_y, scale);

    QSurfaceDataArray* data_array = new QSurfaceDataArray;
    // 预分配QSurfaceDataArray中的元素数量
    data_array->reserve(m_plane_num);
    for(int i = 0; i <= m_plane_num; i++){
        // xoz坐标作为底面，y轴表示高度
        float x = i * m_plane_length / m_plane_num;
        float z = 0;
        QSurfaceDataRow* new_row = new QSurfaceDataRow(2);
        // 进行矩阵变换
        QVector3D point1 = m_plane_matrix * QVector3D(x, 0, z);
        QVector3D point2 = m_plane_matrix * QVector3D(x, m_plane_height, z);
        (*new_row)[0].setPosition(point1);
        (*new_row)[1].setPosition(point2);
        *data_array << new_row;
    }
    m_plane_proxy->resetArray(data_array);

    // 设置平面xoz坐标范围
    m_plane_range_x.clear();
    m_plane_range_x.append(fmin(pos1_.x(), pos2_.x()));
    m_plane_range_x.append(fmax(pos1_.x(), pos2_.x()));
    m_plane_range_z.clear();
    m_plane_range_z.append(fmin(pos1_.y(), pos2_.y()));
    m_plane_range_z.append(fmax(pos1_.y(), pos2_.y()));

    // 更新坐标轴刻度
    m_scale_render_type = PlaneType;
    updateScale();
    update();
}

///**
// * @brief SurfaceArea::updateScale
// * 更新坐标轴刻度
// * 根据本次绘制的类型设置刻度最大值和最小值
// */
//void SurfaceArea::updateScale()
//{
//    switch (m_scale_render_type){
//    case TopoType:{
//        // 计算x y z坐标范围
//        m_max_x = calMaxScaleXZ(m_img_x);
//        m_max_y = calMaxScaleY(m_img_y);
//        m_max_z = calMaxScaleXZ(m_img_z);
//        m_min_x = calMinScaleXZ(-0.01 * m_img_x);
//        m_min_y = 0;
//        m_min_z = calMinScaleXZ(-0.01 * m_max_z);

//        // 设置x y z坐标范围
//        m_graph->axisX()->setRange(m_min_x, m_max_x);
//        m_graph->axisY()->setRange(m_min_y, m_max_y);
//        m_graph->axisZ()->setRange(m_min_z, m_max_z);
//        break;
//    }
//    case PlaneType:{
//        // 计算x y z坐标范围
//        m_min_x = calMinScaleXZ(fmin(-0.1 * m_img_x, m_plane_range_x[0]));
//        m_max_x = calMaxScaleXZ(fmax(m_img_x, m_plane_range_x[1]));
//        m_min_y = 0;
//        m_max_y = calMaxScaleY(m_img_y);
//        m_min_z = calMinScaleXZ(fmin(-0.1 * m_max_z, m_plane_range_z[0]));
//        m_max_z = calMaxScaleXZ(fmax(m_img_z, m_plane_range_z[1]));

//        // 设置x y z坐标范围
//        m_graph->axisX()->setRange(m_min_x, m_max_x);
//        m_graph->axisY()->setRange(m_min_y, m_max_y);
//        m_graph->axisZ()->setRange(m_min_z, m_max_z);
//        break;
//    }
//    case CylinderType:{
//        m_min_x = calMinScaleXZ(fmin(-0.1 * m_max_x, m_cylinder_range_x[0]));
//        m_max_x = calMaxScaleXZ(fmax(m_img_x, m_cylinder_range_x[1]));
//        m_min_y = 0;
//        m_max_y = calMaxScaleY(m_img_y);
//        m_min_z = calMinScaleXZ(fmin(-0.1 * m_max_z, m_cylinder_range_z[0]));
//        m_max_z = calMaxScaleXZ(fmax(m_img_z, m_cylinder_range_z[1]));

//        // 设置x y z坐标范围
//        m_graph->axisX()->setRange(m_min_x, m_max_x);
//        m_graph->axisY()->setRange(m_min_y, m_max_y);
//        m_graph->axisZ()->setRange(m_min_z, m_max_z);
//        break;
//    }
//    default:
//        break;
//    }
//}

/**
 * @brief SurfaceArea::updateScale
 * 更新坐标轴刻度
 * 根据本次绘制的类型设置刻度最大值和最小值
 * 对于x和z坐标是计算他们最大和最小值在预定梯度值下的整数值
 * 对于y坐标则直接设置最小值为0，最大值为图像中的最大高度值
 * 本段代码中仍旧保留了计算y轴最大值对应整数值的语句
 */
void SurfaceArea::updateScale()
{
    switch (m_scale_render_type){
    case TopoType:{
        // 计算x y z坐标范围
        m_max_x = calMaxScaleXZ(m_img_x);
        m_max_y = calMaxScaleY(m_img_y);
        m_max_z = calMaxScaleXZ(m_img_z);
        m_min_x = calMinScaleXZ(-0.01 * m_img_x);
        m_min_y = 0;
        m_min_z = calMinScaleXZ(-0.01 * m_max_z);

        // 设置x y z坐标范围
        m_graph->axisX()->setRange(m_min_x, m_max_x);
        m_graph->axisY()->setRange(m_min_y, m_img_y);
        m_graph->axisZ()->setRange(m_min_z, m_max_z);
        break;
    }
    case PlaneType:{
        // 计算x y z坐标范围
        m_min_x = calMinScaleXZ(fmin(-0.1 * m_img_x, m_plane_range_x[0]));
        m_max_x = calMaxScaleXZ(fmax(m_img_x, m_plane_range_x[1]));
        m_min_y = 0;
        m_max_y = calMaxScaleY(m_img_y);
        m_min_z = calMinScaleXZ(fmin(-0.1 * m_max_z, m_plane_range_z[0]));
        m_max_z = calMaxScaleXZ(fmax(m_img_z, m_plane_range_z[1]));

        // 设置x y z坐标范围
        m_graph->axisX()->setRange(m_min_x, m_max_x);
        m_graph->axisY()->setRange(m_min_y, m_img_y);
        m_graph->axisZ()->setRange(m_min_z, m_max_z);
        break;
    }
    case CylinderType:{
        m_min_x = calMinScaleXZ(fmin(-0.1 * m_max_x, m_cylinder_range_x[0]));
        m_max_x = calMaxScaleXZ(fmax(m_img_x, m_cylinder_range_x[1]));
        m_min_y = 0;
        m_max_y = calMaxScaleY(m_img_y);
        m_min_z = calMinScaleXZ(fmin(-0.1 * m_max_z, m_cylinder_range_z[0]));
        m_max_z = calMaxScaleXZ(fmax(m_img_z, m_cylinder_range_z[1]));

        // 设置x y z坐标范围
        m_graph->axisX()->setRange(m_min_x, m_max_x);
        m_graph->axisY()->setRange(m_min_y, m_img_y);
        m_graph->axisZ()->setRange(m_min_z, m_max_z);
        break;
    }
    default:
        break;
    }
}


/**
 * @brief SurfaceArea::calMaxScaleXZ
 * 计算x z轴的最大刻度值
 * 根据输入的浮点数值num_和设置的步进值，计算比num_大且最接近的整数值
 */
float SurfaceArea::calMaxScaleXZ(float num_)
{
    float max_scale;
    if(num_ < 50){
        int n = num_ / 2;
        max_scale = (n + 1) * 2;
    }
    else if((num_ >= 50) && (num_ < 100)){
        int n = num_ / 10;
        max_scale = (n + 1) * 10;
    }
    else if(num_ >= 100){
        int n = num_ / 100;
        max_scale = (n + 1) * 100;
    }
    else
        max_scale = num_;
    return max_scale;
}

/**
 * @brief SurfaceArea::calMaxScaleY
 * 计算y轴的最大刻度值
 * 根据输入的浮点数值num_和设置的步进值，计算比num_大且最接近的整数值
 */
float SurfaceArea::calMaxScaleY(float num_)
{
    float max_scale;
    if(num_ <= 10){
        max_scale = 10;
    }
    else if((num_ > 10) && (num_ < 100)){
        int n = num_ / 50;
        max_scale = (n + 1) * 50;
    }
    else if(num_ >= 100){
        int n = num_ / 200;
        max_scale = (n + 1) * 200;
    }
    else
        max_scale = num_;
    return max_scale;
}

/**
 * @brief SurfaceArea::calMinScaleXZ
 * 计算x z轴的最小刻度值
 * 根据输入的浮点数值num_和设置的步进值，计算比num_小且最接近的整数值
 */
float SurfaceArea::calMinScaleXZ(float num_)
{
    float min_scale;
    if(num_ > -50){
        int n = num_ / 2;
        min_scale = (n - 1) * 2;
    }
    else if((num_ > -100) && (num_ <= -50)){
        int n = num_ /10;
        min_scale = (n - 1) * 10;
    }
    else if(num_ <= -100){
        int n = num_ / 100;
        min_scale = (n - 1) * 100;
    }
    else
        min_scale = num_;
    return min_scale;
}

/**
 * @brief SurfaceArea::setTarget
 * 设置target指针
 */
void SurfaceArea::setTarget(const Target &target_)
{
    delete m_target;
    m_target = new Target(target_);
    updateTopo();
    renderTopo();
    if(m_target->topo.isNull())
        clearElement();
    else if(m_element != nullptr)
        setElement(m_element);
    update();
}

/**
 * @brief SurfaceArea::setElement
 * 当elemet改变时，根据指针返回元素类型绘制轮廓曲面
 */
void SurfaceArea::setElement(const Element *element_)
{
    int type = element_->getType();
    QVector<QPointF> points = element_->getPoints();
    QVector<QPointF> featurepoints = element_->getFeaturePoints();
    int point_num = points.count();

    switch(type){
    // 当element类型为线段时
    case ElementType::Type_AG_ArrowedLineSegment:
        if(point_num == 2){
            m_element = element_;
            QPointF pos1 = points[0];
            QPointF pos2 = points[1];
            renderPlane(pos1, pos2);
        }
        break;
    // 当element类型为圆时
    case ElementType::Type_AG_ClockwiseCircle:
        if(point_num == 3){
            m_element = element_;
            QPointF pos0 = featurepoints[0];
            QVector2D r = QVector2D(points[0] - pos0);
            qreal R = r.length();
            renderCylinder(pos0, R);
        }
        break;
    // 当element类型为直线时
    case ElementType::Type_AG_ArrowedLine:
        if(point_num == 2){
            m_element = element_;
            QPointF pos1 = featurepoints[2];
            QPointF pos2 = featurepoints[3];
            renderPlane(pos1, pos2);
        }
        break;
    // 当element类型为水平线时
    case ElementType::Type_AG_ArrowedHorizontalLine:{
        m_element = element_;
        QPointF pos1 = featurepoints[0];
        QPointF pos2 = featurepoints[1];
        renderPlane(pos1, pos2);
        break;
    }
    // 当element类型为垂直线时
    case ElementType::Type_AG_ArrowedVerticalLine:{
        m_element = element_;
        QPointF pos1 = featurepoints[0];
        QPointF pos2 = featurepoints[1];
        renderPlane(pos1, pos2);
        break;
    }
    default:
        break;
    }
}

/**
 * @brief SurfaceArea::clearElement
 * 删除元素
 */
void SurfaceArea::clearElement()
{
    m_element = nullptr;
    m_graph->removeSeries(m_plane_series);
    m_graph->removeSeries(m_cylinder_series);
    m_scale_render_type = TopoType;
    if((m_target != nullptr)){
        if(!m_target->topo.isNull()){
            updateScale();
        }
    }
}


