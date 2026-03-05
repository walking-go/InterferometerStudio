QT       += core gui
QT       += core gui widgets serialport  # 引入必要模块（串口+图形界面）
QT       += datavisualization
QT       += charts
QT       += concurrent  # 异步任务支持
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += utf8
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    algorithm/APDA.cpp \
    algorithm/APDAProcessor.cpp \
    algorithm/NPDA.cpp \
    algorithm/PhaseProcessor.cpp \
    algorithm/PhaseUnwrapper.cpp \
    algorithm/czt2.cpp \
    algorithm/deltaEstmationNPDA.cpp \
    algorithm/fringePatternNormalization.cpp \
    algorithm/utils.cpp \
    basler_camera_controller.cpp \
    basler_camera_thread.cpp \
    basler_exposuretime_widget.cpp \
    camera_controller.cpp \
    camera_switcher.cpp \
    image_cropper.cpp \
    interfe_namespace.cpp \
    main.cpp \
    main_window.cpp \
    measurement_control_widget.cpp \
    measurement_worker.cpp \
    roi_selection_dialog.cpp \
    motor_control_widget.cpp \
    motor_controller.cpp \
    motor_serial.cpp \
    page_process2d.cpp \
    page_process3d.cpp \
    rotator_control_widget.cpp \
    rotator_controller.cpp \
    small_camera_thread.cpp \
    uddm_namespace.cpp \
    zoom_control_widget.cpp \
    zoom_controller.cpp \
    utils/ag_element.cpp \
    utils/curve_chart.cpp \
    utils/dialog_color_and_linewidth.cpp \
    utils/drawer.cpp \
    utils/drawing_funs.cpp \
    utils/element.cpp \
    utils/element_canvas.cpp \
    utils/img_viewer.cpp \
    utils/measure2d_results_dialog.cpp \
    utils/mg_element.cpp \
    utils/surface_area.cpp

HEADERS += \
    algorithm/APDA.h \
    algorithm/APDAProcessor.h \
    algorithm/NPDA.h \
    algorithm/PhaseProcessor.h \
    algorithm/PhaseUnwrapper.h \
    algorithm/alg.h \
    algorithm/czt2.h \
    algorithm/deltaEstmationNPDA.h \
    algorithm/fringePatternNormalization.h \
    algorithm/utils.h \
    basler_camera_controller.h \
    basler_camera_thread.h \
    basler_exposuretime_widget.h \
    camera_controller.h \
    camera_switcher.h \
    image_cropper.h \
    interfe_namespace.h \
    main_window.h \
    measurement_control_widget.h \
    measurement_worker.h \
    roi_selection_dialog.h \
    motor_control_widget.h \
    motor_controller.h \
    motor_serial.h \
    page_process2d.h \
    page_process3d.h \
    rotator_control_widget.h \
    rotator_controller.h \
    small_camera_thread.h \
    uddm_namespace.h \
    zoom_control_widget.h \
    zoom_controller.h \
    utils/ag_element.h \
    utils/canvas_namespace.h \
    utils/curve_chart.h \
    utils/dialog_color_and_linewidth.h \
    utils/drawer.h \
    utils/drawing_funs.h \
    utils/element.h \
    utils/element_canvas.h \
    utils/img_viewer.h \
    utils/measure2d_results_dialog.h \
    utils/mg_element.h \
    utils/surface_area.h

FORMS += \
    basler_camera_exposuretime.ui \
    measurement_control.ui \
    motor_control.ui \
    page_process2d.ui \
    page_process3d.ui \
    rotator_control.ui \
    zoom_control.ui \
    utils/dialog_color_and_linewidth.ui \
    utils/measure2d_results_dialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# basler相机
LIBS += -L$$PWD/external_dependencies/camera_basler/lib/x64
INCLUDEPATH += $$PWD/external_dependencies/camera_basler/include
DEPENDPATH += $$PWD/external_dependencies/camera_basler/include

#eigen
INCLUDEPATH += D:/you_need/eigen-5.0.0/eigen-5.0.0

# OpenCV配置
OPENCV_PATH = D:/you_need/opencv/opencv/build

INCLUDEPATH += $$OPENCV_PATH/include
# 根据编译模式链接对应的 OpenCV 库
CONFIG(release, debug|release) {
    # Release 模式：链接不带 d 后缀的库
    LIBS += $$OPENCV_PATH/x64/vc16/lib/opencv_world470.lib
} else {
    # Debug 模式：链接带 d 后缀的库（调试版本）
    LIBS += $$OPENCV_PATH/x64/vc16/lib/opencv_world470d.lib
}

# 仅对 MSVC 生效
win32-msvc* {
    # 强制编译器使用 UTF-8 编码（解决 C4819 警告）
    QMAKE_CXXFLAGS += /utf-8
    DEFINES += UNICODE
    # 可选：禁止 C4819 编码警告（若文件已转 UTF-8 仍报错时启用）
    QMAKE_CXXFLAGS += /wd"4819"
}

RESOURCES += \
    resources.qrc
