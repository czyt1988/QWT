## tag:v7.3.5 (2026-07-24)

### 破坏性变更

- **3D模块架构重构 — Plot + Item 模式**
    - 将 3D 模块从旧的 widget-per-plot 模式完全重构为 **Plot + Item** 架构，与 2D 模块（`QwtPlot` + `QwtPlotItem`）对称：
        - `Qwt3DPlot`（QOpenGLWidget）：纯渲染窗口，管理 GL 上下文、视图变换、光照、坐标系统和 item 列表。不再持有绘图数据。
        - `Qwt3DPlotItem`（新增）：所有 3D 绘图 item 的抽象基类（attach/detach/draw/hull/z/title），与 `QwtPlotItem` 对称。
        - `Qwt3DSurface`：曲面 item，使用 VBO/VAO + GLSL 3.3 Core 着色器渲染。合并了旧 `gridplot.cpp` 和 `meshplot.cpp` 的逻辑。
    - 移除 `namespace Qwt3D` — 所有类使用 `Qwt3D` 前缀定义在全局作用域（如 `Qwt3DPlot`、`Qwt3DSurface`、`Qwt3DFunction`）。
    - 重命名所有 3D 类：`Plot3D` → `Qwt3DPlot`，`SurfacePlot` → `Qwt3DSurface`，`Function` → `Qwt3DFunction`，`CoordinateSystem` → `Qwt3DCoordinateSystem`，`Axis` → `Qwt3DAxis`，`ColorLegend` → `Qwt3DColorLegend`，`Color` → `Qwt3DColor`，`StandardColor` → `Qwt3DStandardColor`，`ColorMapColor` → `Qwt3DColorMapColor`，`Qwt3DTheme`（已有前缀，仅去命名空间），`Mapping` → `Qwt3DMapping`，`GridMapping` → `Qwt3DGridMapping`，`ParametricSurface` → `Qwt3DParametricSurface`，`Scale` → `Qwt3DScale`，`AutoScaler` → `Qwt3DAutoScaler`，`Enrichment` → `Qwt3DEnrichment`，`Drawable` → `Qwt3DDrawable`，`Label` → `Qwt3DLabel`。
    - 删除 stub 类：`GraphPlot`、`MultiPlot`、`VolumePlot` — 如需要后续作为 `Qwt3DPlotItem` 子类重新实现。
    - 绘图方法从 `Qwt3DPlot` 移至 `Qwt3DSurface`：`setPlotStyle()`、`setDataColor()`、`loadFromData()`、`setResolution()`、`setMeshColor()`、`addEnrichment()`、`setFloorStyle()`、`setShading()`、`showNormals()`。
    - 移除 `updateData()` — 改用 `plot->update()` 或 `item->itemChanged()`。
    - 移除 `setCoordinateStyle()` — 直接使用 `Qwt3DCoordinateSystem` API。
    - `Qwt3DFunction` 和 `Qwt3DParametricSurface` 的 target 从 `SurfacePlot*`（widget）改为 `Qwt3DSurface*`（item）。

- **3D模块：现代 OpenGL 迁移**
    - 将所有旧版 OpenGL 调用（立即模式、显示列表、固定管线、GLU 二次曲面）替换为现代 OpenGL：
        - VBO（`QOpenGLBuffer`）+ VAO（`QOpenGLVertexArrayObject`）管理顶点数据。
        - GLSL 3.30 Core 着色器用于所有渲染（surface、line、point、polygon、text）。
        - CPU 端 `QMatrix4x4` 矩阵计算（不使用 GL 矩阵栈）。
        - `QOpenGLTexture` 用于文本标签（替代 `glRasterPos3d` + `glDrawPixels`）。
        - CPU 生成的三角网格几何体用于 Cone/Arrow 装饰（替代 `gluCylinder`/`gluDisk`）。
    - 删除 `qwt3d_openglhelper.h`（旧版 GL 状态管理工具）。
    - gl2ps 矢量导出封装在 `QWT3D_ENABLE_GL2PS` 条件编译宏下（Compatibility Profile 回退）。
    - 新增 10 个 GLSL 着色器文件在 `src/plot3d/shaders/`（surface、polygon、line、point、text — vert+frag 配对）。

- **3D主题系统：Plot级/Item级拆分**
    - `Qwt3DTheme::apply()` 现在将 plot 级属性（背景、坐标颜色、标题、光照）应用到 `Qwt3DPlot`，将 item 级属性（meshColor、dataColorPreset、plotStyle、shading、smoothMesh）通过 `dynamic_cast` 应用到已挂载的 `Qwt3DSurface` item。

- **3D IO系统适配**
    - `Qwt3DNativeReader` 现在搜索 `plot->itemList()` 中的 `Qwt3DSurface` item（如不存在则创建），替代 `dynamic_cast<SurfacePlot*>(plot)`。
    - 重命名 IO 类：`IO` → `Qwt3DIO`，`PixmapWriter` → `Qwt3DPixmapWriter`，`VectorWriter` → `Qwt3DVectorWriter`，`NativeReader` → `Qwt3DNativeReader`。

- **3D示例更新**
    - `examples/3D/` 下全部 6 个示例更新为新的 Plot + Item API。
    - `SIGNAL()/SLOT()` 连接迁移为新式 `connect()` 语法。
    - 示例装饰中的旧版 GL 调用替换为 VBO + 着色器渲染。

## tag:v7.3.4 (2026-07-12)

### 新功能

- **全 2D 模块统一 NaN/Inf 处理**
    - 将 5 种混用的 NaN 检测方式（`qwt_is_nan_or_inf`、`!std::isfinite`、`qIsNaN`、`std::isnan`、本地位级 `qwtIsNaN`）统一为 `qwt_is_nan_or_inf()`，覆盖 `src/plot/` 和 `src/core/`
    - 将 `isSampleNanOrInf()` 从匿名命名空间提升为 `qwt_series_data.h` 中的公共导出 API，供所有绘图项使用
    - 为 10+ 个原先缺失 NaN 处理的绘图项添加 NaN/Inf 跳过：
        - `QwtPlotBarChart`、`QwtPlotHistogram`、`QwtPlotIntervalCurve`、`QwtPlotMultiBarChart`
        - `QwtPlotTradingCurve`、`QwtPlotVectorField`、`QwtPlotBoxChart`、`QwtPlotSpectroCurve`
        - `QwtPolarCurve`、`QwtPlotSpectrogram`（替换本地位级 `qwtIsNaN`）、`QwtPolarSpectrogram`
    - 补全 `QwtPlotCurve` 中 4 处缺失的 NaN 跳过路径
    - `QwtColorMap::rgb()` 和 `colorIndex()` 对 NaN/Inf 输入值返回透明色
    - `QwtBoxStatistics` 在排序和统计计算前预过滤 NaN/Inf
    - 新增双语 NaN 处理用户指南（`docs/use-guide/nan-handling.md`）

- **nanperf — NaN 性能基准测试示例**
    - 新增示例位于 `examples/2D/nanperf/`，用于基准测试 NaN 处理性能
    - `NanDataGenerator` 支持可配置的 NaN 分布模式，附带单元测试
    - `BenchmarkRunner` 提供计时框架，跨过滤模式和数据量进行分析
    - `MainWindow` 采用 2×2 网格布局、控制栏、扫描编排和 Markdown 报告导出
    - `FilterModes` 和 `PlotPanel` 组件用于并排对比

- **PySide6 绑定（POC）**
    - 初始 PySide6/Shiboken6 绑定，覆盖 `qwt::core` 类
    - 类型系统扩展至全部 core 模块类
    - 完整功能测试套件（`bindings/tests/test_core.py`）
    - CMake 集成，包含 `FindPySide6Shiboken.cmake` 和 `Shiboken6Tools`

- **QwtDate 现代化**
    - 将 `QwtDate` 内部实现从已弃用的 `Qt::TimeSpec` 迁移至 `QTimeZone`
    - 在 `qwt_qt5qt6_compat.hpp` 中新增 `qwt::compat::` 时区辅助函数，保持 Qt 5.12+/Qt6 双兼容
    - 更新 `qwt_date_scale_draw.cpp` 和 `qwt_date_scale_engine.cpp` 使用新 API

### Bug 修复

- 修复 `QwtVectorFieldSample` NaN 检查 — 原先检查 `vx` 两次，漏检 `vy`
- 修复 `QwtInterval::contains(NaN)` — 原先错误返回 `true`，现正确返回 `false`
- 修复 `qwtBoundingRect` 越界访问和 NaN 传播 bug
- 修复 `QwtLinearColorMap::colorIndex()` — 注释声称处理 NaN 但实现未处理
- 修复 `QwtGridData::findValueRange` NaN 传播和 `dyMin` 计算 bug
- 修复 `QwtPolarSpectrogram` Indexed 路径缺失 NaN 守卫

### 构建

- 确保 MinGW 下 UTF-8 窄字符串字面量正确（CMake `target_compile_definitions` 添加 `UNICODE`/`_UNICODE`）
- 新增 `qmakeExample` — 演示使用 qmake 进行单文件合并构建

### 文档

- 新增 qmake 单文件构建指南（中文，`docs/zh/build-guide/qmake-single-file.md`）
- 在双语 README 中添加 qmake 使用说明
- 修复曲线文档和头文件注释中错误的默认 `PaintAttribute` 描述
- 更新双语曲线文档中的降采样算法推荐说明

## tag:v7.3.2 (2026-06-20)

### 新功能

- **QwtColorMapPreset 应用于光谱图**
    - `QwtPlotSpectrogram` 和 `QwtPolarSpectrogram` 默认使用 Viridis 色彩映射替代旧的蓝-黄线性映射
    - 光谱图和极坐标光谱图示例新增预设切换 UI 控件
    - 经典色彩映射类型保留向后兼容

- **QwtPolarPlot 背景统一**
    - 统一 `QwtPolarPlot` 背景行为与 `QwtPlot` 一致

### 变更

- **3D 默认色彩映射迁移至 viridis**
    - `StandardColor::reset()` 从蓝-红渐变改为 viridis 预设
    - `Qwt3DTheme` 默认预设从 "jet" 改为 "viridis"，符合现代可视化标准
    - 刷新 3D 截图资源以反映新的默认色彩映射

### 文档

- **文档结构重组** — 扁平化目录层级，新增 40+ 篇用户指南
    - 覆盖曲线、图表、3D 绘图、交互工具和控件等全部主要功能
    - 新增开发者指南：编码规范、注释规范、PIMPL 模式用法
    - 更新 mkdocs.yml 导航，反映新的扁平结构和 i18n 文件夹组织
    - 统一截图和示例路径链接

## tag:v7.3.1 (2026-06-17)

### 新功能

- **qwt::core 共享库**
    - 提取 `qwt::core` 共享库（`qwtcore.dll`），包含颜色映射和调色板公共功能
    - `QwtColorMap` 重构：移除 `QwtInterval` 依赖，新签名 `rgb(vMin, vMax, value)`
    - `QwtColorCycle` 从 plot 模块迁移至 core
    - 新增 `QwtColorMapPreset` 类，提供 22 种科学可视化色彩映射预设（viridis、plasma、inferno、magma、cividis、jet、hot、cool、spring、summer、autumn、winter、gray、bone、copper、rainbow、hsv、turbo、coolwarm、rdylbu、rdylgn、spectral）
    - `src/plot/qwt_colormap_compat.h` 提供向后兼容的 `QwtColorMapCompat` 命名空间，包含基于 `QwtInterval` 的重载函数

- **3D 主题系统**
    - 新增 `Qwt3DTheme` 类，提供 10 种预设主题：Default、Dark、Scientific、Warm、Cool、Matplotlib、EarthTones、Ocean、HighContrast、Presentation
    - 新增 `ColorMapColor` 适配器，桥接 2D 色彩映射到 3D 颜色函数
    - 5 种光照预设：NoLighting、FlatLight、Studio、Outdoor、Soft
    - `Plot3D::setTheme()`、`Plot3D::applyTheme()` 一键应用主题
    - `StandardColor::setPreset()` 快速切换色彩映射预设

### 架构变更

- 三模块架构：`qwtcore.dll` → `qwtplot.dll` + `qwtplot3d.dll`
- `plot` 和 `plot3d` 都依赖 `core`，但彼此独立
- 运行时需要 `qwtcore.dll` + `qwtplot.dll`（+ `qwtplot3d.dll` 如果启用 3D）
- `src/plot/` 中保留向后兼容的转发头文件，支持现有 `#include "qwt_color_map.h"` 路径
- **Core 模块扩展**：将 24 个通用工具模块从 `plot` 迁移至 `core`，core 模块从 4 个模块（颜色 + 全局导出）扩展为包含 28 个模块的完整基础工具库：
    - 数学工具：`qwt_math.h/.cpp`、`qwt_simd_argminmax.h/.cpp`
    - 数据类型：`QwtInterval`、`QwtPoint3D`、`QwtPointPolar`、`QwtSamples`、`QwtBoxStatistics`
    - 几何算法：`QwtBezier`、`QwtClipper`
    - 坐标变换：`QwtTransform`、`QwtScaleMap`、`QwtScaleDiv`、`QwtScaleEngine`
    - 时间处理：`QwtDate`、`QwtSystemClock`
    - 通用算法与兼容层：`qwt_algorithm.hpp`、`qwt_qt5qt6_compat.hpp`
    - 数据容器：`QwtGridData`
    - 数据系列：`QwtSeriesData`、`QwtPointData`、`QwtSeriesStore`
    - 栅格数据：`QwtRasterData`、`QwtMatrixRasterData`、`QwtGridRasterData`

### 构建系统

- 新增 `src/core/CMakeLists.txt` 用于 core 库目标
- 更新 `src/plot/CMakeLists.txt` 链接 `qwt::core`
- 更新 `src/plot3d/CMakeLists.txt` 链接 `qwt::core` 并包含主题源文件
- 更新 amalgamate 工具模板以包含 core 模块文件
- 更新 `classincludes/` 头文件指向 core 模块路径
- 更新 `tools/make-classinclude.py` 适配新模块结构

### 示例

- **figureSurface3D** — 新增主题切换 UI 并修复主题切换时颜色图例的刷新问题

### Bug 修复

- 修复 3D 动画自动启动崩溃问题：将动画定时器延迟到 `showEvent()` 中启动，避免在控件可见前访问 GL 上下文
- 修复 `figureSurface3D` 切换主题时颜色图例不刷新的问题

## tag:v7.3.0 (2026-06-12)

### 新功能

- **QwtColorCycle 颜色循环系统**
    - 新增`QwtColorCycle`类，管理颜色调色板并支持循环使用，内置7种预设调色板及自定义调色板
    - 与`QwtPlot`集成，绘图项在`attach()`时自动从颜色循环中获取颜色
    - 支持的绘图项：`QwtPlotCurve`、`QwtPlotMultiBarChart`、`QwtPlotBoxChart`、`QwtPlotHistogram`、`QwtPlotIntervalCurve`、`QwtPlotBarChart`
    - 新增颜色循环测试用例

- **曲线降采样过滤器**
    - `QwtPlotCurve`新增`FilterPointsPixel`和`FilterPointsLTTB`两种绘制属性
        - `FilterPointsPixel`：基于像素列的降采样，适用于超大数据集的极速渲染
        - `FilterPointsLTTB`：简化版LTTB（MinMax桶）算法，在降采样同时更好地保留视觉形状
    - 默认绘制属性由`FilterPoints`改为`FilterPointsAggressive`
    - `qwtMapPointsQuad`优化NaN/Inf值跳过逻辑和多边形内存预分配

- **SIMD加速的MinMax桶降采样**
    - 新增`qwt_simd_argminmax`模块，提供SIMD加速的argmin/argmax计算
    - 支持SSE2/AVX2/NEON指令集，自动检测并选择最优指令集
    - 显著提升MinMax桶降采样的性能

- **Picker增强**
    - `QwtPlotSeriesDataPickerGroup`增加点击位置获取功能，支持在点击时获取屏幕坐标和数据坐标
    - `QwtPlotSeriesDataPicker`扩展支持所有`QwtPlotSeriesItem`子类型的数据拾取：
        - 交易曲线(K线图)、区间曲线、直方图、向量场
        - 柱状图、箱线图、光谱曲线
    - `FeaturePoint`新增`sampleData`字段（QVariant），可携带完整样本数据（OHLC、区间、向量等）
    - 通过`Q_DECLARE_METATYPE`注册样本类型，支持跨模块传递

- **扁平化风格控件**
    - `QwtSimpleCompassRose`新增`flatStyle`属性，启用后用纯色替代经典双色3D效果
    - `QwtWheel`新增`flatStyle`属性，启用后用扁平颜色和简单线条替代3D渐变
    - `QwtSlider`、`QwtThermo`新增`flatStyle`属性支持现代扁平化UI
    - `QwtDialNeedle`增加多种扁平化指针样式
    - `QwtKnob`默认使用`Flat`模式

### 性能优化

- `qwtProbeOrientation`和`qwtFindVisibleRange`优化采样策略，最多检查50个采样点并确保最小步长为1
- `qwtPixelColumnReduce`优化：预分配polyline向量、使用直接指针赋值替代动态追加
- bins初始化使用`QVector`构造函数简化

### 代码重构

- **PIMPL宏统一迁移**（涉及180+文件）
    - 所有类从手动PIMPL（裸指针`PrivateData* m_data`）迁移至标准宏模式
    - 头文件使用`QWT_DECLARE_PRIVATE(ClassName)`，实现使用`QWT_DECLARE_PUBLIC` + `QWT_PIMPL_CONSTRUCT` + `QWT_D/QWT_DC`
    - 使用`unique_ptr`自动管理生命周期，移除所有`delete m_data`
    - 覆盖绘图核心、坐标轴、文本/图例、颜色映射、画布、柱状图、交互类、数据类、样条、控件、极坐标等全部模块

- **3D绘图模块重构**（涉及53文件）
    - 添加`override`说明符到所有虚函数和析构函数
    - `nullptr`替代`0`作为空指针常量
    - `SurfacePlot`重构为PIMPL模式（`QWT_DECLARE_PRIVATE`）
    - `qwt3d_ptr`重命名为`ClonePtr`，统一include guards

- **现代C++特性**
    - 使用`auto`关键字和基于范围的for循环进行代码现代化
    - 添加移动语义和C++11默认成员初始化器
    - 为简单getter/setter和构造函数添加`noexcept`
    - 扩展`constexpr`到区间、坐标轴、刻度映射和数学工具函数
    - 跨`src/plot`和`src/plot3d`模块进行C++惯用法现代化

### 新增示例

- **renderbench** - 曲线渲染性能基准测试
    - 窗口化数据系列（`WindowedSeriesData`），支持实时数据流模拟
    - 控制面板可调整数据量、降采样模式、线宽等参数
    - 支持生成Markdown格式的性能报告
    - 位于`examples/bench/renderbench/`
- **staticExample** - 重写为Qwt综合特性展示示例
    - 演示曲线、频谱图、柱状图、箱线图、向量场等多种绘图类型
    - 统一更新所有2D示例的颜色调色板和画布背景风格

### 文档

- 新增曲线降采样算法完整文档（中文）
- 新增`QWEN.md` AI Agent指引文件，更新构建说明
- 添加英文文档并重构双语文档站点结构
- 统一执行英文注释规范，所有源码注释改为纯英文Doxygen格式
- 大批量Doxygen双语注释补全（40+批次），覆盖所有核心头文件

### Bug修复

- 修复Qt6下legend图标因`QwtNullPaintDevice::metric()`返回0（设备像素比）导致的空白问题
- 修复`QwtPlotMarker`箭头首尾端点反转问题，改进图例图标渲染
- 修复绘图背景渲染问题，改用`QPalette::Base`动态`backgroundRole`，确保主题切换时背景色正确

## tag:v7.2.1

- 新增刻度朝内显示功能
    - 新增`QwtPlot::TickDirection`枚举，支持`TickOutside`（刻度朝外，默认）和`TickInside`（刻度朝内）
    - 新增`QwtPlot::setAxisTickDirection()`和`QwtPlot::axisTickDirection()`方法，可独立控制每个坐标轴的刻度方向
    - 刻度朝内时，刻度线从画布边缘向内延伸，主干和标签保持在画布外
    - 朝内刻度与朝外刻度共享相同的样式设置（长度、颜色、线宽）
    - 新增示例程序：`examples/2D/ticks_inside/`
    ![ticks_inside](./docs/assets/screenshots/ticks_inside.png)

## tag:v7.2.0

- 新增箱线图(Box Chart)支持
    - 新增`QwtPlotBoxChart`类，用于绘制箱线图（Box-and-Whisker Plot）
    - 新增`QwtBoxStatisticsCalculator`类，用于自动计算统计量
    - 新增数据结构：`QwtBoxSample`、`QwtBoxOutlierSample`、`QwtBoxChartData`、`QwtBoxOutlierChartData`
    - 支持预计算数据和原始数据两种输入方式
    - 支持多种箱体样式：矩形(Rect)、菱形(Diamond)、缺口形(Notch)
    - 支持多种须须计算方法：Tukey(1.5×IQR)、百分位数、最小最大值、标准差、标准误
    - 支持垂直和水平两种显示方向
    - 支持异常值自动检测、自定义符号和抖动显示
    - 新增示例程序：`examples/2D/boxchart/`
	![Box Chart](./docs/assets/screenshots/BoxChart.png)

## tag:v7.0.8

- `QwtFigure`增加`addAxisAlignment`等坐标轴对齐功能，可以指定子绘图的坐标轴进行对齐
	坐标轴对齐的效果如下：
	![axis-alignment](./docs/assets/picture/figure-scale-aligment.png)

- `QwtPointMapper`添加对`NaN`和`Inf`值的异常处理，在数据存在异常值时不会导致坐标映射异常
- `qwt_series_data.cpp`的数据范围判断增加对`NaN`和`Inf`值的异常处理
- 解决合并文件漏了QwtSlider类的问题（Issue #4）

## tag:v7.0.7

- v7.0.5~v7.0.6新增的`QwtScaleWidget::panScale`函数移动到`QwtPlot`中，并改名为`panAxis`，并修复了在对数坐标轴移动异常的问题，解决对数坐标轴坐标移动问题
- 原来`QwtPanner`类改名为`QwtCachePanner`,代表带缓存的`Panner`。Qwt6.0的Panner为了避免频繁刷新使用了一个pixmap进行缓存，因此把这类Panner统称为`CachePanner`
	- 同步`QwtPlotPanner`类改名为`QwtPlotCachePanner`
	- 同步`QwtPolarPanner`类改名为`QwtPolarCachePanner` 
	
	`QwtPlotPanner`效果如下：
	![series-data-picker-yvalue](./docs/assets/screenshots/qwt-realtime-panner.gif)
	
- 重写`QwtPlotPanner`类，继承`QwtPicker`，可实现拖动过程能实时刷新，如果想要原来的带缓存的panner，可使用`CachePanner`相关类
- 新增`QwtCanvasPicker`针对canvas的picker操作都继承此类
- `QwtPlotPanner`支持线性坐标轴、对数坐标轴、多坐标轴的实时平移
- 针对`QwtPlotZoomer`只能绑定2个坐标轴问题，新增`QwtPlotCanvasZoomer`类
	- `QwtPlotCanvasZoomer`无需绑定坐标轴，直接对整个cavas进行缩放
	- `QwtPlotCanvasZoomer`支持多坐标轴的缩放
- 原`QwtPlotZoomer`更名为`QwtPlotAxisZoomer`
- `QwtPlotSeriesDataPicker`将直接继承`QwtPicker`，不再继承`QwtPlotPicker`
- `QwtPlotMagnifier`支持多坐标轴的缩放
- `QwtScaleMap`增加移动语义
- 新增`make-classinclude.py`,可以导出`classincludes`目录，使用方法见`make-classinclude.py`
--

v7.0.7对Qwt的命名进行了调整

|原名称|新名称|备注|
|:--|:--|:--|
|QwtPlotZoomer|QwtPlotAxisZoomer|由于原QwtPlotZoomer只能绑定2个坐标轴，故改名为QwtPlotAxisZoomer|
|新增|QwtPlotCanvasZoomer|针对整个画布缩放的zoomer|
|QwtPanner|QwtCachePanner|原Panner无法实时拖动，故名Cache Panner|
|QwtPlotPanner|QwtPlotCachePanner|原Panner无法实时拖动，故名Cache Panner|
|QwtPolarPanner|QwtPolarCachePanner|原Panner无法实时拖动，故名Cache Panner|
|新增|QwtPlotPanner|原QwtPlotPanner改名为QwtPlotCachePanner后新增QwtPlotPanner|

## tag:v7.0.6

- 新增`QwtPlotSeriesDataPicker`类，提供了绘图数据的拾取
    `QwtPlotSeriesDataPicker`效果如下：
    ![series-data-picker-yvalue](./docs/assets/picture/series-data-picker-yvalue.gif)
    ![series-data-picker-nearest-value](./docs/assets/picture/series-data-picker-nearest-value.gif)
- 完善了寄生绘图的刷新机制，不会在构造时无法完全更新
- 有些接口的索引类型由int改为size_t
- `QwtPlotSeriesDataPicker`支持日期坐标轴的正常显示
- 修正了`QwtFigureWidgetOverlay`的一些事件处理会和坐标轴动作冲突的问题

## tag:v7.0.5

- `QwtScaleWidget`增加坐标轴内置动作功能，实现坐标轴的pan和zoom两种内置动作
	- `QwtScaleWidget`增加`scaleRect`函数，可以获取坐标刻度所在矩形，也就是用来绘制刻度的区域
	- `QwtScaleWidget`增加`QwtScaleWidget::BuiltinActions`枚举，目前包含了两个内置动作
	- 增加`requestScaleRangeUpdate`信号，用于请求绘图刷新坐标轴范围
	- 增加`setSelected`/`isSelected`函数，让坐标轴可选中，增加信号：`selectionChanged`
	- 增加坐标轴动作相关的一些属性设置如：`zoomFactor`、`panScale`等函数
- 增加`QwtPlotScaleEventDispatcher`类，实现绘图上处理坐标轴的相关事件
- `QwtPlot`的寄生绘图相关接口进行了一定调整
- `QwtPLot`增加`setEnableScaleBuildinActions`/`isEnableScaleBuildinActions`/`setupScaleEventDispatcher`等函数用于控制坐标轴的缩放和平移
- `QwtPLot`增加`plotList`函数，能获取所有绘图对象，包括寄生绘图和宿主绘图

## tag:v7.0.4

- 实现寄生绘图功能
- 增加寄生绘图的例子
- `QwtPlot`添加`updateAxisEdgeMargin`函数，让寄生绘图可以自动调整坐标轴位置

## tag:v7.0.2

- 抽取出`QwtPlotLayoutEngine`类
- 增加`QwtPlotParasiteLayout`类，用于实现寄生轴的布局
- 增加`QwtPlotTransparentCanvas`类，用于实现一个完全透明的画布
- `QwtFigure`类实现寄生轴功能
- `QwtScaleWidget`添加了`setEdgeMargin`和`edgeMargin`方法，可以实现刻度和绘图边缘的距离设置
- 调整了`QwtScaleWidget`的布局方案，能支持`edgeMargin`
- 例子增加`parasitePlot`演示如何使用寄生轴
- `QwtPlot`增加`rescaleAxes`,`syncAxis`等方法，用于实现快速调整坐标轴
- 针对nan和inf值的异常处理

## tag:v7.0.1

- 增加`QwtFigure`类，`QwtFigure`用于管理多个QwtPlot，实现类似Matplotlib的Figure功能，支持网格化布局

`QwtFigure`类的效果如下：

![figure](docs/assets/screenshots/qwt_figure.png)

- example增加figure例子，用于演示`QwtFigure`的使用
- 新增`QwtFigureWidgetOverlay`，可以实现在`QwtFigure`上面进行一些动作，目前支持拖动绘图，缩放绘图的功能

## tag:v7.0.0

- 把整个工程合并为QwtPlot.h和QwtPlot.cpp，直接可以引入，文件位于src-amalgamate
- 例子增加staticExample
- 增强`QwtPlotBarChart`的接口以支持pen和brush的设置.
- 增加`QwtGridRasterData`类，相比`QwtMatrixRasterData`，它支持一个二维数据表，以及x,y轴进行网格插值
- `QwtLinearColorMap`增加stopColors函数，修改`QwtLinearColorMap`的`colorStop`函数为`stopPos`
- `Qwt`的初始化参数进行了调整，让默认绘图更符合当前绘图的审美

`QwtPlotCanvas`初始化如下：
```cpp
QwtPlotCanvas::QwtPlotCanvas(QwtPlot* plot) : QFrame(plot), QwtPlotAbstractCanvas(this)
{
    ...

    setLineWidth(0);
    setFrameShadow(QFrame::Plain);
    setFrameShape(QFrame::Box);
}
```

`QwtPlotLayout`初始化调整如下

```cpp
QwtPlotLayout::QwtPlotLayout()
{
...
    setCanvasMargin(-1);
...
}
```

- 去除QWT_MOC_INCLUDE
- 调整了一些类的实现，以便能合并到一个文件中

------

**Qwt 7.0** is my modified version based on Qwt 6.2.0 source code. It complies with Qwt's open-source license, and I'm releasing these modifications as open-source.

Starting from this version, Qwt will support:
- The latest **C++11 standard**
- **CMake build system**

**Modifications in this version:**
1. Enhanced `QwtPlotBarChart` interface to support **pen and brush configuration**  
2. Added `QwtGridRasterData` class – supports **2D data tables** with **grid interpolation** on X/Y axes (vs. `QwtMatrixRasterData`)  
3. Added `stopColors()` function to `QwtLinearColorMap` and renamed `colorStop()` to `stopPos()`  


> **Qwt7.0**是我基于Qwt6.2.0版本对源码进行修改后的版本，遵循Qwt的开源协议，我将修改进行开源
>
>从此版本的Qwt，将有如下改动:
>	
>- 支持最新的**C++11标准**
>- **支持CMake构建**
>
>下面是此版本的修改内容：
>- 增强`QwtPlotBarChart`的接口以支持pen和brush的设置.
>- 增加`QwtGridRasterData`类，相比`QwtMatrixRasterData`，它支持一个二维数据表，以及x,y轴进行网格插值
>- `QwtLinearColorMap`增加stopColors函数，修改`QwtLinearColorMap`的`colorStop`函数为`stopPos`



Qwt 6.2.0

------

0) Requirement for Qt >= 4.8

1) Class Includes added

Include files, that match the class names are available now. So
it is possible to write "#include <QwtPlot>" now instead of "include qwt_plot.h"

2) BSD License for examples

Where possible the code of the examples is available under the 3-clause BSD License 

3) MathML text renderer removed

The code can be found at https://github.com/uwerat/qwt-mml-dev now and is intended
to become a standalone lib. Anyone who is interested to workon it, please let me know.

4) Spline interpolation

The broken implementation of QwtSpline has been replaced by a bunch of classes
offering all sort of functionalities around splines.

The most popular spline approximation/interpolation algos have been implemented:

	- Basis
	- Cardinal
	- ParabolicBlending
	- Akima
	- The one used in MS Excel
	- Cubic

An implementation of the de Casteljau’s algorithm has been added

	- QwtBezier

5) New plot items

	- QwtPlotVectorField
      A new type of plot item for vector fields

    - QwtPlotGraphicItem
      An item displaying a QwtGraphic image ( f.e used by QwtPlotSvgItem )

6) Plot Canvas

	- QwtAbstractPlotCanvas introduced
	- QwtPlotOpenGLCanvas added to support QOpenGLWidget

7) QwtPlotCurve

	- QwtPlotCurve::FilterPointsAggressive mode added - a fast weeding algo
      for huge datasets with increasing x or y values

	- QwtPlotCurve::closestPoint is virtual now
	- QwtPlotCurve: polygon clipping includes the painter clip
	- QwtPlotCurve::setLegendAttributes added
	- QwtValuePointData added for curves, where the x values are the index
	- a couple of new QwtPlotCurve::setSamples alternatives

8) QwtPlotSpectrogram

	- QwtPlotSpectrogram::setColorTableSize added
	- QwtRasterData::setInterval/interval changed into a pure virtual getter
	- QwtMatrixRasterData::BicubicInterpolation added
	- QwtMatrixRasterData::interval: API cleanup
	- QwtHueColorMap, QwtSaturationValueColorMap added

9) QwtPlotRenderer

	- using QPdfWriter where possible

10)
	- LOG_MIN/LOG_MAX removed, use QwtTransform::LogMin/LogMax instead ( values differ ! )
	- qwt_compat.h removed
	- qwtFuzzyGreaterOrEqual/qwtFuzzyLessOrEqual removed
	- qwtGetMin/qwtGetMax removed

11)
    - Not aligning unknown paint engines ( f.e EMF )
    - QwtNullPaintDevice is using a different type than QPaintEngine::User now

12) Many other changes ...
