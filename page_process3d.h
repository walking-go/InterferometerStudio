#ifndef PAGE_PROCESS3D_H
#define PAGE_PROCESS3D_H

#include "utils/canvas_namespace.h"
#include "qpushbutton.h"
#include "uddm_namespace.h"
#include "ui_page_process3d.h"
#include <QWidget>

namespace Ui {
class PageProcess3D;
}

/**
 * @brief The PageProcess3D class
 * 三维处理页，实际上是用户控制左区ElementCanavs等的响应中转站
 *           界面按键与左区所需绘制的元素相对应
 * 类内成员变量和函数的更具体的描述见page_process2d.h文件
 * 功能：
 * · 响应各种图形元素按键的点击事件，并向外广播被选择的元素类型
 * · 管理绘图工具箱
 * · 控制3D标尺显示与否
 * · 控制Topo2DStyle
 */
class PageProcess3D : public QWidget
{
    Q_OBJECT

public:
    explicit PageProcess3D(bool m_show_topo2d_legend,QWidget *parent = nullptr);
    ~PageProcess3D();
    Ui::PageProcess3D *ui;

private:
    ToolBox m_AG_toolbox;
    void initAGToolBox();
    QVector<QPushButton*> m_exclusive_buttons;
    void initExclusiveButtonGroup();
    void doExclusiveInGroup(QPushButton* button_ = nullptr);

protected:
    virtual void showEvent(QShowEvent* event);

signals:
    void scalarStateSetted(bool checked_);
    void refreshTarget();                           /** 请求外界刷新Target相关的显示，与MainWindow中onTargetUpdated()相链接*/
    void setTopo2DStyle(Topo2DStyle);
    void setElementType(ElementType type_);
    void setAGToolBox(const ToolBox& tool_box_);
    void deleteElements(const QString& category_);
     void legendVisibilityChanged(bool visible);  // 通知标尺显示状态变更

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

    // 元素选择
    void on_pushButton_element_clicked_wrapper(bool checked_, ElementType type_, QPushButton* button_){
        if(checked_){
            emit setElementType(type_);
            doExclusiveInGroup(button_);
        }
        else
            emit setElementType(ElementType::Type_NoElement);
    }
    void on_pushButton_lineSegment_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AG_ArrowedLineSegment, ui->pushButton_lineSegment);
    }
    void on_pushButton_circle_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AG_ClockwiseCircle, ui->pushButton_circle);
    }
    void on_pushButton_line_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AG_ArrowedLine, ui->pushButton_line);
    }
    void on_pushButton_verticalLine_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AG_ArrowedVerticalLine, ui->pushButton_verticalLine);
    }
    void on_pushButton_horizontalLine_clicked(bool checked_){
        on_pushButton_element_clicked_wrapper(checked_, Type_AG_ArrowedHorizontalLine, ui->pushButton_horizontalLine);
    }
    void on_pushButton_delete_clicked(bool checked_){
        emit setElementType(ElementType::Type_AG_Bottom);
        on_pushButton_element_clicked_wrapper(checked_, Type_DeleteElement, ui->pushButton_delete);
    }
    void on_pushButton_deleteAll_clicked(){
        emit deleteElements("AG");
        doExclusiveInGroup();
    }

    // 2D标尺
    void on_radioButton_scalarOn_clicked();
    void on_radioButton_scalarOff_clicked();
    void onScalarStateSetted(bool checked_);

    // 统计信息更新
    void updateStatistics(double pv, double rms);
};

#endif // PAGE_PROCESS3D_H
