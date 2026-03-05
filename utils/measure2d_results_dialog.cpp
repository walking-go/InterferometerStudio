#include "measure2d_results_dialog.h"
#include "ui_measure2d_results_dialog.h"

Measrue2DResultsDialog::Measrue2DResultsDialog(QWidget *parent) :
    QDialog(parent, Qt::Widget | Qt::WindowStaysOnTopHint),
    ui(new Ui::Measrue2DResultsDialog)
{
    ui->setupUi(this);
}

Measrue2DResultsDialog::~Measrue2DResultsDialog()
{
    delete ui;
}

void Measrue2DResultsDialog::setText(QString content_)
{
    ui->textBrowser->setText(content_);
}
