#ifndef DIALOG_COLOR_AND_LINEWIDTH_H
#define DIALOG_COLOR_AND_LINEWIDTH_H

#include "canvas_namespace.h"
#include <QDialog>
#include <QColorDialog>

namespace Ui {
class DialogColorAndLineWidth;
}

/**
 * @brief The DialogColorAndLineWidth class
 * 自定义的QDialog类，专用来设置Canvas的绘制工具箱ToolBox
 * 包括各种线条的颜色、线宽
 */
class DialogColorAndLineWidth : public QDialog
{
    Q_OBJECT

public:
    explicit DialogColorAndLineWidth(QWidget *parent = nullptr);
    ~DialogColorAndLineWidth();
    ToolBox getToolBox(const ToolBox& initial, QWidget *parent = nullptr, const QString &title = QString());

private:
    Ui::DialogColorAndLineWidth *ui;
    ToolBox m_tool_box;

public slots:
    // 颜色
    void on_pushButton_measurementColor_clicked();
    void on_pushButton_mainColor_clicked();
    void on_pushButton_basicColor_clicked();
    void on_pushButton_assistColor_clicked();
    void on_pushButton_fontColor_clicked();
    void on_pushButton_areaBrush_clicked();
    // 线宽
    void on_spinBox_measurementWidth_valueChanged(int value_);
    void on_spinBox_mainWidth_valueChanged(int value_);
    void on_spinBox_basicWidth_valueChanged(int value_);
    void on_spinBox_assistWidth_valueChanged(int value_);
};

#endif // DIALOG_COLOR_AND_LINEWIDTH_H
