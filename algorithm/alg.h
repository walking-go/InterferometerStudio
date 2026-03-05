#ifndef ALG_H
#define ALG_H

#include <QDebug>
#include <QMetaType>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>

#include <uddm_namespace.h>
#include <QFile>
#include <QList>
// #include <vector>
#include <QVector>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QBitArray>
#include <QApplication>

// /**
//  * @brief The WavefrontAlg enum
//  * 波前重构使用的算法类型
//  */
// enum WavefrontAlg{
//     Zernike,
//     Iteration
// };

/**
 * @brief The APDAResult class
 * APDA算法运算结果
 */
struct APDAResult{
    // 拷贝构造函数
    APDAResult() = default;
    APDAResult(const APDAResult& result_) = default;
    // 深拷贝函数
    APDAResult copy() const{
        APDAResult shs_result;
        shs_result.original_img = this->original_img.copy();
        shs_result.wavefront = this->wavefront.clone();
        shs_result.real_per_pix = this->real_per_pix;
        shs_result.max_slope = this->max_slope;
        shs_result.n_x = this->n_x;
        shs_result.n_y = this->n_y;

        shs_result.wavefront_pv = this->wavefront_pv;
        shs_result.wavefront_rms = this->wavefront_rms;
        // shs_result.sphere_pv = this->sphere_pv;
        // shs_result.sphere_rms = this->sphere_rms;

        shs_result.is_valid = this->is_valid;
        shs_result.save_path = this->save_path;
        return shs_result;
    };
    QImage original_img;
    cv::Mat wavefront;
    double real_per_pix;

    //QVector<double> zernike_coef;
    double wavefront_pv;
    double wavefront_rms;
    // double sphere_pv;
    // double sphere_rms;
    double max_slope;
    int n_x;
    int n_y;

    // WavefrontAlg method;

    bool is_valid = false;

    // 保存路径（会话文件夹路径，用于保存2D图像）
    QString save_path;
};

// 注册APDAResult类型，使其可以在跨线程的信号槽中使用
Q_DECLARE_METATYPE(APDAResult)

#endif // ALG_H
