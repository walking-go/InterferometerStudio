#include "curve_chart.h"
#include "mg_element.h"

/**
 * @brief myQChartView::myQChartView
 * 构造函数
 */
CurveChart::CurveChart(QWidget  *parent)
    : QChartView{parent}
{
    m_chart = new QChart;
    m_series = new QLineSeries;
    m_axis_x = new QValueAxis;
    m_axis_y = new QValueAxis;
    m_chart->setTitle("Measurement line display");
    initAxis("长度(um)", 0, 100, 5, "高度(um)", 0, 100, 5);
    buildChart();
    setChart(m_chart);
    // 内置边界
    m_chart->layout()->setContentsMargins(0, 0, 0, 0);
    m_chart->setBackgroundRoundness(0);
    // 尺寸策略
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setRenderHint(QPainter::Antialiasing);
}

/**
 * @brief myQChartView::~myQChartView
 * 析构函数
 */
CurveChart::~CurveChart()
{

}

/**
 * @brief myQChartView::initAxis
 * 初始化图像坐标轴
 */
void CurveChart::initAxis(QString name_x_, qreal min_x_, qreal max_x_, int tick_x_, QString name_y_, qreal min_y_, qreal max_y_, int tick_y_)
{
    // 设置坐标轴初始名称，范围，刻度个数
    m_name_x = name_x_;
    m_min_x = min_x_;
    m_max_x = max_x_;
    m_tick_x = tick_x_;

    m_name_y = name_y_;
    m_min_y = min_y_;
    m_max_y = max_y_;
    m_tick_y = tick_y_;

    m_axis_x->setRange(m_min_x, m_max_x);               // 设置范围
    m_axis_x->setLabelFormat("%.2f");                   // 设置刻度的格式
    m_axis_x->setGridLineVisible(true);                 // 网格线可见
    m_axis_x->setTickCount(m_tick_x);                   // 设置多少个大格
    m_axis_x->setMinorTickCount(1);                     // 设置每个大格里面小刻度线的数目
    m_axis_x->setTitleText(m_name_x);                   // 设置坐标轴名称

    m_axis_y->setRange(m_min_y, m_max_y);
    m_axis_y->setLabelFormat("%.2f");
    m_axis_y->setGridLineVisible(true);
    m_axis_y->setTickCount(m_tick_y);
    m_axis_y->setMinorTickCount(1);
    m_axis_y->setTitleText(m_name_y);

    m_chart->addAxis(m_axis_x, Qt::AlignBottom);        // 下：Qt::AlignBottom  上：Qt::AlignTop
    m_chart->addAxis(m_axis_y, Qt::AlignLeft);          // 左：Qt::AlignLeft    右：Qt::AlignRight
}

/**
 * @brief myQChartView::resetAxis
 * 重设坐标轴，当element改变时用到
 */
void CurveChart::resetAxis()
{
    // 调整x轴刻度 根据最大长度计算x轴最小刻度
    if(m_max_length > 100){
        int length = abs(m_max_length);
        int n = 0;
        while(length >= 10){
            length = length / 10;
            n++;
        }
        int delta = pow(10, n);
        m_min_scale_x = delta / 2;
    }
    else if(m_max_length > 1 && m_max_length <= 100){
        int length = abs(m_max_length);
        int n = floor(length / 5);
        if(n >= 2)
            m_min_scale_x = floor((float)(n) / 2.0f) * 5;
        else
            m_min_scale_x = 2;
    }
    else if(m_max_length <= 1){
        m_min_scale_x = 1;
    }

    // 调整y轴刻度 根据最大高度计算y轴最小刻度
    if(m_max_height > 100){
        int height = abs(m_max_height);
        int n = 0;
        while(height >= 10){
            height = height / 10;
            n++;
        }
        int delta = pow(10, n);
        m_min_scale_y = delta / 2;
    }
    else if(m_max_height > 1 && m_max_height <= 100){
        int height = abs(m_max_height);
        int n = floor(height / 10);
        if(n >= 2)
            m_min_scale_y = floor((float)(n) / 2.0f) * 10;
        else
            m_min_scale_y = 10;
    }
    else if(m_max_height <= 1){
        m_min_scale_y = 2;
    }

    // 设置X轴坐标刻度值
    int n = ceil(m_max_length / m_min_scale_x);
    qreal axis_max_x = n * m_min_scale_x;
    m_axis_x->setMax(axis_max_x);
    m_axis_x->setMin(0);
    m_axis_x->setTickCount(n + 1);

    // 设置Y轴坐标刻度值
    //    int m = ceil(m_max_height / m_min_scale_y);
    //    qreal axis_max_y = (m + 1) * m_min_scale_y;
    //    m_axis_y->setMax(axis_max_y);
    //    m_axis_y->setMin(0);
    //    m_axis_y->setTickCount(m + 2);
    qreal axis_max_y = m_max_height;
    m_axis_y->setMax(axis_max_y);
    m_axis_y->setMin(0);
    m_axis_y->setTickCount(5);
}

/**
 * @brief myQChartView::resetAxis
 * 绘制Chart图像
 */
void CurveChart::buildChart()
{
    m_series->setPen(QPen(Qt::black, 2, Qt::SolidLine));        // 设置线条格式
    m_chart->setAnimationOptions(QChart::NoAnimation);          // 设置动画模式
    m_chart->legend()->hide();                                  // 隐藏图例
    m_chart->addSeries(m_series);                               // 输入数据
    m_series->attachAxis(m_axis_x);
    m_series->attachAxis(m_axis_y);
}

/**
 * @brief myQChartView::loadheigntData
 * 加载数据点的高度数据
 */
void CurveChart::loadHeigntData()
{
    // 先清除原来数据，再加载新数据
    m_series->clear();
    m_series->replace(m_height_data);
}

/**
 * @brief myQChartView::findcircleData
 * 获取圆周轮廓线上点的(x,y)坐标
 */
void CurveChart::findCircleData(QPointF pos0_, qreal r_)
{
    m_circle_num = 0;
    qreal temp_x = 0;
    qreal temp_y = 0;
    qreal ratio = qMin(calRealCircumference(r_) / m_resolution, m_max_sampling_pts);
    qDebug() << "circle sample pts =" << ratio;

    qreal theta = 2 * PI / ratio;

    // 如果指针不为空指针 则在分配内存前释放内存
    if(m_circle_data2D != nullptr)
        delete[] m_circle_data2D;

    m_circle_data2D = new QPointF[(int)floor(ratio)];
    for(int i = 0; i < (int)floor(ratio); i++)
    {
        temp_x = pos0_.x() + r_*cos(i*theta);
        temp_y = pos0_.y() + r_*sin(i*theta);
        m_circle_data2D[m_circle_num++] = QPointF(temp_x, temp_y);
    }
}

/**
 * @brief myQChartView::findstraightData
 * 获取直线轮廓线上点的(x,y)坐标
 */
void CurveChart::findStraightData(QPointF pos1_, QPointF pos2_)
{
    m_straight_num = 0;
    qreal temp_x = 0;
    qreal temp_y = 0;
    qreal ratio = qMin(calRealStrightDistance(pos1_, pos2_) / m_resolution, m_max_sampling_pts);
    QVector2D vector = QVector2D(pos2_ - pos1_) / ratio;
    qDebug() << "straight sample pts =" << ratio;

    // 如果指针不为空指针 则在分配内存前释放内存
    if(m_straight_data2D != nullptr)
        delete[] m_straight_data2D;

    m_straight_data2D = new QPointF[(int)floor(ratio) + 1];
    for(int i = 0; i < (int)floor(ratio) + 1; i++)
    {
        temp_x = pos1_.x() + i*vector.x();
        temp_y = pos1_.y() + i*vector.y();
        m_straight_data2D[m_straight_num++] = QPointF(temp_x, temp_y);
    }
}

/**
 * @brief myQChartView::findcircleHeight
 * 获取圆周轮廓线上点的高度z坐标，并转换成实际高度坐标
 */
void CurveChart::findCircleHeight()
{
    m_height_data.clear();
    m_height = 0;
    m_max_height = 0;
    m_max_length = 0;
    for(int i = 0; i < m_circle_num; i++)
    {
        // 获取每个像素点的坐标(x,y)
        qreal x = m_circle_data2D[i].x();
        qreal y = m_circle_data2D[i].y();

        // 指针指向每个像素点, 然后取像素点的值
        if(x < -0.5 || x > (m_target->topo.width() - 0.5) || y < -0.5 || y > (m_target->topo.height() - 0.5))
        {
            // 超出图像范围不读取
            m_height = 0;
        }
        else
        {
            // 计算浮点数像素坐标周围四个整数点像素坐标值 并进行二次样条差值计算出该点高度
            QVector<double> round_point_height = findRoundPointHeight(x, y);
            m_height = cubicSpline(x, y, round_point_height[0], round_point_height[1], round_point_height[2], round_point_height[3]);
        }
        // 更新高度最大值
        if(m_height > m_max_height)
            m_max_height = m_height;
        // 获取实际坐标下每个点距离起始点的距离  高度数据
        m_height_data.append(QPointF(i * m_pix2real_ratio, m_height));
    }
    // 计算轮廓线长度
    m_max_length = m_circle_num * m_pix2real_ratio;
}

/**
 * @brief myQChartView::findstraightHeight
 * 获取直线轮廓线上点的高度z坐标，并转换成实际高度坐标
 */
void CurveChart::findStraightHeight()
{
    m_height_data.clear();
    m_height = 0;
    m_max_height = 0;
    m_max_length = 0;
    for(int i = 0; i < m_straight_num; i++)
    {
        // 获取每个像素点的坐标(x,y)
        qreal x = m_straight_data2D[i].x();
        qreal y = m_straight_data2D[i].y();
        // 指针指向每个像素点, 然后取像素点的值
        if(x < -0.5 || x > (m_target->topo.width() - 0.5) || y < -0.5 || y > (m_target->topo.height() - 0.5))
        {
            // 超出图像范围不读取
            m_height = 0;
        }
        else
        {
            // 计算浮点数像素坐标周围四个整数点像素坐标值 并进行二次样条差值计算出该点高度
            QVector<double> round_point_height = findRoundPointHeight(x, y);
            m_height = cubicSpline(x, y, round_point_height[0], round_point_height[1], round_point_height[2], round_point_height[3]);
        }
        // 更新高度最大值
        if(m_height > m_max_height)
            m_max_height = m_height;
        // 获取实际坐标下每个点距离起始点的距离  高度数据
        m_height_data.append(QPointF(i * m_pix2real_ratio, m_height));
    }
    // 计算轮廓线长度
    m_max_length = m_straight_num * m_pix2real_ratio;
}

/**
 * @brief myQChartView::pix2realDistance
 * 重载函数，将直线上两点间的像素坐标间距转换为实际坐标间距
 */
qreal CurveChart::calRealStrightDistance(QPointF pos1_, QPointF pos2_)
{
    QVector2D vec = QVector2D(pos1_ - pos2_);
    qreal distance = m_pix2real_ratio * vec.length();
    return distance;
}

/**
 * @brief myQChartView::pix2realDistance
 * 重载函数，将圆周上的像素坐标长度转换为实际坐标长度
 */
qreal CurveChart::calRealCircumference(qreal r_)
{
    qreal distance = m_pix2real_ratio * 2 * PI * r_;
    return distance;
}

/**
 * @brief CurveChart::findRoundPointHeight
 * 寻找浮点坐标(xf_,yf_)临近的四个整数坐标
 */
QVector<qreal> CurveChart::findRoundPointHeight(qreal xf_, qreal yf_)
{
    // 将采样点坐标向下取整
    int x0 = qFloor(xf_);
    int y0 = qFloor(yf_);

    /**
     * 初始化z1,z2,z3,z4四个邻近点坐标
     * 在二次样条差值采样时，选取的邻近四个点坐标有如下关系
     *      z1,z2       ->      (x1,y1) (x2,y1)
     *      z3,z4       ->      (x1,y2) (x2,y2)
     */
    int x1, x2;
    int y1, y2;

    QVector<QPoint> round_point;
    // 对x坐标进行边缘处理
    // 当采样点位于左边缘时
    if(x0 == -1){
        x1 = x0 + 1;
        x2 = x1 + 1;
    }
    // 当采样点位于右边缘时
    else if(x0 == (m_topo.width() - 1)){
        x1 = x0;
        x2 = x1;
    }
    else{
        x1 = x0;
        x2 = x1 + 1;
    }
    // 对y坐标进行边缘处理
    // 当采样点位于上边缘时
    if(y0 == -1){
        y1 = y0 + 1;
        y2 = y1 + 1;
    }
    // 当采样点位于下边缘时
    else if(y0 == (m_topo.height() - 1)){
        y1 = y0;
        y2 = y1;
    }
    else{
        y1 = y0;
        y2 = y1 + 1;
    }

    // 保存邻近点坐标
    round_point.append(QPoint(x1, y1));
    round_point.append(QPoint(x2, y1));
    round_point.append(QPoint(x1, y2));
    round_point.append(QPoint(x2, y2));


    // 读取邻近点高度值
    QVector<double> round_point_height;
    for(int i = 0; i < 4; i++){
        int xi = round_point[i].x();
        int yi = round_point[i].y();
        uchar* uchar_ptr = m_topo.scanLine((int)yi);
        ushort* ushort_ptr = reinterpret_cast<ushort*>(uchar_ptr);
        float color = *(ushort_ptr + xi);
        qreal height = color * m_target->real_per_gray_level;
        round_point_height.append(height);
    }

    // 返回邻近点高度值
    return round_point_height;
}

/**
 * @brief CurveChart::quadraticSpline
 * 二次样条插值，用于计算浮点数坐标下的高度值
 */
qreal CurveChart::quadraticSpline(qreal xf_, qreal yf_, qreal round_height_1_, qreal round_height_2_, qreal round_height_3_, qreal round_height_4_)
{
    // 计算x0和y0
    int x0 = (int)floor(xf_);
    int y0 = (int)floor(yf_);

    // 计算四个点的z值
    double z11 = round_height_1_;
    double z21 = round_height_2_;
    double z12 = round_height_3_;
    double z22 = round_height_4_;

    // 计算二次样条插值
    double h1 = xf_ - x0;
    double h2 = yf_ - y0;
    double a1 = z11;
    double a2 = z21 - z11;
    double a3 = z12 - z11;
    double a4 = z11 - z21 - z12 + z22;
    double z = a1 + a2 * h1 + a3 * h2 + a4 * h1 * h2;

    // 返回差值结果
    return z;
}
// 三次样条插值：计算两点之间的平滑过渡
qreal CurveChart::cubicSpline(qreal xf_, qreal yf_, qreal round_height_1_, qreal round_height_2_, qreal round_height_3_, qreal round_height_4_)
{
    int x0 = qFloor(xf_);
    int y0 = qFloor(yf_);
    double h1 = xf_ - x0; // 0~1之间的小数
    double h2 = yf_ - y0;

    // 三次样条系数计算（基于周围4点的高度）
    double z00 = round_height_1_; // (x0,y0)
    double z10 = round_height_2_; // (x0+1,y0)
    double z01 = round_height_3_; // (x0,y0+1)
    double z11 = round_height_4_; // (x0+1,y0+1)

    // 双三次插值公式（更复杂但更平滑）
    double xCoeff[4] = {
        2*h1*h1*h1 - 3*h1*h1 + 1,
        h1*h1*h1 - 2*h1*h1 + h1,
        -2*h1*h1*h1 + 3*h1*h1,
        h1*h1*h1 - h1*h1
    };
    double yCoeff[4] = {
        2*h2*h2*h2 - 3*h2*h2 + 1,
        h2*h2*h2 - 2*h2*h2 + h2,
        -2*h2*h2*h2 + 3*h2*h2,
        h2*h2*h2 - h2*h2
    };

    double z = 0;
    z += z00 * xCoeff[0] * yCoeff[0];
    z += z10 * xCoeff[2] * yCoeff[0];
    z += z01 * xCoeff[0] * yCoeff[2];
    z += z11 * xCoeff[2] * yCoeff[2];
    return z;
}
/**
 * @brief myQChartView::setTarget
 * 当target改变时，设置m_target指针
 */
void CurveChart::setTarget(const Target &target_)
{
    // 当target成功构建时，初始化输入数据图像
    if(!target_.topo.isNull()){
        delete m_target;
        m_target = new Target(target_);
        m_pix2real_ratio = m_target->real_per_pix;
        m_resolution = qMin(m_target->x_scope, m_target->y_scope) / m_resolution_facotr;
        m_resolution = m_pix2real_ratio;  // 让采样步距 = 1 像素

        // 读图
        m_topo = target_.topo;
    }
    // 反之则清空target和图形
    else{
        delete m_target;
        m_target = nullptr;
    }
    qDebug() << "resolution =" << m_resolution;

    // 刷新曲线
    refresh();
}

/**
 * @brief myQChartView::setElement
 * 当elemet改变时，根据指针返回元素类型绘制轮廓曲线
 */
void CurveChart::setElement(const Element *element_)
{
    // 新元素点数不够时（无法绘制），保留旧元素
    // 新元素为Type_NoElement时，保留旧元素
    if(element_->getLength() != element_->getN()
        || element_->getType() == Type_NoElement)
        return;
    // 更新元素
    m_element = element_;
    refresh();
}

/**
 * @brief myQChartView::clearElement
 * 删除元素
 */
void CurveChart::clearElement()
{
    if(m_element != nullptr)
        m_element = nullptr;
    refresh();
}

/**
 * @brief myQChartView::refresh
 *
 * 根据当前m_element和m_target的状态更新画面
 * 不对m_element或m_target进行任何修改
 * 如果m_element或m_target任意一方为空，则清空曲线
 */
void CurveChart::refresh()
{
    // 判断当前状态
    // 如果不满足绘制条件,则清空曲线
    // -------------------------
    if(m_element == nullptr || m_target == nullptr){
        m_series->clear();
        m_axis_x->setRange(0, 100);
        m_axis_x->setTickCount(5);
        m_axis_y->setRange(0, 100);
        m_axis_y->setTickCount(5);
        return;
    }
    // 满足绘制条件,刷新曲线
    // ------------------
    int type = m_element->getType();
    QVector<QPointF> points = m_element->getPoints();
    QVector<QPointF> featurepoints = m_element->getFeaturePoints();
    int point_num = points.count();

    switch(type){
    // 当element类型为线段时
    case ElementType::Type_AG_ArrowedLineSegment:
        if(point_num == 2){
            QPointF pos1 = points[0];
            QPointF pos2 = points[1];
            findStraightData(pos1, pos2);
            findStraightHeight();
            loadHeigntData();
            resetAxis();
        }
        break;
    // 当element类型为圆时
    case ElementType::Type_AG_ClockwiseCircle:
        if(point_num == 3){
            QPointF pos0 = featurepoints[0];
            QVector2D r = QVector2D(points[0] - pos0);
            qreal R = r.length();
            findCircleData(pos0, R);
            findCircleHeight();
            loadHeigntData();
            resetAxis();
        }
        break;
    // 当element类型为直线时
    case ElementType::Type_AG_ArrowedLine:
        if(point_num == 2){
            QPointF pos1 = featurepoints[2];
            QPointF pos2 = featurepoints[3];
            findStraightData(pos1, pos2);
            findStraightHeight();
            loadHeigntData();
            resetAxis();
        }
        break;
    // 当element类型为水平线时
    case ElementType::Type_AG_ArrowedHorizontalLine:{
        QPointF pos1 = featurepoints[0];
        QPointF pos2 = featurepoints[1];
        findStraightData(pos1, pos2);
        findStraightHeight();
        loadHeigntData();
        resetAxis();
        break;
    }
    // 当element类型为垂直线时
    case ElementType::Type_AG_ArrowedVerticalLine:{
        QPointF pos1 = featurepoints[0];
        QPointF pos2 = featurepoints[1];
        findStraightData(pos1, pos2);
        findStraightHeight();
        loadHeigntData();
        resetAxis();
        break;
    }
    default:
        break;
    }
}
