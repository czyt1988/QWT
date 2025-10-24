# QwtFigure 寄生轴使用指南

寄生轴（Parasite Axes）是 `QwtFigure` 中在同一个绘图区域内创建多个具有不同刻度和标签的坐标轴。寄生轴与宿主轴共享绘图区域，但可以拥有独立的坐标系统，这在需要展示不同量级或单位的数据时特别有用。

## 寄生轴的工作原理

寄生轴是通过 `QwtFigure::createParasiteAxes` 方法创建的，它与宿主轴共享绘图区域，但具有以下特点：

1. **共享绘图区域**：寄生轴与宿主轴使用相同的绘图区域
2. **独立坐标系统**：寄生轴可以有自己的刻度范围和标签
3. **透明背景**：寄生轴具有透明背景，只显示坐标轴本身和绘制的曲线
4. **自动同步**：可以选择性地与宿主轴共享 X 轴或 Y 轴的刻度

## 创建寄生轴

寄生轴只能通过 `QwtFigure::createParasiteAxes` 方法创建，该方法定义如下：

```cpp
QwtPlot* parasitePlot = figure->createParasiteAxes(
    QwtPlot* hostPlot,           // 宿主绘图
    QwtAxis::Position enableAxis, // 启用的轴位置
    bool shareX = true,          // 是否共享X轴
    bool shareY = false          // 是否共享Y轴
);
```

`enableAxis`为寄生轴显示的坐标轴，`shareX` 和 `shareY` 参数则指定和宿主共享哪个轴：
- `shareX = true`：寄生轴与宿主轴共享X轴刻度
- `shareY = true`：寄生轴与宿主轴共享Y轴刻度

该方法创建的寄生轴对象，与宿主轴共享绘图区域，但可以拥有独立的坐标系统，此时宿主轴内部会记录绑定的寄生轴对象，**当宿主轴被销毁时，寄生轴也会被销毁。**

下面是创建寄生轴的代码示例：

```cpp
// 创建宿主绘图
QwtPlot* hostPlot = new QwtPlot();
figure->addAxes(hostPlot, 0.1, 0.1, 0.8, 0.8);

// 创建寄生轴，启用右侧Y轴
QwtPlot* parasitePlot = figure->createParasiteAxes(
    hostPlot, 
    QwtAxis::YRight,  // 启用右侧Y轴
    true,             // 共享X轴刻度
    false             // 不共享Y轴刻度
);
```

完整的示例可参阅`examples/figure`,以下示例来自此示例程序中的 `createGrid32_parasitePlot` 函数：

```cpp hl_lines="32 33"
void createGrid32_parasitePlot(QwtFigure* figure)
{
    // 1. 创建宿主绘图
    QwtPlot* hostPlot = new QwtPlot();
    hostPlot->setCanvasBackground(Qt::white);
    
    // 2. 设置宿主绘图坐标轴标题
    hostPlot->setTitle("Grid Layout (3x2, Cell 2,0-1)");
    hostPlot->setAxisTitle(QwtPlot::xBottom, "X Axis");
    hostPlot->setAxisTitle(QwtPlot::yLeft, "Y1 Axis");
    hostPlot->setAxisTitle(QwtPlot::xTop, "X1 Axis Top");
    hostPlot->setAxisTitle(QwtPlot::yRight, "Y1 Axis Right");
    hostPlot->enableAxis(QwtPlot::yRight, true);
    hostPlot->enableAxis(QwtPlot::xTop, true);

    // 3. 给宿主绘图添加网格和曲线
    QwtPlotGrid* grid = new QwtPlotGrid();
    grid->setPen(Qt::gray, 0.0, Qt::DotLine);
    grid->attach(hostPlot);

    QwtPlotCurve* hostCurve = new QwtPlotCurve("Host Sine Wave");
    hostCurve->setSamples(generateSampleData(100, 1.2, 0.8));
    hostCurve->setPen(Qt::darkCyan, 0.5);
    hostCurve->attach(hostPlot);

    // 4. 把宿主绘图添加到figure中
    figure->addAxes(hostPlot, 3, 2, 2, 0, 1, 2);  // 3x2网格，第2行第0列，跨2列

    // 5. 创建寄生轴
    QwtPlot* parasitePlot = figure->createParasiteAxes(hostPlot, QwtAxis::YLeft);

    // 6. 调整宿主轴和寄生轴的显示位置
    hostPlot->axisWidget(QwtAxis::YLeft)->setEdgeMargin(80);
    parasitePlot->axisWidget(QwtAxis::YLeft)->setMargin(60);

    // 7. 设置寄生轴标题和数据
    parasitePlot->setAxisTitle(QwtPlot::yLeft, "Y2 Axis");
    QwtPlotCurve* parasiteCurve = new QwtPlotCurve("Sine Wave 6");
    parasiteCurve->setSamples(generateSampleData(100, 2000, 2.3));
    parasiteCurve->attach(parasitePlot);
}
```

此例中展示了如何创建一个包含两个寄生轴的绘图区域。由于寄生轴与宿主轴共享绘图区域，因此需要调整宿主轴的和寄生轴的边缘位置，给坐标轴预留出可显示的空间(如上面代码32、33行所示)，避免轴标签重叠。

!!! tips "说明"
    目前QwtFigure不支持自动计算预留空间，需要手动调节


## 注意事项

### 1. 生命周期管理

- 寄生轴的生命周期与宿主轴绑定
- 当宿主轴被移除或销毁时，寄生轴也会自动被清理
- 不需要手动删除寄生轴对象

### 2. 布局限制

- 寄生轴不能作为`QwtFigure`当前激活的坐标轴
- 寄生轴不参与`QwtFigureLayout`的布局计算
- 寄生轴的位置由宿主轴控制