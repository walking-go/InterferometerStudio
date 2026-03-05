#ifndef ZOOMCONTROLWIDGET_H
#define ZOOMCONTROLWIDGET_H

#include <QWidget>
#include <QSerialPortInfo>

class ZoomController; // 前向声明

namespace Ui {
class ZoomControlWidget;
}

// 只负责UI，不处理业务逻辑
class ZoomControlWidget : public QWidget
{
    Q_OBJECT

public:
    // explicit ZoomControlWidget(ZoomController *zoomController, QWidget *parent = nullptr);
    explicit ZoomControlWidget(QWidget *parent = nullptr);
    ~ZoomControlWidget();

signals:
    // 固定倍率信号
    void fixedZoomClicked(quint8 cmd);
    // 普通变倍信号
    void zoomStart(quint8 cmd);
    void zoomStop();
    // 微调信号
    void fineTuneStart(quint8 cmd);
    void fineTuneStop();

public slots:
    // 接收业务逻辑层的信号，更新UI
    void updateZoomDisplay(QString currentZoom);  // 更新倍率显示
    void updateSerialStatus(QString status);      // 更新串口状态显示
    void showError(QString errorMsg);             // 显示错误弹窗

private slots:
    // 固定倍率按钮点击（文档0x20~0x28指令）
    void on_btn_07x_clicked();
    void on_btn_10x_clicked();
    void on_btn_15x_clicked();
    void on_btn_20x_clicked();
    void on_btn_25x_clicked();
    void on_btn_30x_clicked();
    void on_btn_35x_clicked();
    void on_btn_40x_clicked();
    void on_btn_45x_clicked();
    // 普通变倍按钮（按下/松开，文档0x06/0x07指令）
    void on_btn_zoomOut_pressed();
    void on_btn_zoomOut_released();
    void on_btn_zoomIn_pressed();
    void on_btn_zoomIn_released();
    // 微调按钮（按下/松开，文档0x05/0x08指令）
    void on_btn_fineTuneIn_pressed();
    void on_btn_fineTuneIn_released();
    void on_btn_fineTuneInPlus_pressed();
    void on_btn_fineTuneInPlus_released();
    // void on_btn_fineTuneOut_pressed();
    // void on_btn_fineTuneOut_released();

private:
    Ui::ZoomControlWidget *ui;
    ZoomController *m_zoomController;

    void initSerialPortComboBox();
    void connectSignals();
    QString getSelectedSerialPort() const;
};

#endif // ZOOMCONTROLWIDGET_H
