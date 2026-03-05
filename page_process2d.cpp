#include "page_process2d.h"
#include "utils/dialog_color_and_linewidth.h"
#include "qthread.h"
#include "ui_page_process2d.h"
#include <QFontDialog>
#include <QThread>



PageProcess2D::PageProcess2D(bool showLegend, QWidget *parent ) :
    QWidget(parent),
    ui(new Ui::PageProcess2D)
{
    // UI初始化
    ui->setupUi(this);
    ui->checkBox_scaler->setChecked(showLegend);
    ui->checkBox_resultDialog->setChecked(false);
    ui->radioButton_grayscale->setChecked(false);
    ui->radioButton_pseudoColor->setChecked(true);
    // 成员变量初始化
    m_results_dialog = new Measrue2DResultsDialog;
    initMGToolBox();
    initAGToolBox();
    initExclusiveButtonGroup();
    // 信号槽初始化
    connect(m_results_dialog, SIGNAL(finished(int)), this, SLOT(on_results_dialog_closed(int)));

}

PageProcess2D::~PageProcess2D()
{
    delete ui;
    delete m_results_dialog;
}

/**
 * @brief PageProcess2D::initMGToolBox
 * 初始化MG工具箱, MG工具箱:控制MG(MainGraph)颜色、线宽、字体等
 */
void PageProcess2D::initMGToolBox()
{
    m_MG_toolbox = ToolBox();
    // measurement_pen
    m_MG_toolbox.measurement_pen.setColor(Qt::red);
    m_MG_toolbox.measurement_pen.setWidth(4);
    m_MG_toolbox.measurement_pen.setCapStyle(Qt::RoundCap);
    // main_pen
    m_MG_toolbox.main_pen.setColor(Qt::blue);
    m_MG_toolbox.main_pen.setStyle(Qt::DotLine);
    m_MG_toolbox.main_pen.setWidth(4);
    m_MG_toolbox.main_pen.setCapStyle(Qt::RoundCap);
    // basic_pen
    m_MG_toolbox.basic_pen.setColor(Qt::red);
    m_MG_toolbox.basic_pen.setWidth(2);
    m_MG_toolbox.basic_pen.setCapStyle(Qt::RoundCap);
    // assist_pen
    m_MG_toolbox.assist_pen.setColor(Qt::yellow);
    m_MG_toolbox.assist_pen.setWidth(2);
    m_MG_toolbox.assist_pen.setCapStyle(Qt::RoundCap);
    // font_pen
    m_MG_toolbox.font_pen.setColor(Qt::black);
    // basic_font
    m_MG_toolbox.basic_font.setFamily("Times New Roman");
    m_MG_toolbox.basic_font.setWeight(QFont::Normal);
    // font_bg_brush
    m_MG_toolbox.font_bg_brush.setColor(Qt::white);
    // area_brush
    QColor area_brush_color = m_MG_toolbox.measurement_pen.color();
    area_brush_color.setAlpha(30);
    m_MG_toolbox.area_brush.setColor(area_brush_color);
}

/**
 * @brief PageProcess2D::initAGToolBox
 * 初始化AG工具箱, AG工具箱:控制AG(AssistGraph)颜色、线宽、字体等
 */
void PageProcess2D::initAGToolBox()
{
    m_AG_toolbox = ToolBox();
    // main_pen
    m_AG_toolbox.main_pen.setColor(Qt::blue);
    m_AG_toolbox.main_pen.setStyle(Qt::DotLine);
    m_AG_toolbox.main_pen.setWidth(3);
    m_AG_toolbox.main_pen.setCapStyle(Qt::RoundCap);
}

/**
 * @brief PageProcess2D::initExclusiveButtonGroup
 * 初始化互斥按键组,
 * 互斥按键组：当一个按键按下时，和它互斥的按键将全部被弹起，这样的一组按键构成一个互斥按键组
 */
void PageProcess2D::initExclusiveButtonGroup()
{
    m_exclusive_buttons.append(ui->pushButton_MM_deltaXY);
    m_exclusive_buttons.append(ui->pushButton_MM_deltaZ);
    m_exclusive_buttons.append(ui->pushButton_MM_radian);
    m_exclusive_buttons.append(ui->pushButton_MM_diameter);
    m_exclusive_buttons.append(ui->pushButton_MM_radius);
    m_exclusive_buttons.append(ui->pushButton_MM_count);
    m_exclusive_buttons.append(ui->pushButton_AM_circle);
    m_exclusive_buttons.append(ui->pushButton_AM_rect);
    m_exclusive_buttons.append(ui->pushButton_AM_polygon);
    m_exclusive_buttons.append(ui->pushButton_MG_delete);

    m_exclusive_buttons.append(ui->pushButton_AG_dot);
    m_exclusive_buttons.append(ui->pushButton_AG_lineSegment);
    m_exclusive_buttons.append(ui->pushButton_AG_circle);
    m_exclusive_buttons.append(ui->pushButton_AG_line);
    m_exclusive_buttons.append(ui->pushButton_AG_horizontalLine);
    m_exclusive_buttons.append(ui->pushButton_AG_verticalLine);
    m_exclusive_buttons.append(ui->pushButton_AG_delete);
}

/**
 * @brief PageProcess2D::doExclusiveInGroup
 * 执行互斥操作：选中button_，取消其所在互斥按键组中其他按键的选中
 */
void PageProcess2D::doExclusiveInGroup(QPushButton *button_)
{
    for(QVector<QPushButton*>::iterator iter = m_exclusive_buttons.begin();
         iter != m_exclusive_buttons.end();
         iter++){
        if((*iter) != button_)
            (*iter)->setChecked(false);
        else
            (*iter)->setChecked(true);
    }
}

/**
 * @brief PageProcess2D::showEvent
 * 显示事件
 * 向外广播2D处理部分MG、AG绘图工具箱
 */
void PageProcess2D::showEvent(QShowEvent *event)
{
    emit setMGToolBox(m_MG_toolbox);
    emit setAGToolBox(m_AG_toolbox);
}

/**
 * @brief PageProcess2D::on_checkBox_scaler_clicked
 * 响应checkBox_scaler点击事件，用以指示左区Topo2D显示部分是否显示topo2d的图例
 */
void PageProcess2D::on_checkBox_scaler_clicked(bool checked_)
{
    emit legendVisibilityChanged(checked_); // 发送信号替代直接修改全局变量
    emit refreshTarget();
    emit scalarStateSetted(checked_);
}

/**
 * @brief PageProcess2D::onScalarStateSetted
 * 响应外界标尺开关状态更新事件
 */
void PageProcess2D::onScalarStateSetted(bool checked_)
{
    ui->checkBox_scaler->setChecked(checked_);
}

/**
 * @brief PageProcess2D::on_pushButton_MG_colorAndLineWidth_clicked
 * 响应pushButton_MG_colorAndLineWidth按下事件，用来设置MG绘图工具箱的颜色&线宽
 */
void PageProcess2D::on_pushButton_MG_colorAndLineWidth_clicked()
{
    DialogColorAndLineWidth dialog;
    m_MG_toolbox = dialog.getToolBox(m_MG_toolbox, nullptr, tr("颜色/线宽"));
    emit setMGToolBox(m_MG_toolbox);
}

/**
 * @brief PageProcess2D::on_pushButton_MG_font_clicked
 * 响应on_pushButton_MG_font_clicked按下事件，用来设置MG绘图工具箱的字体
 */
void PageProcess2D::on_pushButton_MG_font_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_MG_toolbox.basic_font);
    if(ok){
        m_MG_toolbox.basic_font = font;
        emit setMGToolBox(m_MG_toolbox);
    }
}

/**
 * @brief PageProcess2D::on_checkBox_resultDialog_clicked
 * 响应checkBox_resultDialog点击事件，控制是否显示2D测量结果窗口
 */
void PageProcess2D::on_checkBox_resultDialog_clicked(bool checked_)
{
    if(checked_)
        m_results_dialog->show();
    else
        m_results_dialog->close();
}

/**
 * @brief PageProcess2D::updateMeasurementResults
 * 更新2D测量结果窗口中的数据
 */
void PageProcess2D::updateMeasurementResults(const QString &text_)
{
    m_results_dialog->setText(text_);
}

/**
 * @brief PageProcess2D::on_results_dialog_closed
 * 响应2D测量结果窗口关闭事件
 */
void PageProcess2D::on_results_dialog_closed(int result_)
{
    ui->checkBox_resultDialog->setChecked(false);
}

