#ifndef ELEMENT_H
#define ELEMENT_H

#include "uddm_namespace.h"
#include "canvas_namespace.h"
#include <QDebug>

/**
 * @brief The Element class 抽象类
 *
 * 数据上：Element是点的集合，随着用户鼠标点击画布而被创建和完善
 * 绘制上：Element是各种图元（Dot、Line等）的组合体
 */
class Element : public QObject
{
    Q_OBJECT
public:
    explicit Element(const ElementType& type_):m_type(type_){};
    explicit Element(const ElementType& type_, const Target* target_):m_target(target_), m_type(type_){};
    ~Element(){if(m_feature_exceptions != nullptr) delete m_feature_exceptions; };

    // 点集的设置
    // --------
    // 设置意向点
    virtual void setIntention(const QPointF& pos_);
    // 设置下一点
    virtual void setPoint(const QPointF& pos_);
    // 设置终止点
    virtual void setEndPoint(const QPointF& pos_) {};
    // 撤销上一点
    virtual void stepBack();
    // 计算测量结果, 针对测量类元素(MG ELement)有效
    virtual qreal calculateResult(bool recalculate_ = false){return 0; };
    // 重新计算测量结果, 当Target更新时使用
    virtual qreal reCalculateResult(){return calculateResult(true); };

    // Get Set类函数
    // ------------
    bool isFinished() const {return m_finished; };
    int getLength() const {return m_points.size(); };
    ElementType getType() const {return m_type; };
    int getN() const {return m_N; };
    void setTarget(const Target* target_){
        m_target = target_;
        if(m_type == ElementType::Type_MM_DeltaZ)
            reCalculateResult();};
    qreal getResult() const {return m_result; };
    QString getLabel() const {return m_label; };
    QVector<QPointF> getPoints() const {return m_points;};
    QVector<QPointF> getFeaturePoints() const {return m_feature_points;};
    virtual void setIDPrefix(QString prefix_){
        m_prefix = prefix_;
        m_label = m_prefix + m_description + QString::number(m_result, 'g', DIST_PRECISION + numOfIntergerDigit(m_result)) + m_unit;};
    QPointF getLastPoint() const{
        if(m_points.isEmpty())
            return QPointF(-1.0, -1.0);
        else
            return m_points.back();
    }

    // 需要重写的纯虚函数
    // ---------------
    virtual void draw(ElementCanvas* canvas_, QPainter* painter_, const ToolBox& tool_box_, const QString& text_ = NULL) = 0;
    virtual bool aroundMe(ElementCanvas* canvas_, const QPointF& cursor_pos_, const ToolBox& tool_box_) = 0;
    virtual bool aroundMyFeature(ElementCanvas* canvas_, QPointF& feature_pt_, int* exception_, int n_exception_);
    virtual bool aroundMyFeature(ElementCanvas* canvas_, QPointF& feature_pt_);

protected:
    // 类型及状态
    ElementType m_type;                   /**< Element类型*/
    ElementStatus m_status = Empty;       /**< 当前状态*/
    bool m_finished = false;              /**< 是否已完成*/
    // 点集
    int m_N = 99999;                      /**< 完成该Element所须的点数*/
    QVector<QPointF> m_points;            /**< 点集合, 坐标系:逻辑坐标系*/
    // 特征点
    QVector<QPointF> m_feature_points;    /**< 特征点集合, 坐标系:逻辑坐标系*/
    int* m_feature_exceptions = nullptr;  /**< 检测是否临近特征点时跳过的特征点*/
    int m_n_feature_exceptions = 0;       /**< 检测是否临近特征点时跳过的特征点的个数*/
    // 针对M2d的属性
    QString m_prefix = "[1]";             /**< 文字前缀, 对应测量列表中的ID*/
    const Target* m_target = nullptr;     /**< 该元素所针对的目标*/
    qreal m_result = 0.0;                 /**< 测量结果*/
    QString m_unit = "";                  /**< 测量单位*/
    QString m_label = "";                 /**< m_prefix + m_result + m_unit*/
    QString m_description = "";           /**< 类型描述*/
};
#endif // ELEMENT_H
