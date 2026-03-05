#include "drawer.h"
#include "ag_element.h"
#include "element.h"
#include "mg_element.h"
#include "element_canvas.h"
#include "drawing_funs.h"

Drawer::Drawer(ElementCanvas* canvas_, QVector<Element*>* elements_)
    :m_canvas(canvas_), m_elements(elements_)
{
    m_tool_box = ToolBox();
    m_element_pix = QPixmap();
}

Drawer::~Drawer()
{
    if(m_current_element != nullptr)
        delete m_current_element;
}

/**
 * @brief Drawer::create
 * 创建新Element
 */
void Drawer::create(const ElementType& type_)
{
    if((int)m_elements->size() == m_max_n_element)
        return;

    // 清空创建到一半的旧元素
    eraseCurrentElement();

    // 创建新元素
    switch(type_){
    // MG
    // --
    // MM
    case Type_MM_DeltaXY:
        m_current_element = new MM_DeltaXY(type_, m_canvas->getTarget());
        break;
    case Type_MM_Radius:
        m_current_element = new MM_Radius(type_, m_canvas->getTarget());
        break;
    case Type_MM_Diameter:
        m_current_element = new MM_Diameter(type_, m_canvas->getTarget());
        break;
    case Type_MM_Radian:
        m_current_element = new MM_Radian(type_, m_canvas->getTarget());
        break;
    case Type_MM_DeltaZ:
        m_current_element = new MM_DeltaZ(type_, m_canvas->getTarget());
        break;
    case Type_MM_Count:
        m_current_element = new MM_Count(type_, m_canvas->getTarget());
        break;
    // AM
    case Type_AM_Circle:
        m_current_element = new AM_Circle(type_, m_canvas->getTarget());
        break;
    case Type_AM_Rect:
        m_current_element = new AM_Rect(type_, m_canvas->getTarget());
        break;
    case Type_AM_Polygon:
        m_current_element = new AM_Polygon(type_, m_canvas->getTarget());
        break;
    // AG
    // --
    case Type_AG_Dot:
        m_current_element = new AG_Dot(type_, m_canvas->getTarget());
        break;
    case Type_AG_LineSegment:
        m_current_element = new AG_LineSegment(type_, m_canvas->getTarget());
        break;
    case Type_AG_Circle:
        m_current_element = new AG_Circle(type_, m_canvas->getTarget());
        break;
    case Type_AG_Line:
        m_current_element = new AG_Line(type_, m_canvas->getTarget());
        break;
    case Type_AG_HorizontalLine:
        m_current_element = new AG_HorizontalLine(type_, m_canvas->getTarget());
        break;
    case Type_AG_VerticalLine:
        m_current_element = new AG_VerticalLine(type_, m_canvas->getTarget());
        break;
    // AG3D
    case Type_AG_ClockwiseCircle:
        m_current_element = new AG_ClockwiseCircle(type_, m_canvas->getTarget());
        break;
    case Type_AG_ArrowedLineSegment:
        m_current_element = new AG_ArrowedLineSegment(type_, m_canvas->getTarget());
        break;
    case Type_AG_ArrowedLine:
        m_current_element = new AG_ArrowedLine(type_, m_canvas->getTarget());
        break;
    case Type_AG_ArrowedHorizontalLine:
        m_current_element = new AG_ArrowedHorizontalLine(type_, m_canvas->getTarget());
        break;
    case Type_AG_ArrowedVerticalLine:
        m_current_element = new AG_ArrowedVerticalLine(type_, m_canvas->getTarget());
        break;
    default:
        m_current_element = nullptr;
    }

    // 为新元素设置id_prefix;
    if(m_current_element != nullptr)
        m_current_element->setIDPrefix(QString("[") + QString::number(m_elements->size() + 1) + QString("]"));

    // 更新状态
    refresh();
}

/**
 * @brief Drawer::setCursor
 * 设置光标位置
 */
void Drawer::setCursor(const QPointF &pos_)
{
    // 更新光标位置
    m_cursor_pos = pos_;
    refresh();
};

/**
 * @brief Drawer::setIntention
 * 设置意向点
 */
void Drawer::setIntention(const QPointF& pos_)
{
    // 更新光标位置
    setCursor(pos_);

    if(m_current_element == nullptr)
        return;
    if(m_current_element->getLastPoint() == pos_)
        return;

    m_current_element->setIntention(pos_);
    emit currentElementUpdated(m_current_element);
}

/**
 * @brief Drawer::setPoint
 * 设置下一点
 */
void Drawer::setPoint(const QPointF& pos_)
{
    if(m_current_element == nullptr)
        return;

    m_current_element->setPoint(pos_);
    refresh();
}

/**
 * @brief Drawer::setEndPoint
 * 设置终止点
 */
void Drawer::setEndPoint(const QPointF &pos_)
{
    if(m_current_element == nullptr)
        return;

    m_current_element->setEndPoint(pos_);
    refresh();
}

/**
 * @brief Drawer::stepBack
 * 撤销上一点
 */
void Drawer::stepBack()
{
    if(m_current_element == nullptr)
        return;
    else
        m_current_element->stepBack();
    // 更新状态
    refresh();
}

/**
 * @brief Drawer::draw
 * 绘制
 */
void Drawer::draw(QPainter* painter_)
{
    // 1. 先绘制一张透明背景的QPixmap:element_pix
    // ---------------------------------------
    m_element_pix = QPixmap(m_canvas->size());
    m_element_pix.fill(Qt::transparent);
    QPainter element_pix_painter(&m_element_pix);

    // 1.1 创建用来强调的ToolBox
    // -----------------------
    ToolBox emphasized_tool_box = m_tool_box;
    emphasizeToolBox(emphasized_tool_box);

    // 1.2 绘制已建元素
    // --------------
    QVector<Element*>::iterator iter;
    int idx = 0;
    for(iter = m_elements->begin(); iter != m_elements->end(); iter++){
        idx += 1;
        // 当没有构建中的元素时，强调光标临近的已建元素
        if(m_emphasizeClosest
            && idx == m_adjcent_element_idx
            && m_current_element == nullptr)
            (*iter)->draw(m_canvas, &element_pix_painter, emphasized_tool_box);
        else
            (*iter)->draw(m_canvas, &element_pix_painter, m_tool_box);
    }
    // 当有构建中元素，且构建中第一个点已确定时，将其他已建元素半透明
    if(m_semitransFinished && m_current_element != nullptr)
        if(m_current_element->getLength() != 1)
            setAlpha(&element_pix_painter, m_canvas->rect(), m_alpha);

    // 1.3 绘制构建中的元素
    // -----------------
    if(m_current_element != nullptr)
        m_current_element->draw(m_canvas, &element_pix_painter, m_tool_box);


    // 2. 再将element_pix绘制painter_上
    // ------------------------------
    painter_->drawPixmap(m_canvas->rect(), m_element_pix, QRect());
}

/**
 * @brief Drawer::getAdjcentElementIdx
 *
 * 获取光标最邻近位置的元素序号
 * 最小序号为1,无临近元素时返回-1
 */
int Drawer::getAdjcentElementIdx()
{
    for(int i = (int)m_elements->size() - 1; i >= 0; i--)
        if((*m_elements)[i]->aroundMe(m_canvas, m_cursor_pos, m_tool_box) == true)
            return i+1;
    //如果没有临近返回-1
    return -1;
}

/**
 * @brief Drawer::refresh
 *
 * 更新状态，包括：
 * 1. 当前光标最邻近元素
 * 2. Drawer状态
 * 3. 正在构建元素的状态
 */
void Drawer::refresh()
{
    m_adjcent_element_idx = getAdjcentElementIdx();
    // 如果current_element为空，设置状态为闲置
    if(m_current_element == nullptr){
        m_status = Leisure;
        return;
    }
    // 检查current_element是否结束
    if(m_current_element->isFinished()){
        m_elements->push_back(m_current_element);
        m_current_element = nullptr;
        m_status = Leisure;
        emit newElementSettled();
    } else
        m_status = Composing;
}

/**
 * @brief Drawer::eraseCurrentElement
 * 清空正在构建的元素
 */
void Drawer::eraseCurrentElement()
{
    if(m_current_element == nullptr)
        return;
    else{
        delete m_current_element;
        m_current_element = nullptr;
    }
}

/**
 * @brief Drawer::eraseElementUnderPos
 * 清除坐标pt_下的元素
 */
void Drawer::eraseElementUnderPos(const QPointF &pt_)
{
    if(m_adjcent_element_idx == -1)
        return;

    // 将ElementUnderPos后的元素的id_prefix减1
    QVector<Element*>::iterator iter = m_elements->begin() + m_adjcent_element_idx;
    for(; iter != m_elements->end(); iter++ ){
        int id = iter - m_elements->begin() + 1;
        (*iter)->setIDPrefix(QString("[") + QString::number(id - 1) + QString("]"));
    }
    // 将ElementUnderPos删除
    delete (*m_elements)[m_adjcent_element_idx - 1];
    m_elements->erase(m_elements->begin() + m_adjcent_element_idx - 1);

    refresh();
}

/**
 * @brief Drawer::ajacentFeaturePoint
 * 查找临近特征点,若存在临近特征点，则修改pt_至该特征点位置
 */
bool Drawer::ajacentFeaturePoint(QPointF &pt_)
{
    for(int i = (int)m_elements->size() - 1; i >= 0; i--)
        if((*m_elements)[i]->aroundMyFeature(m_canvas, pt_) == true)
            return true;
    return false;
}

/**
 * @brief Drawer::onTargetUpdated
 * 当target被更新时，做出响应，对测量元素(MG Element)的测量结果重新计算
 */
void Drawer::onTargetUpdated()
{
    // 遍历Elements
    for(QVector<Element*>::iterator iter = m_elements->begin();
         iter != m_elements->end();
         iter++ )
        (*iter)->setTarget(m_canvas->getTarget());
    if(m_current_element != nullptr)
        m_current_element->setTarget(m_canvas->getTarget());
}
