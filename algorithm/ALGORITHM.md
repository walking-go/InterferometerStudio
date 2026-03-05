# APDA 算法文档

本文档记录 InterferometerStudio 项目中干涉仪相位测量算法的实现细节。

---

## 算法概述

APDA (Adaptive Phase Detection Algorithm) 是一种用于干涉图相位提取的自适应算法，主要用于处理带有移相误差的干涉图序列。

### 处理流程

```
输入图像序列 → 预处理 → 条纹归一化 → 初值估计 → 图像筛选 → NPDA迭代优化 → 后处理 → 输出相位图
```

---

## 文件结构

| 文件 | 功能 |
|------|------|
| `APDAProcessor.cpp/h` | 算法入口，负责图像预处理和调用核心算法 |
| `APDA.cpp/h` | APDA 核心算法实现 |
| `NPDA.cpp/h` | 非线性相位检测算法（迭代优化） |
| `deltaEstmationNPDA.cpp/h` | 倾斜移相系数初值估计 |
| `fringePatternNormalization.cpp/h` | 条纹图归一化处理 |
| `PhaseProcessor.cpp/h` | 相位后处理（解包裹、去倾斜、Zernike拟合） |
| `PhaseUnwrapper.cpp/h` | 相位解包裹实现 |
| `czt2.cpp/h` | 2D Chirp-Z 变换 |
| `utils.cpp/h` | 工具函数（文件加载、mask创建、显示等） |
| `alg.cpp/h` | 其他辅助算法 |

---

## 核心数据结构

### CoeffsMDPD
倾斜移相系数：
```cpp
struct CoeffsMDPD {
    std::vector<double> kxs;  // X方向倾斜系数
    std::vector<double> kys;  // Y方向倾斜系数
    std::vector<double> ds;   // 相位偏移量
};
```

### NPDAResult
NPDA算法输出结果：
```cpp
struct NPDAResult {
    cv::Mat estPhase;           // 估计的相位
    cv::Mat estA;               // 背景项
    cv::Mat estB;               // 调制度
    std::vector<double> estKxs; // X方向倾斜系数
    std::vector<double> estKys; // Y方向倾斜系数
    std::vector<double> estDs;  // 相位偏移量
    std::vector<double> historyEK;  // 迭代历史
    std::vector<double> historyEIt;
    int nIters;                 // 迭代次数
};
```

### DeltaEstimationResult
初值估计结果：
```cpp
struct DeltaEstimationResult {
    std::vector<double> estKxs;     // X方向倾斜系数
    std::vector<double> estKys;     // Y方向倾斜系数
    std::vector<double> estDs;      // 相位偏移量
    std::vector<double> resErrs;    // 残差误差（用于图像筛选）
    std::vector<cv::Mat> estDeltas; // 估计的相位差
    std::vector<cv::Mat> estPhase;  // 估计的相位
};
```

### NormalizationResult
归一化结果：
```cpp
struct NormalizationResult {
    std::vector<cv::Mat> normalizedIs;  // 归一化后的图像
    double ampFactor;                    // 幅度因子
    cv::Mat estA;                        // 背景估计
    cv::Mat estB;                        // 调制度估计
    cv::Mat outputMask;                  // 输出mask
};
```

---

## API 说明

### APDAProcessor

算法处理器类，提供两种调用方式：

```cpp
class APDAProcessor {
public:
    // 从文件加载图像并处理
    cv::Mat run(const std::string& capturedPath, const std::string& fileType, int nCapture);

    // 从内存图像列表处理（推荐）
    cv::Mat run(const std::vector<cv::Mat>& capturedImages);
};
```

**参数说明：**
- `capturedPath`: 图像文件夹路径
- `fileType`: 文件类型，如 `"*.png"`
- `nCapture`: 图像数量
- `capturedImages`: 内存中的图像列表（CV_8UC1 或 CV_8UC3）

**返回值：**
- `cv::Mat`: 最终相位图（CV_32FC1），单位为弧度

### APDA

核心算法函数：

```cpp
cv::Mat APDA(
    const std::vector<cv::Mat>& noisedIs,  // 预处理后的图像（CV_32F，值范围[0,1]）
    const cv::Mat& mask,                    // 圆形mask
    double maxTiltFactor                    // 最大倾斜因子
);
```

---

## 关键参数

### APDAProcessor 参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `resizeFactor` | 0.5 | 图像缩放因子 |
| `maxTiltFactor` | 10 | 最大倾斜因子 |
| ROI | 512x512 | 感兴趣区域大小 |

### APDA 参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `resThresh` | 0.4 | 残差阈值，用于筛选有效图像 |
| `estSize` | 256 | 初值估计使用的图像尺寸 |

---

## 输入要求

### 图像格式
- **类型**: CV_8UC1（灰度图）或 CV_8UC3（彩色图，会自动转灰度）
- **尺寸**: 最小 64x64 像素，推荐 512x512 或更大
- **数量**: 建议 20-50 张移相图像

### 图像预处理流程
```
输入 CV_8UC1 [0, 255]
    ↓
ROI 裁剪 (默认 512x512)
    ↓
转换为 CV_32F [0.0, 1.0]
    ↓
缩放到目标尺寸 (resizeFactor=0.5 → 256x256)
    ↓
应用圆形 mask
```

---

## 调试输出

当前版本包含详细的调试输出，可在控制台查看：

### MeasurementWorker
```
[MeasurementWorker] 首张输入图像 - 类型: X (CV_8UC1=0), 通道数: X, 值范围: [X, X]
```

### APDAProcessor
```
[APDAProcessor] 输入图像尺寸: WxH, 类型: X, 通道数: X
[APDAProcessor] ROI: (X,Y) WxH, 缩放后尺寸: WxH
[APDAProcessor] 首张预处理图像 - 类型: X (CV_32F=5), 值范围: [X, X] (期望: [0, 1])
```

### APDA
```
[APDA] 有效图像数量: X / Y (resThresh=0.4)
[APDA] resErr 统计 (前10个): X X X ...
[APDA] 保留: X, 筛除: Y
[APDA] 最终相位图 - 尺寸: WxH, 类型: X, 值范围: [X, X], 均值: X
```

---

## 常见问题

### 1. 结果异常 - 有效图像数量过少
**现象**: `[APDA] 警告：有效图像数量过少 (X)，可能导致结果异常！`

**原因**: 大部分图像的 resErr 超过阈值 (0.4)

**解决方案**:
- 检查输入图像质量
- 确认图像为正确的移相干涉图序列
- 检查图像预处理是否正确（值范围应为 [0, 1]）

### 2. 图像值范围异常
**现象**: `[APDAProcessor] 警告：图像值范围异常！期望 [0,1]`

**原因**: 图像转换或缩放过程中出现问题

**解决方案**:
- 确认输入图像为 8 位灰度图
- 检查 `convertTo` 的缩放因子是否为 `1/255.0`

### 3. 最终相位图包含 NaN
**现象**: `[APDA] 警告：最终相位图包含 NaN 值！`

**原因**: 数值计算溢出或除零错误

**解决方案**:
- 检查有效图像数量是否大于 3
- 检查 mask 是否正确

### 4. 相位图为常量
**现象**: `[APDA] 警告：最终相位图值范围过小，可能是常量图像！`

**原因**: 输入图像无有效干涉条纹或算法收敛到错误解

**解决方案**:
- 确认输入为有效的干涉图像
- 检查相机曝光和增益设置

---

## 与 demo 版本的差异

| 项目 | demo 版本 | algorithm 版本 |
|------|-----------|----------------|
| ROI 位置 | 固定 (1, 1) | 固定 (1, 1) |
| resThresh | 0.4 | 0.4 |
| 输入来源 | 文件加载 | 内存传入 |
| 调试输出 | 少量 | 详细 |

---

## 性能计时

算法各阶段的执行时间会输出到控制台：
```
--- fringePatternNormalization function time: Xms ---
--- deltaEstmation function time: Xms ---
--- NPDA function time: Xms ---
--- Processor function time: Xms ---
```

---

## 更新记录

| 日期 | 变更内容 |
|------|----------|
| 2024-12-18 | 添加详细调试输出 |
| 2024-12-18 | 统一 resThresh 为 0.4 |
| 2024-12-18 | 创建本文档 |
