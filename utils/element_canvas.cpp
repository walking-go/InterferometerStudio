#include "element_canvas.h"
#include "QDebug"
#include "drawer.h"
#include "element.h"

/**
 * @brief ElementCanvas::ElementCanvas
 * 构造函数
 */
ElementCanvas::ElementCanvas(QWidget *parent)
    : ImgViewer{parent}
{
    // 成员变量初始化
    m_canvas_painter = new QPainter();
    m_MG_element_drawer = new Drawer(this, &m_MG_elements);
    m_AG_element_drawer = new Drawer(this, &m_AG_elements);
    m_current_drawer = m_MG_element_drawer;
    m_MG_elements = QVector<Element*>();
    m_AG_elements = QVector<Element*>();
    m_current_element_type = Type_NoElement;
    m_measurement_results = "";
    m_target = nullptr;
    // 信号槽初始化
    connect(m_AG_element_drawer, &Drawer::currentElementUpdated, this, &ElementCanvas::elementUpdated);
    connect(m_MG_element_drawer, SIGNAL(newElementSettled()), this, SLOT(refreshMeasurementResults()));
}

/**
 * @brief ElementCanvas::~ElementCanvas
 * 析构函数
 */
ElementCanvas::~ElementCanvas()
{
    // 删除元素
    QVector<Element*>::Iterator iter;
    for(iter = m_MG_elements.begin(); iter != m_MG_elements.end(); iter++)
        delete (*iter);
    for(iter = m_AG_elements.begin(); iter != m_AG_elements.end(); iter++)
        delete (*iter);
    // 删除画家
    delete m_MG_element_drawer;
    delete m_AG_element_drawer;
    // 删除painter
    delete m_canvas_painter;
}

// 事件
// ---
/**
 * @brief ElementCanvas::paintEvent
 * 绘制事件回调函数
 */
void ElementCanvas::paintEvent(QPaintEvent *event)
{
    // 先画图像
    // -------
    ImgViewer::paintEvent(event);
    // 再画元素
    // -------
    m_canvas_painter->begin(this);
    m_MG_element_drawer->draw(m_canvas_painter);
    m_AG_element_drawer->draw(m_canvas_painter);
    m_canvas_painter->end();
}

/**
 * @brief ElementCanvas::wheelEvent
 * 鼠标滚动事件回调函数
 */
void ElementCanvas::wheelEvent(QWheelEvent *event)
{
    ImgViewer::wheelEvent(event);
}

/**
 * @brief ElementCanvas::mousePressEvent
 * 鼠标点击事件回调函数，根据当前元素类型和画家状态做出不同的操作
 * 具体操作见下图：
 *       |   NoElement : return
 *       |   WithElement
 *              |       左键
 *                       |       drawer.status == Leisure : create(type); setIntention(pos);
 *                       |       drawer.status == Composing : setPoint(pos);
 *              |       右键
 *                       |       drawer.status == Composing : stepBack(); setIntention(pos);
 */
void ElementCanvas::mousePressEvent(QMouseEvent *event)
{
    QPointF pos = event->pos();
    pos = physicalToLogical(pos);
    adsorbCursorToFeaturePoint(pos);

    qDebug("xxx");
    // 当前不准备构建Element
    if(m_current_element_type == ElementType::Type_NoElement){
        if(m_current_drawer->getAdjcentElementIdx() != -1){
            if(m_current_drawer == m_AG_element_drawer)
                emit elementSelected(m_AG_elements[m_current_drawer->getAdjcentElementIdx() - 1]);
            else if(m_current_drawer == m_MG_element_drawer)
                emit elementSelected(m_MG_elements[m_current_drawer->getAdjcentElementIdx() - 1]);
        }
        ImgViewer::mousePressEvent(event);
        qDebug("xxx1");
        return;
    }
    else if(m_current_element_type == ElementType::Type_DeleteElement){
        m_current_drawer->eraseElementUnderPos(pos);
        refreshMeasurementResults();
        emit elementErased();
        qDebug("xxx2");
    }
    // 当前准备构建Element
    else{
        qDebug("xxx3");
        // 左键
        if(event->button() == Qt::LeftButton){
            qDebug("xxx31");
            switch(m_current_drawer->getStatus()){
            case Leisure:
                m_current_drawer->create(m_current_element_type);
                m_current_drawer->setIntention(pos);
                break;
            case Composing:
                m_current_drawer->setPoint(pos);
                break;
            }
        }
        // 右键
        if(event->button() == Qt::RightButton){
            qDebug("xxx32");
            switch(m_current_drawer->getStatus()){
            case Leisure:
                break;
            case Composing:
                m_current_drawer->stepBack();
                m_current_drawer->setIntention(pos);
                break;
            }
        }
    }
    // 新元素构建完成
    update();
}

/**
 * @brief ElementCanvas::mouseMoveEvent
 * 鼠标移动事件回调函数
 */
void ElementCanvas::mouseMoveEvent(QMouseEvent *event)
{
    QPointF pos = event->pos();
    pos = physicalToLogical(pos);
    adsorbCursorToFeaturePoint(pos);

    if(m_current_drawer->getStatus() == Leisure
        && m_current_element_type != Type_NoElement)
        m_current_drawer->create(m_current_element_type);

    ElementCanvas::setIntention(pos);

    ImgViewer::mouseMoveEvent(event);
    update();
}

/**
 * @brief ElementCanvas::mouseDoubleClickEvent
 * 鼠标双击事件回调函数
 */
void ElementCanvas::mouseDoubleClickEvent(QMouseEvent* event)
{
    QPointF pos = event->pos();
    pos = physicalToLogical(pos);

    m_current_drawer->setEndPoint(pos);
}

/**
 * @brief ElementCanvas::setIntention
 * 设置意向点，三个画家均设置
 */
void ElementCanvas::setIntention(QPointF pos_)
{
    m_current_drawer->setIntention(pos_);
    m_MG_element_drawer->setCursor(pos_);
    m_AG_element_drawer->setCursor(pos_);
}

/**
 * @brief ElementCanvas::adsorbCursorToFeaturePoint
 * 检测 “逻辑坐标系” 下的点pt_是否在 “物理坐标系” 下临近某特征点
 * 如果临近，则将pt_修改为该特征点
 */
void ElementCanvas::adsorbCursorToFeaturePoint(QPointF& pt_)
{
    if(m_current_drawer->ajacentFeaturePoint(pt_))
        return;
    else if(m_MG_element_drawer->ajacentFeaturePoint(pt_))
        return;
    else if(m_AG_element_drawer->ajacentFeaturePoint(pt_))
        return;
}

/**
 * @brief ElementCanvas::setType
 * 设置当前元素类型
 */
void ElementCanvas::setType(ElementType type_)
{
    m_current_element_type = type_;
    // 向外发送当前Element清空的信号
    emit elementErased();
    // 清除当前Element
    m_current_drawer->eraseCurrentElement();
    if(type_ >= ElementType::Type_MG_Bottom && type_ <= ElementType::Type_MG_Top
        && m_target != nullptr)
        m_current_drawer = m_MG_element_drawer;
    if(type_ >= ElementType::Type_AG_Bottom && type_ <= ElementType::Type_AG_Top)
        m_current_drawer = m_AG_element_drawer;
    update();
}

/**
 * @brief ElementCanvas::setTarget
 * 设置Target,并清除绘制痕迹
 */
void ElementCanvas::setTarget(const Target& target_)
{
    /**< Debug begin */
    delete m_target;
    m_target = new Target(target_);
    updateImg(m_target->topo2d, true, true);
    /**< Debug end */
    deleteElements("AG");
    deleteElements("MG");
    refreshMeasurementResults();
}

/**
 * @brief ElementCanvas::updateTarget
 * 更新Target,保留绘制痕迹并重新计算测量结果
 */
void ElementCanvas::updateTarget(const Target &target_)
{
    if(m_target == nullptr || target_.isNull())
        setTarget(target_);
    else{
        delete m_target;
        m_target = new Target(target_);
        m_MG_element_drawer->onTargetUpdated();
        m_AG_element_drawer->onTargetUpdated();
        refreshMeasurementResults();
        update();
    }
}

/**
 * @brief ElementCanvas::deleteElement
 * 清除属于category_范畴的所有元素
 *
 * @param category_: 元素范畴
 * 包括"MG"-测量图形 和 "AG"-辅助图像两种
 */
void ElementCanvas::deleteElements(const QString &category_)
{
    Drawer* drawer;
    QVector<Element*>* elements;
    // 判断种类
    if(category_ == "MG"){
        drawer = m_MG_element_drawer;
        elements = &m_MG_elements;
        emit elementErased();
    }
    else if(category_ == "AG"){
        drawer = m_AG_element_drawer;
        elements = &m_AG_elements;
        emit elementErased();
    }
    else
        return;
    // 清空当前元素
    drawer->eraseCurrentElement();
    // 清空已建元素
    QVector<Element*>::Iterator iter;
    for(iter = elements->begin(); iter != elements->end(); iter++)
        delete (*iter);
    elements->clear();
    m_current_element_type = Type_NoElement;
    // 刷新
    refreshMeasurementResults();
    update();
}

/**
 * @brief ElementCanvas::setMMAMToolBox
 * 设置主测量和面积测量用到的ToolBox
 */
void ElementCanvas::setMGToolBox(const ToolBox &tool_box_)
{
    m_MG_element_drawer->setToolBox(tool_box_);
}

/**
 * @brief ElementCanvas::setAGToolBox
 * 设置辅助图形ToolBox
 */
void ElementCanvas::setAGToolBox(const ToolBox &tool_box_)
{
    m_AG_element_drawer->setToolBox(tool_box_);
}

/**
 * @brief ElementCanvas::getMeasurementResult
 * 获取所有元素的测量结果列表
 */
QString ElementCanvas::getMeasurementResults()
{
    return m_measurement_results;
}

/**
 * @brief ElementCanvas::refreshMeasurementResult
 * 更新测量结果列表
 */
void ElementCanvas::refreshMeasurementResults()
{
    // 更新
    m_measurement_results = QString();
    QVector<Element*>::Iterator iter;
    for(iter = m_MG_elements.begin(); iter != m_MG_elements.end(); iter++)
        m_measurement_results += (*iter)->getLabel() + "\n";
    // 发出消息
    emit measurementResultsUpdated(m_measurement_results);
}
