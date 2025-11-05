# QwtFigure绘图容器窗口

`QwtFigure`是一个类似 `matplotlib` 的 `Figure` 类的容器，用于组织和管理多个 `QwtPlot` 绘图组件。它提供了灵活的布局选项，包括**归一化坐标定位**和**网格布局**，支持创建**寄生绘图（任意多个x轴和y轴）**。

## 主要功能特性

### 1. 支持多种布局方式

- **归一化坐标布局**：使用 `[0,1]` 范围内的坐标系统进行相对位置控制
- **网格布局**：支持行列式的网格排列方式，方便创建规整的多图组合

### 2. 寄生绘图支持

- 可以为宿主绘图创建共享绘图区域但拥有独立坐标轴的寄生绘图
- 适用于在同一绘图区域内展示不同量级的数据

### 3. 图形导出功能

- 支持将整个图形导出为图片文件或 QPixmap
- 可指定 DPI 进行高分辨率输出

### 4. 灵活的外观定制

- 支持自定义背景色、边框颜色和线宽
- 支持复杂背景（渐变、纹理等）

## 使用方法

`QwtFigure`是一个窗口容器，可以进行多个子绘图（`QwtPlot`）布局

### 添加绘图组件

#### 使用归一化坐标添加

归一化坐标就是占`QwtFigure`窗口的百分比

```cpp
QwtPlot* plot = new QwtPlot;
// 添加一个占据图形左上角四分之一的绘图
figure->addAxes(plot, QRectF(0.0, 0.0, 0.5, 0.5));

// 或者使用分离参数形式
figure->addAxes(plot, 0.0, 0.0, 0.5, 0.5);
```

!!! warning "归一化坐标注意事项"
    - 使用 Qt 标准的左上角坐标系
    - 归一化坐标范围为 [0,1]

#### 使用网格布局添加

网格布局参考`matplotlib`中的`subplot`布局方式

```cpp
// 创建一个2x2网格并添加绘图
QwtPlot* topPlot = new QwtPlot;
// 添加一个占据整个顶行（第0行，第0-1列）的绘图
figure->addAxes(topPlot, 2, 2, 0, 0, 1, 2);

// 添加一个到底部左侧单元格（第1行，第0列）的绘图
QwtPlot* bottomLeftPlot = new QwtPlot;
figure->addAxes(bottomLeftPlot, 2, 2, 1, 0);
```

### 设置图形外观

```cpp
// 设置背景颜色
figure->setFaceColor(Qt::lightGray);

// 设置渐变背景
QLinearGradient gradient(0, 0, 0, 1);
gradient.setColorAt(0, Qt::white);
gradient.setColorAt(1, Qt::lightGray);
figure->setFaceBrush(QBrush(gradient));

// 设置边框
figure->setEdgeColor(Qt::black);
figure->setEdgeLineWidth(2);
```

### 寄生绘图使用

寄生绘图（parasite axes命名参考`matplotlib`）可以创建多个具有独立坐标轴的绘图，适用于在同一绘图区域内展示不同量级或单位数据。常用双y轴，多y轴等特殊的绘图场景

寄生绘图创建示例代码如下：

```cpp
// 创建宿主绘图
QwtPlot* hostPlot = new QwtPlot(figure);
figure->addAxes(hostPlot, 0.1, 0.1, 0.8, 0.8);

// 创建寄生绘图，默认显示在YRight轴并
QwtPlot* parasiteYRight = figure->createParasiteAxes(hostPlot, QwtAxis::YRight);

// 分别设置轴标题
hostPlot->setAxisTitle(QwtAxis::YLeft, "Primary Y");
parasiteYRight->setAxisTitle(QwtAxis::YRight, "Secondary Y");
```

!!! warning "寄生绘图注意事项"
    - 寄生绘图不能作为当前激活的坐标轴
    - 寄生绘图会随着宿主轴的移除而被自动隐藏
    - 如果宿主绘图销毁，寄生绘图会跟随销毁

寄生绘图的详细使用方法请参考：[寄生绘图](parasite-axes.md)

### 图形导出

```cpp
// 保存为图片文件
figure->saveFig("figure.png", 300); // 300 DPI

// 获取 QPixmap
QPixmap pixmap = figure->saveFig(300);
```


!!! example "寄生绘图示例"
    完整的示例代码可参阅`examples/figure`


