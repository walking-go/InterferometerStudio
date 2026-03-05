#ifndef MEASURE2D_RESULTS_DIALOG_H
#define MEASURE2D_RESULTS_DIALOG_H

#include "qdebug.h"
#include <QDialog>

namespace Ui {
class Measrue2DResultsDialog;
}

/**
 * @brief The Measrue2DResultsDialog class
 * 2D测量结果显示窗口，仅用来显示文字
 * 该窗口设置了Qt::WindowStaysOnTopHint属性，故始终保持在最上层显示
 */
class Measrue2DResultsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit Measrue2DResultsDialog(QWidget *parent = nullptr);
    ~Measrue2DResultsDialog();
private:
    Ui::Measrue2DResultsDialog *ui;

public slots:
    void setText(QString content_);
};

#endif // MEASURE2D_RESULTS_DIALOG_H
