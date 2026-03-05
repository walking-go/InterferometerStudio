#ifndef PAGE_PROCESS2D_H
#define PAGE_PROCESS2D_H

#include "utils/canvas_namespace.h"
#include "qpushbutton.h"
#include <QWidget>
#include <QFont>
#include "QDebug"
#include "uddm_namespace.h"
#include "ui_page_process2d.h"
#include "utils/measure2d_results_dialog.h"

namespace Ui {
class PageProcess2D;
}

/**
 * @brief The PageProcess2D class
 * 二维处理页，实际上是用户控制左区ElementCanavs等的响应中转站
 *           界面按键与左区所需绘制的元素相对应
 * 特点：
 * · 元素分MG(MainGraph)和AG(AssistGraph)两大类
 * · MG包括主测量、面积测量两大类
 * · AG包括辅助图形一类
 * · 按键之间有合适的互斥关系,当一个按键按下时，和它互斥的按键将全部被弹起
 * 功能：
 * · 响应各种图形元素按键的点击事件，并向外广播被选择的元素类型
 * · 管理主测量/面积测量的绘图工具箱
 * · 控制2D标尺显示与否
 * · 控制2D测量结果窗口显示与否
 */
class PageProcess2D : public QWidget
{
    Q_OBJECT

public:
    explicit PageProcess2D(bool m_show_topo2d_legend,QWidget *parent = nullptr);
    ~PageProcess2D();
    Ui::PageProcess2D *ui;
    Measrue2DResultsDialog* m_results_dialog;

private:
    ToolBox m_MG_toolbox;   /** MG工具箱, 控制颜色、线宽、字体等*/
    ToolBox m_AG_toolbox;   /** AG工具箱, 控制颜色、线宽、字体等*/
    void initMGToolBox();   /** 初始化MG工具箱*/
    void initAGToolBox();   /** 初始化AG工具箱*/
    QVector<QPushButton*> m_exclusive_buttons;                  /** 互斥按键组*/
    void initExclusiveButtonGroup();                            /** 初始化互斥按键组*/
    void doExclusiveInGroup(QPushButton* button_ = nullptr);    /** 执行互斥操作：选中button_，取消其所在互斥按键组中其他按键的选中*/

protected:
    virtual void showEvent(QShowEvent* event);

signals:
    void refreshTarget();                           /** 请求外界刷新Target相关的显示，与MainWindow中onTargetUpdated()相链接*/
    void setTopo2DStyle(Topo2DStyle style_);        /** 广播：设置Topo2DStyle*/
    void setElementType(ElementType type_);         /** 广播：下一个需要绘制的Element类型*/
    void setMGToolBox(const ToolBox& tool_box_);    /** 广播：MG工具箱*/
    void setAGToolBox(const ToolBox& tool_box_);    /** 广播：AG工具箱*/
    void deleteElements(const QString& category_);  /** 广播：删除category_类别(含"MG"和"AG")的所有元素*/
    void scalarStateSetted(bool checked_);          /** 广播：2D标尺开/关*/
    void legendVisibilityChanged(bool visible);     //标尺更新信号

public slots:
    // Topo2DStyle
    void on_radioButton_pseudoColor_clicked(){
        emit setTopo2DStyle(Topo2DStyle::PseudoColor);
    }
    void on_radioButton_grayscale_clicked(){
        emit setTopo2DStyle(Topo2DStyle::Grayscale);
    }
    void onTopo2DStyleSetted(Topo2DStyle style_){
        if(style_ == Topo2DStyle::Grayscale)
            ui->radioButton_grayscale->setChecked(true);
        if(style_ == Topo2DStyle::PseudoColor)
            ui->radioButton_pseudoColor->setChecked(true);
    };
    // Scalar
    void on_checkBox_scaler_clicked(bool checked_);
    void onScalarStateSetted(bool checked_);
    // 元素选择
    void on_pushButton_element_clicked_wrapper(bool checked_, ElementType type_, QPushButton* button_){
        if(checked_){
            emit setElementType(type_);
            doExclusiveInGroup(button_);
        }
        else
            emit setElementType(ElementType::Type_NoElement);
    }
    // MG
    // --
    // MM
    void on_pushButton_MM_deltaXY_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_MM_DeltaXY, ui->pushButton_MM_deltaXY);
    }
    void on_pushButton_MM_deltaZ_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_MM_DeltaZ, ui->pushButton_MM_deltaZ);
    }
    void on_pushButton_MM_radius_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_MM_Radius, ui->pushButton_MM_radius);
    }
    void on_pushButton_MM_diameter_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_MM_Diameter, ui->pushButton_MM_diameter);
    }
    void on_pushButton_MM_radian_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_MM_Radian, ui->pushButton_MM_radian);
    }
    void on_pushButton_MM_count_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_MM_Count, ui->pushButton_MM_count);
    }
    // AM
    void on_pushButton_AM_circle_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AM_Circle, ui->pushButton_AM_circle);
    }
    void on_pushButton_AM_rect_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AM_Rect, ui->pushButton_AM_rect);
    }
    void on_pushButton_AM_polygon_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AM_Polygon, ui->pushButton_AM_polygon);
    }
    // AG
    void on_pushButton_AG_dot_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AG_Dot, ui->pushButton_AG_dot);
    }
    void on_pushButton_AG_lineSegment_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AG_LineSegment, ui->pushButton_AG_lineSegment);
    }
    void on_pushButton_AG_circle_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AG_Circle, ui->pushButton_AG_circle);
    }
    void on_pushButton_AG_line_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AG_Line, ui->pushButton_AG_line);
    }
    void on_pushButton_AG_horizontalLine_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AG_HorizontalLine, ui->pushButton_AG_horizontalLine);
    }
    void on_pushButton_AG_verticalLine_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AG_VerticalLine, ui->pushButton_AG_verticalLine);
    }
    // 删除
    void on_pushButton_MG_delete_clicked(bool checked_){
        //emit setElementType(ElementType::Type_MG_Bottom);
        on_pushButton_element_clicked_wrapper(checked_, Type_DeleteElement, ui->pushButton_MG_delete);
    }
    void on_pushButton_AG_delete_clicked(bool checked_){
        //emit setElementType(ElementType::Type_AG_Bottom);
        on_pushButton_element_clicked_wrapper(checked_, Type_DeleteElement, ui->pushButton_AG_delete);
    }
    void on_pushButton_MG_deleteAll_clicked(){
        emit deleteElements("MG");
        doExclusiveInGroup();
    }
    void on_pushButton_AG_deleteAll_clicked(){
        emit deleteElements("AG");
        doExclusiveInGroup();
    }
    // 颜色 & 线宽 & 字体
    void on_pushButton_MG_colorAndLineWidth_clicked();
    void on_pushButton_MG_font_clicked();
    // 测量结果显示
    void on_checkBox_resultDialog_clicked(bool checked_);
    void updateMeasurementResults(const QString& text_);
    void on_results_dialog_closed(int result_);
};

#endif // PAGE_PROCESS2D_H
