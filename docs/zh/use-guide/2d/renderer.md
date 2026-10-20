# 绘图导出 - QwtPlotRenderer

`QwtPlotRenderer` 提供将绘图导出为各种格式文件的功能，支持导出为图片文件（PNG、JPG等）、PDF、SVG矢量格式以及打印输出。

## 主要功能特性

- 支持导出 PNG、JPG、BMP、PDF、SVG、PS 等格式
- 可设置导出图片的 DPI 分辨率
- 可自定义导出尺寸或使用毫米单位
- 导出时可选择忽略标题、图例等组件

## 使用方法

### 1. 导出到图片文件

所有文件导出都经由 `QwtPlotRenderer::renderDocument()` 完成：传入 plot、目标文件名（由扩展名决定格式）、以毫米为单位的输出尺寸（`QSizeF`）以及以 DPI 为单位的分辨率（默认 85）。对于图片格式，像素尺寸按 `mm / 25.4 * DPI` 换算得到。下面的例子把同一个 plot 分别写成 PNG、JPG 和 BMP：

```cpp
#include <QwtPlotRenderer>

QwtPlotRenderer renderer;

// 导出到PNG：200 x 150 mm、300 DPI（约 2362 x 1772 像素）
renderer.renderDocument(plot, "plot.png", QSizeF(200, 150), 300);

// 导出到JPG：相同物理尺寸、150 DPI（约 1181 x 886 像素）
renderer.renderDocument(plot, "plot.jpg", QSizeF(200, 150), 150);

// 导出到BMP，使用默认分辨率 85 DPI
renderer.renderDocument(plot, "plot.bmp", QSizeF(200, 150));

// 文件格式自动根据扩展名判断
```

### 2. 导出到PDF

PDF 导出使用同一个 `renderDocument()` 入口，把文件扩展名换成 `.pdf` 即可。输出是矢量文档，任意缩放都保持清晰，适合打印与文档嵌入：

```cpp
// 导出到PDF
renderer.renderDocument(plot, "plot.pdf", QSizeF(200, 150), 300);  // 200x150 mm, 300 DPI

// PDF支持矢量图形，适合高分辨率打印
```

### 3. 导出到SVG

文件名以 `.svg` 结尾时，`renderDocument()` 生成可缩放的矢量图形（需要 Qwt 构建时启用 SVG 支持，即 `QWT_CONFIG_QWTSVG`）。SVG 输出保持可编辑，也能直接嵌入网页：

```cpp
// 导出到SVG矢量格式
renderer.renderDocument(plot, "plot.svg", QSizeF(200, 150));

// SVG优势：
// - 矢量图形，无损缩放
// - 文件较小
// - 可在浏览器查看
```

### 4. 导出到QImage/QPixmap

除了写文件，渲染器还能画进内存中的绘图设备：`renderTo(QwtPlot*, QPaintDevice&)` 重载接受任何自行按所需像素尺寸创建的 `QImage` 或 `QPixmap`，便于在应用内显示或做后续图像处理：

```cpp
// 导出为QImage：先创建图像，再渲染到其中
QSize size(800, 600);
QImage image(size, QImage::Format_ARGB32);
image.fill(Qt::white);
renderer.renderTo(plot, image);
image.save("plot.png");  // 可选：保存为文件

// 导出为QPixmap
QPixmap pixmap(size);
pixmap.fill(Qt::white);
renderer.renderTo(plot, pixmap);

// 可用于应用程序内显示或进一步处理
```

### 5. 导出选项

`DiscardFlag` 枚举用于在输出中剔除指定组件：`DiscardBackground`、`DiscardTitle`、`DiscardLegend`、`DiscardCanvasBackground`、`DiscardFooter` 与 `DiscardCanvasFrame`。单个标志用 `setDiscardFlag()` 切换，也可以用 `setDiscardFlags()` 一次性赋值整组：

```cpp
// 设置导出布局选项
renderer.setDiscardFlag(QwtPlotRenderer::DiscardTitle, true);     // 忽略标题
renderer.setDiscardFlag(QwtPlotRenderer::DiscardLegend, true);    // 忽略图例
renderer.setDiscardFlag(QwtPlotRenderer::DiscardFooter, true);    // 忽略页脚
renderer.setDiscardFlag(QwtPlotRenderer::DiscardCanvasBackground, true);  // 忽略画布背景

// 设置框架选项
renderer.setDiscardFlag(QwtPlotRenderer::DiscardCanvasFrame, true);  // 忽略画布边框

// 恢复默认：赋空标志集（渲染全部组件）
renderer.setDiscardFlags(QwtPlotRenderer::DiscardNone);
```

### 6. 打印功能

打印时把配置好的 `QPrinter` 交给渲染器（`renderTo(QwtPlot*, QPrinter&)`），由它在页面上排布 plot。打印目标由 printer 自身决定：可以是物理打印机，也可以如下例输出为 PDF 文件：

```cpp
#include <QPrinter>

// 打印到打印机（或通过 setOutputFileName 输出为 PDF 文件）
QPrinter printer(QPrinter::HighResolution);
printer.setOutputFileName("print.pdf");  // 不设置则输出到物理打印机

renderer.renderTo(plot, printer);

// 也可以用 exportTo()：弹出文件对话框并导出到所选文件
renderer.exportTo(plot, "plot.pdf");
```

### 7. 使用毫米单位设置尺寸

`renderDocument()` 的尺寸参数是 `QSizeF`，按毫米解释，因此导出文档具有确定的物理尺寸；`resolution` 参数（默认 85）设定栅格化内容所用的每英寸点数：

```cpp
// 使用毫米单位指定尺寸
QSizeF sizeMm(200, 150);  // 200mm x 150mm
renderer.renderDocument(plot, "plot.pdf", sizeMm, 300);  // 300 DPI
```

## 导出格式对比

| 格式 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| PNG | 无损压缩、支持透明 | 文件较大 | 网络分享、截图 |
| JPG | 文件小、广泛支持 | 有损压缩、无透明 | 照片类图表 |
| PDF | 矢量、高分辨率 | 需PDF阅读器 | 打印、文档 |
| SVG | 矢量、可编辑 | 复杂图形渲染慢 | 网页、编辑 |
| BMP | 无损、简单 | 文件很大 | 特定软件 |

## 导出选项枚举

| 选项 | 说明 |
|------|------|
| `DiscardBackground` | 忽略绘图背景 |
| `DiscardTitle` | 忽略标题 |
| `DiscardLegend` | 忽略图例 |
| `DiscardFooter` | 忽略页脚 |
| `DiscardCanvasBackground` | 忽略画布背景 |
| `DiscardCanvasFrame` | 忽略画布边框（使用样式表时无效，此时边框属于背景的一部分） |

## 核心方法总结

| 方法 | 说明 |
|------|------|
| `renderDocument()` | 导出到文件（由扩展名或显式格式参数决定格式） |
| `renderTo()` | 渲染到 `QPaintDevice`（`QImage`/`QPixmap`）、`QPrinter` 或 `QSvgGenerator` |
| `exportTo()` | 弹出文件对话框并导出到所选文件 |
| `setDiscardFlag()` / `setDiscardFlags()` | 设置/重置忽略选项（`DiscardNone` 表示全部渲染） |

> **提示 — 导出建议**
>
> - 打印使用PDF或高分辨率PNG（300 DPI）
> - 网页使用SVG或PNG
> - 文档嵌入使用PDF或SVG
> - 截图分享使用PNG或JPG

> **示例 — 相关示例**
>
> - 导出功能演示：`examples/2D/barchart`