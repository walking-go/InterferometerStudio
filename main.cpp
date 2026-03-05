#include <QApplication>
#include "main_window.h" // 包含主窗口头文件
#include "algorithm/alg.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 注册APDAResult类型，使其可以在跨线程的信号槽中使用
    qRegisterMetaType<APDAResult>("APDAResult");

    MainWindow w; // 实例化主窗口
    w.show();
    return a.exec();
}
