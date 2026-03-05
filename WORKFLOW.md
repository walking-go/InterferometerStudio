# 算法更新工作流

## 背景

Software 文件夹是从 GitHub 仓库 `walking-go/InterferometerStudio` 克隆的本地副本。项目由实验室成员协作开发（QT + C++），本人负责其中 `algorithm/` 子目录的算法更新。

## 仓库信息

- **远程仓库**：https://github.com/walking-go/InterferometerStudio
- **本地路径**：`Engineering/MotorPSI/Software/`
- **算法目录**：`algorithm/`（本人维护）
- **外部依赖**：`external_dependencies/` 中的 `opencv_world470.dll` 和 `opencv_world470d.dll` 因体积过大被 `.gitignore` 排除，需通过 U 盘等方式离线分发（已存放于 `Temp/external_dependencies/` 备份）

## 新环境配置

克隆仓库后，需要完成以下配置才能编译运行。

### 1. 安装依赖

| 依赖 | 版本 | 说明 |
|------|------|------|
| Eigen | 5.0.0 | 纯头文件库，下载解压即可 |
| OpenCV | 4.7.0 | 下载 [opencv-4.7.0-windows.exe](https://github.com/opencv/opencv/releases/download/4.7.0/opencv-4.7.0-windows.exe) 自解压 |

### 2. 创建本地配置文件

项目使用 `local_config.pri` 管理因电脑而异的路径（Eigen、OpenCV 安装位置），该文件被 `.gitignore` 排除，不会提交到 Git，因此不会与其他成员冲突。

```bash
cp local_config.pri.example local_config.pri
```

编辑 `local_config.pri`，填入本地路径：

```ini
# Eigen 路径（包含 Eigen/ 子目录的那一层）
EIGEN_PATH = D:/Applications/eigen-5.0.0

# OpenCV 路径（build 目录）
OPENCV_PATH = D:/Applications/opencv_4.7.0/build
```

`.pro` 文件通过 `include(local_config.pri)` 读取 `$$EIGEN_PATH` 和 `$$OPENCV_PATH`，无需修改 `.pro` 本身。

### 3. 补充离线 DLL

将以下两个文件拷贝到 `external_dependencies/`（因超过 GitHub 100MB 限制，需离线分发）：

- `opencv_world470.dll`（61 MB）
- `opencv_world470d.dll`（120 MB）

备份位置：`Temp/external_dependencies/`

---

## 工作流程

### 1. 更新前：拉取最新代码

```bash
cd Engineering/MotorPSI/Software
git pull origin main
```

确保本地代码与远程同步，避免冲突。

### 2. 修改算法

在 `algorithm/` 目录下进行修改。主要文件：

| 文件                                 | 职责        |
| ---------------------------------- | --------- |
| `APDAProcessor.cpp/h`              | 算法入口      |
| `APDA.cpp/h`                       | APDA 核心   |
| `NPDA.cpp/h`                       | NPDA 迭代优化 |
| `deltaEstmationNPDA.cpp/h`         | 初值估计      |
| `czt2.cpp/h`                       | CZT 变换    |
| `fringePatternNormalization.cpp/h` | 条纹归一化     |
| `PhaseProcessor.cpp/h`             | 相位后处理     |
| `PhaseUnwrapper.cpp/h`             | 相位解包裹     |

修改时注意：

- 算法接口（`ALGORITHM.md` 中记录的数据结构和 API）如有变更需同步更新文档
- C++ 实现需与 MATLAB 原型 (`Code/algorithm/`) 保持逻辑一致

### 3. 本地验证

在 QT 环境中编译并测试，确认算法功能正确。

### 4. 提交并推送

```bash
git add algorithm/
git commit -m "描述修改内容"
git push origin main
```

### 5. 通知团队

推送后通知协作成员拉取更新。

## 注意事项

- **只修改 `algorithm/` 目录**：其余模块（相机控制、电机控制、UI 等）由其他成员负责
- **DLL 依赖离线分发**：`opencv_world470.dll`、`opencv_world470d.dll` 不在 Git 中管理，新环境部署时需手动拷贝到 `external_dependencies/`
- **此 Git 仓库独立于外层项目仓库**：Software 文件夹有自己的 `.git`，与 `ClaudeCode Management` 的 Git 仓库互不干扰
