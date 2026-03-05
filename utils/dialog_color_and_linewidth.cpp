#include "dialog_color_and_linewidth.h"
#include "ui_dialog_color_and_linewidth.h"

DialogColorAndLineWidth::DialogColorAndLineWidth(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogColorAndLineWidth)
{
    ui->setupUi(this);
    m_tool_box = ToolBox();
    // 设置线宽允许范围
    // -------------
    ui->spinBox_measurementWidth->setMinimum(MIN_LINE_WIDTH);
    ui->spinBox_measurementWidth->setMaximum(MAX_LINE_WIDTH);
    ui->spinBox_mainWidth->setMinimum(MIN_LINE_WIDTH);
    ui->spinBox_mainWidth->setMaximum(MAX_LINE_WIDTH);
    ui->spinBox_basicWidth->setMinimum(MIN_LINE_WIDTH);
    ui->spinBox_basicWidth->setMaximum(MAX_LINE_WIDTH);
    ui->spinBox_assistWidth->setMinimum(MIN_LINE_WIDTH);
    ui->spinBox_assistWidth->setMaximum(MAX_LINE_WIDTH);
}

DialogColorAndLineWidth::~DialogColorAndLineWidth()
{
    delete ui;
}

ToolBox DialogColorAndLineWidth::getToolBox(const ToolBox &initial, QWidget *parent, const QString &title)
{
    this->setWindowTitle(title);
    // 设置成员变量
    // ----------
    m_tool_box = initial;
    // 设置QColorDialog的自定义颜色
    QColorDialog::setCustomColor(0, initial.font_pen.color());
    QColorDialog::setCustomColor(1, initial.area_brush.color());
    QColorDialog::setCustomColor(2, initial.measurement_pen.color());
    QColorDialog::setCustomColor(3, initial.main_pen.color());
    QColorDialog::setCustomColor(4, initial.basic_pen.color());
    QColorDialog::setCustomColor(5, initial.assist_pen.color());

    // 设置ui初始状态
    // ------------
    // 颜色
    ui->pushButton_fontColor->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                .arg(initial.font_pen.color().red())
                                                .arg(initial.font_pen.color().green())
                                                .arg(initial.font_pen.color().blue())
                                                .arg(initial.font_pen.color().alpha()));
    ui->pushButton_areaBrush->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                .arg(initial.area_brush.color().red())
                                                .arg(initial.area_brush.color().green())
                                                .arg(initial.area_brush.color().blue())
                                                .arg(initial.area_brush.color().alpha()));
    ui->pushButton_measurementColor->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                       .arg(initial.measurement_pen.color().red())
                                                       .arg(initial.measurement_pen.color().green())
                                                       .arg(initial.measurement_pen.color().blue())
                                                       .arg(initial.measurement_pen.color().alpha()));
    ui->pushButton_mainColor->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                .arg(initial.main_pen.color().red())
                                                .arg(initial.main_pen.color().green())
                                                .arg(initial.main_pen.color().blue())
                                                .arg(initial.main_pen.color().alpha()));
    ui->pushButton_basicColor->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                 .arg(initial.basic_pen.color().red())
                                                 .arg(initial.basic_pen.color().green())
                                                 .arg(initial.basic_pen.color().blue())
                                                 .arg(initial.basic_pen.color().alpha()));
    ui->pushButton_assistColor->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                  .arg(initial.assist_pen.color().red())
                                                  .arg(initial.assist_pen.color().green())
                                                  .arg(initial.assist_pen.color().blue())
                                                  .arg(initial.assist_pen.color().alpha()));
    // 线宽
    ui->spinBox_measurementWidth->setValue(initial.measurement_pen.width());
    ui->spinBox_mainWidth->setValue(initial.main_pen.width());
    ui->spinBox_basicWidth->setValue(initial.basic_pen.width());
    ui->spinBox_assistWidth->setValue(initial.assist_pen.width());

    // 执行
    // ---
    this->exec();
    if(this->result())
        return m_tool_box;
    else
        return initial;
}

void DialogColorAndLineWidth::on_pushButton_measurementColor_clicked()
{
    // 弹窗
    const QColor color = QColorDialog::getColor(m_tool_box.measurement_pen.color(), this, tr("选择颜色 "), QColorDialog::ShowAlphaChannel);
    if(color.isValid()){
        // 设置ui状态
        ui->pushButton_measurementColor->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                           .arg(color.red())
                                                           .arg(color.green())
                                                           .arg(color.blue())
                                                           .arg(color.alpha()));
        // 设置ToolBox状态
        m_tool_box.measurement_pen.setColor(color);
    }
}

void DialogColorAndLineWidth::on_pushButton_mainColor_clicked()
{
    // 弹窗
    const QColor color = QColorDialog::getColor(m_tool_box.main_pen.color(), this, tr("选择颜色 "), QColorDialog::ShowAlphaChannel);
    if(color.isValid()){
        // 设置ui状态
        ui->pushButton_mainColor->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                    .arg(color.red())
                                                    .arg(color.green())
                                                    .arg(color.blue())
                                                    .arg(color.alpha()));
        // 设置ToolBox状态
        m_tool_box.main_pen.setColor(color);
    }
}

void DialogColorAndLineWidth::on_pushButton_basicColor_clicked()
{
    // 弹窗
    const QColor color = QColorDialog::getColor(m_tool_box.basic_pen.color(), this, tr("选择颜色 "), QColorDialog::ShowAlphaChannel);
    if(color.isValid()){
        // 设置ui状态
        ui->pushButton_basicColor->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                     .arg(color.red())
                                                     .arg(color.green())
                                                     .arg(color.blue())
                                                     .arg(color.alpha()));
        // 设置ToolBox状态
        m_tool_box.basic_pen.setColor(color);
    }
}

void DialogColorAndLineWidth::on_pushButton_assistColor_clicked()
{
    // 弹窗
    const QColor color = QColorDialog::getColor(m_tool_box.assist_pen.color(), this, tr("选择颜色 "), QColorDialog::ShowAlphaChannel);
    if(color.isValid()){
        // 设置ui状态
        ui->pushButton_assistColor->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                      .arg(color.red())
                                                      .arg(color.green())
                                                      .arg(color.blue())
                                                      .arg(color.alpha()));
        // 设置ToolBox状态
        m_tool_box.assist_pen.setColor(color);
    }
}

void DialogColorAndLineWidth::on_pushButton_fontColor_clicked()
{
    // 弹窗
    const QColor color = QColorDialog::getColor(m_tool_box.font_pen.color(), this, tr("选择颜色 "), QColorDialog::ShowAlphaChannel);
    if(color.isValid()){
        // 设置ui状态
        ui->pushButton_fontColor->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                    .arg(color.red())
                                                    .arg(color.green())
                                                    .arg(color.blue())
                                                    .arg(color.alpha()));
        // 设置ToolBox状态
        m_tool_box.font_pen.setColor(color);
    }
}

void DialogColorAndLineWidth::on_pushButton_areaBrush_clicked()
{
    // 弹窗
    const QColor color = QColorDialog::getColor(m_tool_box.area_brush.color(), this, tr("选择颜色 "), QColorDialog::ShowAlphaChannel);
    if(color.isValid()){
        // 设置ui状态
        ui->pushButton_areaBrush->setStyleSheet(QString("background-color:rgba(%1, %2, %3, %4)")
                                                    .arg(color.red())
                                                    .arg(color.green())
                                                    .arg(color.blue())
                                                    .arg(color.alpha()));
        // 设置ToolBox状态
        m_tool_box.area_brush.setColor(color);
    }
}

void DialogColorAndLineWidth::on_spinBox_measurementWidth_valueChanged(int value_)
{
    m_tool_box.measurement_pen.setWidth(value_);
}

void DialogColorAndLineWidth::on_spinBox_mainWidth_valueChanged(int value_)
{
    m_tool_box.main_pen.setWidth(value_);
}

void DialogColorAndLineWidth::on_spinBox_basicWidth_valueChanged(int value_)
{
    m_tool_box.basic_pen.setWidth(value_);
}

void DialogColorAndLineWidth::on_spinBox_assistWidth_valueChanged(int value_)
{
    m_tool_box.assist_pen.setWidth(value_);
}
