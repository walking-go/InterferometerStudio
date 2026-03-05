#include "uddm_namespace.h"
#include "alg.h"
#include "qdatetime.h"
#include <QFileDialog>
#include <QWidget>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include <QFile>
#include <QTextStream>

#include <objbase.h>
#include <ShObjIdl.h>
#include <ShlDisp.h>
#include <QVariant>
#include <ActiveQt/qaxbase.h>
#include<QApplication>
#pragma comment(lib, "Ole32.lib")
#include <exdisp.h>

// 路径相关
// ===========================================================
QString readIniConfig()
{
    // 从.ini配置文件读取路径
    QString ini_file_path = QApplication::applicationDirPath() + "/config.ini";
    QSettings settings(ini_file_path, QSettings::IniFormat);
    QString data_path = settings.value("Settings/DataPath").toString().replace("/", "\\");

    return data_path;
}

void writeIniConfig(QString& path_)
{
    // 从.ini配置文件读取路径
    QString ini_file_path = QApplication::applicationDirPath() + "/config.ini";
    QSettings settings(ini_file_path, QSettings::IniFormat);
    QString data_path = path_.replace("\\", "/").replace("//", "/");
    settings.setValue("Settings/DataPath", data_path);
}

/**
 * @brief initFilePath
 * 初始化文件路径, 默认在项目路径下新建以当天日期命名(yyyyMMdd)的文件夹
 */
bool initFilePath(QString& path_)
{
    // 读取.ini文件中配置的路径
    QString data_path = readIniConfig();

    // 创建项目路径
    QDir pro_dir;
    pro_dir.mkpath(data_path);

    // 当前当日路径
    QString path = data_path; // + QDateTime::currentDateTime().toString("yyyyMMdd") + '\\';
    QDir dir(path);
    if(!dir.exists())
        if(!dir.mkdir(path))
            return false;
    // 赋值
    path_ = path;
    return true;
}


/**
 * @brief saveSpotImg
 * 保存光斑图
 */
bool saveSpotImg(QString path_, const QImage &spot_img_)
{
    // 路径检查与创建
    if(!checkAndCreateFolder(path_, true))
        return false;

    // 检查光斑图
    if(spot_img_.isNull())
        return false;

    // 保存光斑图
    if(spot_img_.save(path_))
        return true;
    else
        return false;
}

/**
 * @brief saveTopo2D
 * 保存地形图
 */
bool saveTopo2D(QString path_, const QImage &topo2d_)
{
    // 路径检查与创建
    if(!checkAndCreateFolder(path_, true))
        return false;

    // 检查光斑图
    if(topo2d_.isNull())
        return false;

    // 保存光斑图
    if(topo2d_.scaled(QSize(800, 800), Qt::KeepAspectRatio, Qt::SmoothTransformation).save(path_))
        return true;
    else
        return false;
}

/**
 * @brief saveResultImages
 * 保存测量结果图像（原始图像和2D地形图）
 */
void saveResultImages(const QString& savePath, const QImage& originalImg, const QImage& topo2d)
{
    if (savePath.isEmpty()) {
        return;
    }

    // 保存原始图像（光斑图）
    QString spotImgPath = savePath + "/original_img.png";
    if (saveSpotImg(spotImgPath, originalImg)) {
        qDebug() << "[saveResultImages] 保存原始图像成功:" << spotImgPath;
    } else {
        qDebug() << "[saveResultImages] 保存原始图像失败:" << spotImgPath;
    }

    // 保存2D地形图
    QString topo2dPath = savePath + "/topo2d.png";
    if (saveTopo2D(topo2dPath, topo2d)) {
        qDebug() << "[saveResultImages] 保存2D地形图成功:" << topo2dPath;
    } else {
        qDebug() << "[saveResultImages] 保存2D地形图失败:" << topo2dPath;
    }

    qDebug() << "[saveResultImages] 图像保存完成，路径:" << savePath;
}

/**
 * @brief savePVRMSData
 * 保存PV/RMS数据到TXT文件
 * @param savePath 保存路径（会话文件夹路径）
 * @param pv PV（峰谷值）
 * @param rms RMS（均方根值）
 * @return 保存成功返回true，否则返回false
 */
bool savePVRMSData(const QString& savePath, double pv, double rms)
{
    if (savePath.isEmpty()) {
        return false;
    }

    // 构建PV_RMS.txt文件路径（与captured_images同级）
    QString filePath = savePath + "/PV_RMS.txt";

    // 检查并创建文件夹
    if (!checkAndCreateFolder(filePath, true)) {
        qDebug() << "[savePVRMSData] 创建文件夹失败:" << filePath;
        return false;
    }

    // 打开文件进行写入
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "[savePVRMSData] 打开文件失败:" << filePath;
        return false;
    }

    // 写入数据
    QTextStream out(&file);
    out.setCodec("UTF-8");  // 设置编码为UTF-8

    // 写入时间戳
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    out << "测量时间: " << timestamp << "\n";
    out << "========================================\n";

    // 写入PV和RMS数据
    out << QString("PV (峰谷值): %1 um\n").arg(pv, 0, 'f', 6);
    out << QString("RMS (均方根值): %1 um\n").arg(rms, 0, 'f', 6);

    file.close();

    qDebug() << "[savePVRMSData] 保存PV/RMS数据成功:" << filePath
             << " PV=" << pv << " RMS=" << rms;

    return true;
}

/**
 * @brief checkAndCreateFolder
 * 检查文件路径所在的文件夹是否存在，并可选择是否自动创建文件夹。
 * @param path_ 文件路径
 * @param auto_create_ 是否在文件夹不存在时自动创建，默认为true
 * @return 如果文件夹存在或创建成功，返回true；否则返回false
 */
bool checkAndCreateFolder(const QString& path_, bool auto_create_)
{
    // 获取文件所在的文件夹路径
    QFileInfo file_info(path_);
    QString folder_path = file_info.absolutePath();

    // 使用QDir检查文件夹是否存在
    QDir dir(folder_path);
    if (!dir.exists())
        // 如果文件夹不存在，并且设置了自动创建
        if (auto_create_) {
            // 创建文件夹
            bool success = dir.mkpath(folder_path);
            // 返回创建文件夹的成功与否
            return success;
        }
        else
            // 文件夹不存在，且不创建
            return false;
    else
        // 文件夹已存在
        return true;
}


// 图像相关
// ===========================================================
/**
 * @brief qImageToMat
 * 将QImage转换成cv::Mat
 * deep_copy_为true时深拷贝，false时浅拷贝
 */
cv::Mat qImageToMat(const QImage& img_, bool deep_copy_)
{
    // 先进行格式转换，此时数据为浅拷贝
    cv::Mat mat;
    switch(img_.format()){
    case QImage::Format_Grayscale8:
        mat = cv::Mat(img_.height(), img_.width(), CV_8UC1, (void*)img_.bits(), img_.bytesPerLine());
        break;
    case QImage::Format_Grayscale16:
        mat = cv::Mat(img_.height(), img_.width(), CV_16UC1, (void*)img_.bits(), img_.bytesPerLine());
        break;
    case QImage::Format_RGB888:
        mat = cv::Mat(img_.height(), img_.width(), CV_8UC3, (void*)img_.bits(), img_.bytesPerLine());
        break;
    case QImage::Format_RGB32:
    case QImage::Format_RGBA8888:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(img_.height(), img_.width(), CV_8UC4, (void*)img_.bits(), img_.bytesPerLine());
    default:
        break;
    }

    // 深拷贝判断
    if(deep_copy_)
        return mat.clone();
    else
        return mat;
};

/**
 * @brief matToQImage
 * 将cv::Mat转换成QImage
 * deep_copy_为true时深拷贝，false时浅拷贝
 * 当输入的cv::Mat为浮点类型时，输出的QImage为Grayscale16类型，此时不支持数据浅拷贝
 */
QImage matToQImage(const cv::Mat mat_, bool deep_copy_)
{
    QImage img_;
    switch(mat_.type()){
    case CV_8UC1:
        img_ = QImage((uchar*) mat_.ptr(0), mat_.cols, mat_.rows, mat_.cols, QImage::Format_Grayscale8);
        break;
    case CV_16UC1:
        img_ = QImage((uchar*) mat_.ptr(0), mat_.cols, mat_.rows, mat_.cols * 2, QImage::Format_Grayscale16);
        break;
    case CV_8UC3:
        img_ = QImage((uchar*) mat_.ptr(0), mat_.cols, mat_.rows, mat_.cols * 3, QImage::Format_BGR888);
        break;
    case CV_8UC4:
        img_ = QImage((uchar*) mat_.ptr(0), mat_.cols, mat_.rows, mat_.cols * 4, QImage::Format_RGBA8888);
        break;
    // 浮点类型的数据只支持深拷贝
    case CV_32FC1:
    case CV_64FC1:
        assert(deep_copy_ == true);
        cv::Mat mat = stretch(mat_);
        img_ = QImage((uchar*) mat.ptr(0), mat.cols, mat.rows, mat.cols * 2, QImage::Format_Grayscale16).copy();
    }

    if(deep_copy_)
        return img_.copy();
    else
        return img_;
};

/**
 * @brief pseudoColor
 * 对单通道cv::Mat做伪颜色处理，色图为cv::COLORMAP_PARULA
 */
cv::Mat pseudoColor(const cv::Mat mat_)
{
    assert(mat_.channels() == 1);
    // 转换成浮点型
    cv::Mat mat;
    mat_.convertTo(mat, CV_64FC1);
    // 找最大、最小值并拉伸
    double min_value, max_value;
    cv::minMaxLoc(mat, &min_value, &max_value);
    mat -= min_value;
    cv::minMaxLoc(mat, &min_value, &max_value);
    mat *= 1 / max_value;
    // 转换成CV_8UC1
    mat.convertTo(mat, CV_8UC1, 255);
    cv::applyColorMap(mat, mat, cv::COLORMAP_PARULA);

    return mat;
};

/**
 * @brief pseudoColor
 * 对单通道QImage做伪颜色处理，色图为cv::COLORMAP_PARULA
 *
 * 必须在最后进行一次数据深拷贝，因为mat只在中间过程存在，
 * 当所有mat都被析构后，opencv会自动清理数据内存
 */
QImage pseudoColor(const QImage& img_)
{
    cv::Mat mat = qImageToMat(img_);
    mat = pseudoColor(mat);
    QImage img = matToQImage(mat);
    return img.copy();
};

/**
 * @brief stretch
 * 将单通道QImage转换为Format_Grayscale16型并拉伸
 * 拉伸：将img_中的数值做线性变换，最小值映射至0，最大值映射至65535
 *
 * 必须在最后进行一次数据深拷贝，因为mat只在中间过程存在，
 * 当所有mat都被析构后，opencv会自动清理数据内存
 */
QImage stretch(const QImage& img_)
{
    cv::Mat mat = qImageToMat(img_);
    assert(mat.channels() == 1);
    cv::Mat stretched_mat = stretch(mat);
    return matToQImage(stretched_mat).copy();
};

/**
 * @brief stretch
 * 将单通道的cv::Mat转换为CV_16UC1型并拉伸
 * 拉伸：将mat_中的数值做线性变换，最小值映射至0，最大值映射至65535
 */
cv::Mat stretch(const cv::Mat &mat_)
{
    cv::Mat mat;
    // 转换成浮点型
    mat_.convertTo(mat, CV_64FC1);
    // 找最大、最小值并拉伸
    double min_value, max_value;
    cv::minMaxLoc(mat, &min_value, &max_value);
    mat -= min_value;
    cv::minMaxLoc(mat, &min_value, &max_value);
    mat *= 1 / max_value;
    // 转换成CV_16UC1
    mat.convertTo(mat, CV_16UC1, 65535);

    return mat;
}

/**
 * @brief processTopo2D
 * 获得地形数据用于显示的图像，通过mode_设置显示风格
 */
QImage processTopo2D(const QImage &img_, Topo2DStyle mode_)
{
    switch(mode_){
    case Grayscale:
        return stretch(img_);
    case PseudoColor:
        return pseudoColor(img_);
    }
}

/**
 * @brief preProcessLegendTag
 * 预处理颜色图例，产生灰阶图和伪颜色图，避免重复计算
 * 预处理的图像存在全局变量g_grayscale_legend和g_pseudocolor_legend中
 */
cv::Mat g_grayscale_legend = cv::Mat();
cv::Mat g_pseudocolor_legend = cv::Mat();
void preProcessLegendTag()
{
    // 如果预处理图例已经生成过，函数直接返回
    if(!g_grayscale_legend.empty() || !g_pseudocolor_legend.empty())
        return;

    // 创建300x50的单通道灰阶图
    g_grayscale_legend = cv::Mat(300, 50, CV_8UC1);
    double slope = 255.0 / (g_grayscale_legend.rows - 1);
    for (int i = 0; i < g_grayscale_legend.rows; i++)
        g_grayscale_legend.row(g_grayscale_legend.rows - 1 - i) = (i * slope);

    // 创建300x50的三通道伪颜色图
    applyColorMap(g_grayscale_legend, g_pseudocolor_legend, cv::COLORMAP_PARULA);

    // 将二者转化为RGBA格式
    cv::cvtColor(g_grayscale_legend, g_grayscale_legend, cv::COLOR_GRAY2RGBA);
    cv::cvtColor(g_pseudocolor_legend, g_pseudocolor_legend, cv::COLOR_BGR2RGBA);
}

/**
 * @brief prcoessLegendTag
 * 获得图例用来显示地形显示图像中颜色与高度的对应关系
 */
QImage prcoessLegendTag(Topo2DStyle mode_, double z_scope_, double aspect_ratio_)
{
    // 利用预处理数据，避免灰阶和伪颜色图的重复计算
    // ====================================
    preProcessLegendTag();
    cv::Mat legend_img;
    if(mode_ == Topo2DStyle::Grayscale)
        legend_img = g_grayscale_legend;
    else if(mode_ == Topo2DStyle::PseudoColor)
        legend_img = g_pseudocolor_legend;

    // 创建透明背景图像
    // =============
    // 1000*aspect_ratio_ x 1000的4通道
    int bg_height = 1000;
    int bg_width = bg_height * aspect_ratio_;
    cv::Mat background(bg_height, bg_width, CV_8UC4, cv::Scalar(0, 0, 0, 0));

    // 将图例放置在透明背景图像的左上角附近
    // ==============================
    int offset_x = 50;
    int offset_y = 50;
    cv::Rect roi(offset_x, offset_y, legend_img.cols, legend_img.rows);
    cv::Mat img_roi = background(roi);
    legend_img.copyTo(img_roi);
    // 绘制边框
    int bounder_thickness = 1;
    cv::rectangle(background, roi, cv::Scalar(0, 0, 0, 255), bounder_thickness);

    // 书写文字
    // =======
    int font = cv::FONT_HERSHEY_SIMPLEX;
    double font_scale = 1;
    cv::Scalar font_color = cv::Scalar(255, 255, 255, 255);
    cv::Scalar font_bg_color = cv::Scalar(0, 0, 0, 255);
    int font_thickness = 2;
    int delta_x = legend_img.cols + offset_x + 10;
    int delta_y = legend_img.rows + offset_y;
    // 在伪彩色图像右下角位置外侧写0.00
    cv::Point text_origin(delta_x, delta_y);
    cv::String text = "0.00";
    cv::putText(background, text, text_origin + cv::Point(2, 2), font, font_scale, font_bg_color, font_thickness);
    cv::putText(background, text, text_origin, font, font_scale, font_color, font_thickness, cv::LINE_AA);
    // 在伪彩色图像右上角位置外侧写z_scope,并保留3位小数
    text_origin = cv::Point(delta_x, delta_y - legend_img.rows + font_scale*20);
    text = QString::number(z_scope_, 'f', 2).toStdString() + "um";
    cv::putText(background, text, text_origin + cv::Point(2, 2), font, font_scale, font_bg_color, font_thickness);
    cv::putText(background, text, text_origin, font, font_scale, font_color, font_thickness, cv::LINE_AA);

    return matToQImage(background, true);
}


