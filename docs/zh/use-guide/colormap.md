# 科学 Colormap 预设

`QwtColorMapPreset`（`qwt::core` 模块）提供 22 种知名科学 colormap 预设，涵盖感知均匀色带（viridis、plasma…）、MATLAB 经典色带（jet、hot、cool…）、彩虹色带（rainbow、hsv、turbo）与发散型色带（coolwarm、rdylbu…），名称与 matplotlib / MATLAB 保持一致，可以零成本替换既有代码中的色带。2D 光谱图、3D 表面图与颜色条（color bar）共用同一套预设，颜色条的刻度与图案颜色永远由同一个 colormap 决定。

## 主要功能特性

- ✅ **22 种预设**：与 matplotlib / MATLAB 同名的科学色带，按名字创建，无需关心色标定义
- ✅ **感知均匀**：viridis、plasma、inferno、magma、cividis 亮度单调，色盲友好、打印安全
- ✅ **发散型色带**：coolwarm、rdylbu、rdylgn、spectral，适合含正负号的物理量
- ✅ **随处可用**：2D 光谱图、3D 表面、坐标轴颜色条、以及直接取色都使用同一工厂函数
- ✅ **可程序化查询**：可列出全部预设名、取色标节点、按名字/枚举互转
- ✅ **配套示例**：`examples/2D/colormap` 可交互切换对比，并支持 `--screenshot` 批量导出效果图

## 基本使用

### 1. 按名字创建 colormap

```cpp
#include <qwt_colormap.h>
#include <qwt_colormap_preset.h>

// 按名字创建（名字不区分大小写）
std::unique_ptr< QwtLinearColorMap > cmap = QwtColorMapPreset::create("viridis");
```

`create()` 返回的是 `QwtLinearColorMap` 智能指针，其父类 `QwtColorMap` 提供 `rgb(vMin, vMax, value)` 取色接口。全部预设名可通过 `availablePresets()` 查询：

```cpp
for (const QString& name : QwtColorMapPreset::availablePresets())
    qInfo() << name;   // viridis plasma inferno magma cividis jet hot cool
                       // spring summer autumn winter gray bone copper rainbow
                       // hsv turbo coolwarm rdylbu rdylgn spectral
```

### 2. 在 2D 光谱图中使用

光谱图的图像与右侧坐标轴的颜色条需要分别设置 colormap（两者独立持有实例）：

```cpp
QwtPlotSpectrogram* spectrogram = new QwtPlotSpectrogram();
spectrogram->setColorMap(QwtColorMapPreset::create("plasma").release());
spectrogram->attach(plot);

// 右侧坐标轴作为颜色条，与光谱图共用同一个 colormap
QwtScaleWidget* rightAxis = plot->axisWidget(QwtAxis::YRight);
rightAxis->setTitle("Intensity");
rightAxis->setColorBarEnabled(true);
rightAxis->setColorMap(dataInterval, QwtColorMapPreset::create("plasma").release());
plot->setAxisScale(QwtAxis::YRight, dataInterval.minValue(), dataInterval.maxValue());
plot->setAxisVisible(QwtAxis::YRight);
```

QWT7.3 之前的代码里曾使用 `QwtColorMap::rgb(interval, value)` 形式，现已统一为 `rgb(double vMin, double vMax, double value)`。

### 3. 直接对数据取色

不绘图、只想取色时（例如自定义渲染、报表着色），直接调用 `rgb()`：

```cpp
const std::unique_ptr< QwtLinearColorMap > cmap = QwtColorMapPreset::create("inferno");
const QRgb color = cmap->rgb(0.0, 100.0, 42.0);   // 值 42 映射到 [0,100] 区间的颜色
```

### 4. 在 3D 表面图中使用

3D 模块通过 `Qwt3DColorMapColor` 适配器桥接 core 的 colormap，因此 22 种预设可以直接用于曲面：

```cpp
Qwt3DTheme theme(Qwt3DTheme::Scientific);
theme.setDataColorPreset("plasma");   // 内部即通过 QwtColorMapPreset 创建色带
theme.apply(plot);
```

### 5. 其他查询接口

```cpp
// 枚举与名字互转
QwtColorMapPreset::Preset preset  = QwtColorMapPreset::Viridis;
QString name = QwtColorMapPreset::presetName(preset);          // "viridis"
QwtColorMapPreset::Preset from = QwtColorMapPreset::presetFromName("TURBO");

// 取色标节点（位置, 颜色），可用于自绘颜色条
QVector< QPair< double, QColor > > stops = QwtColorMapPreset::colorStops(QwtColorMapPreset::Jet);
```

## 运行示例

示例位于：`examples/2D/colormap`，界面如下：

![colormap 示例界面](../../assets/screenshots/colormap.png)

界面功能：

- **左侧列表**：22 种预设，条目前缀小图标即该色带的渐变外观，点击即切换；
- **Backdrop**：亮色 / 暗色背景切换，用于观察色带在不同底色下的观感；
- **Color Table**：颜色表大小（Smooth / 256 / 1024 / 16384），可观察色带量化产生的色阶（banding）；
- **Contours**：叠加等高线，便于判断等值区域；
- **Save Screenshots...**：选择目录后批量导出全部效果图。

效果图全部渲染在经典 MATLAB `peaks` 演示场上（一个尖峰旁带深谷的同心波纹场，归一化到 [0, 1]），右侧颜色条显示数值到颜色的映射。

### 命令行参数：--screenshot

示例支持一个命令行参数 `--screenshot <目录>`，可在无界面环境下把全部预设的效果图导出到指定目录，退出码为 0 表示成功：

```bash
./colormap --screenshot ./colormap-shots
```

导出内容：

| 文件 | 说明 |
|------|------|
| `<预设名>.png` × 22 | 每种预设一张效果图（1280×800，含数据场、颜色条、坐标轴） |
| `overview.png` | 22 种色带渐变的 2×11 总览图 |
| `colormap.png` | 示例程序界面截图 |

该模式复用界面的窗口系统渲染，导出的文字（标题、刻度、颜色条标签）与窗口中看到的一致；在没有显示器的环境（如无头 Linux CI）中会自动回退到 `offscreen` 平台——此时图片仍可生成，但文字会退化成方框。显式设置 `QT_QPA_PLATFORM` 环境变量可覆盖此行为。GUI 上的 `Save Screenshots...` 按钮与之等价。

## 22 种预设效果总览

下图为 22 种预设的渐变外观（每行由 `create("name")` 采样绘制，顺序与 `availablePresets()` 一致）：

![22 种 colormap 预设总览](../../assets/screenshots/colormap/overview.png)

| 名称 | 类别 | 特点 | 适用场景 |
|------|------|------|----------|
| `viridis` | 顺序·感知均匀 | 紫-绿-黄，matplotlib 默认 | 通用首选，论文图表 |
| `plasma` | 顺序·感知均匀 | 紫红-粉-亮黄 | 强调高值区域 |
| `inferno` | 顺序·感知均匀 | 黑-紫-橙-亮黄 | 深色背景 / 发光效果 |
| `magma` | 顺序·感知均匀 | 黑-紫-粉-近白 | 天文、暗背景数据 |
| `cividis` | 顺序·感知均匀 | 蓝-灰-黄 | 色觉障碍友好、灰度打印 |
| `jet` | 顺序·彩虹（MATLAB） | 蓝-青-黄-红 | 兼容旧代码，不推荐新项目 |
| `hot` | 顺序·热力 | 黑-红-黄-白 | 温度场、热力图 |
| `cool` | 顺序 | 青-品红 | 数学曲面示意 |
| `spring` | 顺序 | 品红-黄 | 数学曲面示意 |
| `summer` | 顺序 | 绿系 | 数学曲面示意 |
| `autumn` | 顺序 | 红-黄 | 数学曲面示意 |
| `winter` | 顺序 | 蓝-绿 | 数学曲面示意 |
| `gray` | 顺序·无色相 | 黑-白灰度 | 打印、灰度文档 |
| `bone` | 顺序·无色相 | 黑-灰-白带蓝调 | 医学影像风格 |
| `copper` | 顺序·单色相 | 黑-铜褐 | 地质、金属质感 |
| `rainbow` | 循环·彩虹 | 全色相环 | 周期性物理量（相位、角度） |
| `hsv` | 循环·HSV | 全 HSV 色相环 | 周期数据的最大值等于最小值 |
| `turbo` | 顺序·彩虹改进 | Google turbo，比 jet 均匀 | 需要彩虹观感又要可读性 |
| `coolwarm` | 发散 | 蓝-白-红 | 含正负号的差值场（距平、误差） |
| `rdylbu` | 发散 | 红-黄-蓝（ColorBrewer） | 距平、海拔填色 |
| `rdylgn` | 发散 | 红-黄-绿（ColorBrewer） | 增减方向明确的比率场 |
| `spectral` | 发散·多色相 | 红-黄-绿-蓝 | 定性区间也易分辨 |

下面的分类对比图由示例 `--screenshot` 导出，每张图的数据场完全相同，只有色带不同，方便横向比较。

### 感知均匀型

`viridis`、`plasma`、`inferno`、`magma`、`cividis` 的感知亮度单调变化：转灰度后仍是均匀渐变，不会出现"看起来一样深其实差很多"的伪结构，是科研论文的首选。

| viridis | plasma |
|---------|--------|
| ![viridis](../../assets/screenshots/colormap/viridis.png) | ![plasma](../../assets/screenshots/colormap/plasma.png) |

| inferno | magma |
|---------|-------|
| ![inferno](../../assets/screenshots/colormap/inferno.png) | ![magma](../../assets/screenshots/colormap/magma.png) |

| cividis |
|---------|
| ![cividis](../../assets/screenshots/colormap/cividis.png) |

### MATLAB 经典与单色相型

`jet` 是最广为人知的 MATLAB 默认色带，可以看到它在青-黄过渡处引入了一条肉眼对比较强的假等值线，且灰度化后明暗不再单调——这正是新代码应改用 turbo 或 viridis 的原因。`hot`、`cool`、`spring`、`summer`、`autumn`、`winter`、`gray`、`bone`、`copper` 为传统单色相 / 双色相色带，胜在简单直观。

| jet | hot | gray |
|-----|-----|------|
| ![jet](../../assets/screenshots/colormap/jet.png) | ![hot](../../assets/screenshots/colormap/hot.png) | ![gray](../../assets/screenshots/colormap/gray.png) |

| cool | bone | copper |
|------|------|--------|
| ![cool](../../assets/screenshots/colormap/cool.png) | ![bone](../../assets/screenshots/colormap/bone.png) | ![copper](../../assets/screenshots/colormap/copper.png) |

| spring | summer | autumn | winter |
|--------|--------|--------|--------|
| ![spring](../../assets/screenshots/colormap/spring.png) | ![summer](../../assets/screenshots/colormap/summer.png) | ![autumn](../../assets/screenshots/colormap/autumn.png) | ![winter](../../assets/screenshots/colormap/winter.png) |

### 彩虹系

`rainbow` 与 `hsv` 用于周期性数据时首尾颜色相同，可以无缝衔接；`turbo` 是 Google 给出的均衡彩虹色带，比 `jet` 的感知线性度好得多，适合必须使用彩虹观感的场合。

| rainbow | hsv | turbo |
|---------|-----|-------|
| ![rainbow](../../assets/screenshots/colormap/rainbow.png) | ![hsv](../../assets/screenshots/colormap/hsv.png) | ![turbo](../../assets/screenshots/colormap/turbo.png) |

### 发散型

数据存在"中心值 / 零值 / 中心偏离"时（距平、误差、正负号），使用双端强调、中心浅色的发散色带，可以把"偏离方向"编码成颜色方向。

| coolwarm | rdylbu | rdylgn | spectral |
|---------|--------|--------|----------|
| ![coolwarm](../../assets/screenshots/colormap/coolwarm.png) | ![rdylbu](../../assets/screenshots/colormap/rdylbu.png) | ![rdylgn](../../assets/screenshots/colormap/rdylgn.png) | ![spectral](../../assets/screenshots/colormap/spectral.png) |

## 选择与避坑

- **顺序型数据用感知均匀色带**：缺省选 `viridis`；想要更醒目的暖色渐变用 `plasma`；暗色背景用 `inferno` / `magma`。
- **有色相方向的语义时用发散型**：例如正负误差用 `coolwarm`，海平面距平用 `rdylbu`。
- **周期性数据用循环色带**：`rainbow` / `hsv` 首尾同色，注意把数据范围设置为一个完整周期，否则红-红相接处会出现假边界。
- **避免 `jet` / `rainbow` 表达顺序数据**：彩虹色带的感知亮度非单调，会在黄-青过渡处制造并不存在的等值线，灰度打印后尤其明显。
- **打印与灰度场景**：选 `cividis` 或 `gray`，转灰度后值的大小顺序依然可读。
- **色觉障碍友好**：避开红-绿同时表达大小关系的组合，`viridis`、`cividis`、`coolwarm` 为安全选项。

!!! tip "快速自检办法"
    把图表另存为灰度图（或将 `Color Table` 切到 `gray` 预设）再观察一遍：如果灰度变化与数据大小不再一致，说明该色带正在误导读者。

## 注意事项

- `setColorMap()` 转移所有权：传入的 `QwtColorMap*` 由 item / scale widget 持有并负责释放，因此用 `release()` 交出智能指针所有权即可，**不要**把同一个指针同时设置给光谱图和颜色条——示例中为两者分别 `create()` 了一次。
- `QwtColorMap::rgb()` 的形式是 `rgb(double vMin, double vMax, double value)`，在 QWT7.3.1 起统一；早期的 `rgb(const QwtInterval&, double)` 已移除。
- 预设创建的是 `QwtLinearColorMap`，默认 `RGB` 格式（连续插值）；若追求极致的渲染性能，可以用 `setColorTableSize(256)` 之类的量化色表，代价是可能出现色阶（banding），示例的 `Color Table` 组合框可以直接观察该效果。

## 相关类/源文件

| 类 / 文件 | 说明 |
|-----------|------|
| `QwtColorMapPreset` (`src/core/qwt_colormap_preset.h`) | 22 种预设的工厂与查询接口 |
| `QwtLinearColorMap` (`src/core/qwt_colormap.h`) | 线性渐变 colormap 实现 |
| `QwtPlotSpectrogram` (`src/plot/qwt_plot_spectrogram.h`) | 2D 光谱图 / 热力图 |
| `QwtScaleWidget::setColorMap()` (`src/plot/qwt_scale_widget.h`) | 坐标轴颜色条 |
| `Qwt3DColorMapColor` (`src/plot3d/qwt3d_colormap_color.h`) | 3D 表面 colormap 桥接 |
| `Qwt3DTheme::setDataColorPreset()` (`src/plot3d/qwt3d_theme.h`) | 3D 主题中指定预设 |

!!! example "相关示例"
    - colormap 预设对比：`examples/2D/colormap`（支持 `--screenshot` 导出）
    - 光谱图综合示例：`examples/2D/spectrogram`
    - 极坐标光谱图：`examples/2D/polarspectrogram`
    - 3D 表面图：`examples/3D/figureSurface3D`
