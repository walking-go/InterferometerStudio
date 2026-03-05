#include "page_process3d.h"
#include "ui_page_process3d.h"


PageProcess3D::PageProcess3D(bool m_show_topo2d_legend, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageProcess3D)
{
    // UI初始化
    ui->setupUi(this);
    ui->radioButton_scalarOn->setChecked(m_show_topo2d_legend);
    ui->radioButton_scalarOff->setChecked(!m_show_topo2d_legend);
    // 成员变量初始化
    initAGToolBox();
    initExclusiveButtonGroup();
}

PageProcess3D::~PageProcess3D()
{
    delete ui;
}

/**
 * @brief PageProcess3D::initAGToolBox
 * 初始化AG工具箱, AG工具箱:控制AG(AssistGraph)颜色、线宽、字体等
 */
void PageProcess3D::initAGToolBox()
{
    m_AG_toolbox = ToolBox();
    // main_pen
    m_AG_toolbox.main_pen.setColor(Qt::red);
    m_AG_toolbox.main_pen.setStyle(Qt::SolidLine);
    m_AG_toolbox.main_pen.setWidth(3);
    m_AG_toolbox.main_pen.setCapStyle(Qt::RoundCap);
}

/**
 * @brief PageProcess3D::initExclusiveButtonGroup
 * 初始化互斥按键组,
 * 互斥按键组：当一个按键按下时，和它互斥的按键将全部被弹起，这样的一组按键构成一个互斥按键组
 */
void PageProcess3D::initExclusiveButtonGroup()
{
    m_exclusive_buttons.append(ui->pushButton_lineSegment);
    m_exclusive_buttons.append(ui->pushButton_circle);
    m_exclusive_buttons.append(ui->pushButton_line);
    m_exclusive_buttons.append(ui->pushButton_verticalLine);
    m_exclusive_buttons.append(ui->pushButton_horizontalLine);
    m_exclusive_buttons.append(ui->pushButton_delete);
}

/**
 * @brief PageProcess3D::doExclusiveInGroup
 * 执行互斥操作：选中button_，取消其所在互斥按键组中其他按键的选中
 */
void PageProcess3D::doExclusiveInGroup(QPushButton* button_)
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
 * @brief PageProcess3D::showEvent
 * 显示事件
 * 向外广播3D处理部分AG绘图工具箱
 */
void PageProcess3D::showEvent(QShowEvent *event)
{
    emit setAGToolBox(m_AG_toolbox);
}

/**
 * @brief PageProcess3D::on_radioButton_scalarOn_clicked
 * 响应radioButton_scalarOn点击事件，向外通知打开3D标尺
 */
void PageProcess3D::on_radioButton_scalarOn_clicked()
{
    emit legendVisibilityChanged(true);  // 发送打开信号
    emit refreshTarget();
    emit scalarStateSetted(true);
}

/**
 * @brief PageProcess3D::on_radioButton_scalarOff_clicked
 * 响应radioButton_scalarOff点击事件，向外通知关闭3D标尺
 */
void PageProcess3D::on_radioButton_scalarOff_clicked()
{
    emit legendVisibilityChanged(false);  // 发送关闭信号
    emit refreshTarget();
    emit scalarStateSetted(false);
}

/**
 * @brief PageProcess3D::onScalarStateSetted
 * 响应外界标尺开关状态更新事件
 */
void PageProcess3D::onScalarStateSetted(bool checked_)
{
    if(checked_)
        ui->radioButton_scalarOn->setChecked(true);
    else
        ui->radioButton_scalarOff->setChecked(true);
}

/**
 * @brief PageProcess3D::updateStatistics
 * 更新统计信息显示（PV和RMS值）
 */
void PageProcess3D::updateStatistics(double pv, double rms)
{
    ui->label_pv_value->setText(QString::number(pv, 'f', 3));
    ui->label_rms_value->setText(QString::number(rms, 'f', 3));
}
