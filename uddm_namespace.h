#ifndef UDDM_NAMESPACE_H
#define UDDM_NAMESPACE_H

#include <opencv2/opencv.hpp>
#include <QWidget>
#include <QImage>
#include <QMutex>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WINSOCKAPI_
#define byte win_byte
#include <Windows.h>
#undef byte

// 文件相关
// ===========================================================
QString readIniConfig();
void writeIniConfig(QString& path_);
bool initFilePath(QString& path_);
struct Target;
// class SHSConfig;
// class SHSResult;
//bool openFolderInExplorer(QString dir_);
enum class SavedObject{
    SpotImg,
    Topo2D,
    SHSResult,
    All
};

//bool saveResultFiles(QString path_, const Target target_, const SHSConfig config_, const SHSResult shs_result_, QString img_suffix_);

bool saveSpotImg(QString path_, const QImage& spot_img_);
bool saveTopo2D(QString path_, const QImage& topo2d_);
void saveResultImages(const QString& savePath, const QImage& originalImg, const QImage& topo2d);
bool savePVRMSData(const QString& savePath, double pv, double rms);

//bool saveSHSResult(QString path_, const SHSConfig& shs_config_, const SHSResult& shs_result_);

bool checkAndCreateFolder(const QString& path_, bool auto_create_);
// 保存相关
struct SaveInfoManager{
    bool auto_save = false;
    bool save_to_default_path = true;
    QString default_path = "";
    QString current_dir = "";
    QString file_prefix = "";
    QString img_suffix = ".png";
    QString csv_suffix = ".csv";
};

// 界面相关
// ===========================================================
/**
 * @brief The LeftIdx enum
 * 左区页号
 */
enum LeftIdx{
    BroswerScene = 0,
    Process2DScene,
    Process3DScene,
    ZernikeScene
};

/**
 * @brief The RightIdx enum
 * 右区页号
 */
enum RightIdx{
    File = 0,
    HardWare,
    Measurement,
    Process2D,
    Process3D
};

// 模式相关
// ===========================================================
enum SoftwareMode{
    Simulate = 0,
    Real
};

enum MeasurementMode{
    Continuous = 0,
    Single
};

// 目标相关
// ===========================================================
const double MAX_X_SCOPE = 1000.0;
const double MAX_Y_SCOPE = 1000.0;
const double MIN_X_SCOPE = 1.0;
const double MIN_Y_SCOPE = 1.0;
const double DEFAULT_X_SCOPE = 100.0;
const double DEFAULT_Y_SCOPE = 100.0;
const double SPSILON = 1e-6;
/**
 * @brief The Topo2DStyle enum
 * 地形图显示风格
 */
enum Topo2DStyle
{
    Grayscale = 0,  /**< 黑白*/
    PseudoColor     /**< 伪颜色*/
};

/**
 * @brief Target 目标相关数据结构体
 */
struct Target
{
    // 默认构造函数
    Target() = default;
    // 拷贝构造函数
    Target(const Target& target_){
        original_img = target_.original_img.copy();
        topo = target_.topo.copy();
        topo2d = target_.topo2d.copy();
        topo2d_legend = target_.topo2d_legend.copy();
        topo2d_style = target_.topo2d_style;
        x_scope = target_.x_scope;
        y_scope = target_.y_scope;
        z_scope = target_.z_scope;
        real_per_pix = target_.real_per_pix;
        real_per_gray_level = target_.real_per_gray_level;
    };
    // 深拷贝函数
    Target copy() const{
        return Target(*this);
    }
    // 是否为空
    bool isNull() const{
        return original_img.isNull();
    };

    QImage original_img;                /**< 原始图像, Format_Grayscale8*/
    QImage topo;                        /**< 点云数据, Format_Grayscale16
                                  灰度值表示该点Z方向高度值，
                                  灰度值范围：0 - 65535，其中65535对应z_scope*/
    QImage topo2d;                      /**< 对点云数据进行2D显示的图像*/
    QImage topo2d_legend;               /**< topo2d图例,用来显示颜色与Z轴位置的对应关系*/
    Topo2DStyle topo2d_style;           /**< topo2d显示风格*/
    double x_scope = DEFAULT_X_SCOPE;   /**< X方向视野范围*/
    double y_scope = DEFAULT_Y_SCOPE;   /**< Y方向视野范围*/
    double z_scope = 0.0;               /**< Z方向实际最大高度差*/
    double real_per_pix = 1.0;          /**< 图片像素与实际距离的比例,单位是微米*/
    double real_per_gray_level = 0.0;   /**< topo中灰度值与实际Z方向位置的比例,单位时微米*/
};

// 数值计算相关
// ===========================================================
const double PI = 3.14159265358979323846;
/**
 *  @brief 将浮点数num_与0比较
 *  容许的误差范围为epsilon，默认值为1e-6
 *  接近0返回0，大于0返回1，小于0返回-1
 */
template <typename T>
int compareWithZero(const T& num_, const T& epsilon_ = SPSILON)
{
    if(num_ - epsilon_ > 0){
        return 1;
    }
    else if(num_ + epsilon_ < 0){
        return -1;
    }
    else
        return 0;
};

/**
 *  @brief 将浮点数num_1与num_2比较
 *  容许的误差范围为epsilon，默认值为1e-6
 *  接近返回true, 否则返回false
 */
template <typename T>
bool closeToEachOther(const T& num_1_, const T& num_2_, const T& epsilon_ = SPSILON)
{
    if(abs(num_1_ - num_2_) < epsilon_)
        return true;
    else
        return false;
};

/**
 *  @brief 判断浮点数num_是否接近0
 *  容许的误差范围为epsilon，默认值为1e-6
 *  接近0返回true, 否则返回false
 */
template <typename T>
bool closeToZero(const T& num_, const T& epsilon_ = SPSILON)
{
    if(abs(num_) < epsilon_)
        return true;
    else
        return false;
};

/**
 * @brief 计算一个数的整数位数
 */
template <typename T>
int numOfIntergerDigit(const T& num_)
{
    int num = floor(num_);
    int count = 0;
    while(num != 0){
        count ++;
        num /= 10;
    }
    return count;
};

// 图像相关
// ===========================================================
// QImage转cv::Mat
cv::Mat qImageToMat(const QImage& img_, bool deep_copy_ = false);
// cv::Mat转QImage
QImage matToQImage(const cv::Mat mat_, bool deep_copy_ = false);
// 对cv::Mat做伪颜色处理
cv::Mat pseudoColor(const cv::Mat mat_);
// 对QImage做伪颜色处理
QImage pseudoColor(const QImage& img_);
// 将单通道的QImage转换为Format_Grayscale16型并拉伸
// 拉伸：将img_中的数值做线性变换，最小值映射至0，最大值映射至65535
QImage stretch(const QImage& img_);
// 将单通道的cv::Mat转换为CV_16UC1型并拉伸
// 拉伸：将mat_中的数值做线性变换，最小值映射至0，最大值映射至65535
cv::Mat stretch(const cv::Mat& mat_);
// 获得地形数据用于显示的图像
QImage processTopo2D(const QImage& img_, Topo2DStyle mode_);
// 预处理颜色图例，产生灰阶图和伪颜色图，避免重复计算
void preProcessLegendTag();
// 获得图例用来显示地形显示图像中颜色与高度的对应关系
QImage prcoessLegendTag(Topo2DStyle mode_, double z_scope_, double aspect_ratio_ = 1);

#endif // UDDM_NAMESPACE_H

