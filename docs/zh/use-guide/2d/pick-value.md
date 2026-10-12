# 数据拾取

数据拾取功能是一个绘图控件常用的需求，旨在用户通过鼠标移动来显示当前鼠标所在位置的具体数值情况

`Qwt7`增加了`QwtPlotSeriesDataPicker`类，它提供了在绘图区域移动鼠标时实时显示数据点信息的功能。该类支持两种拾取模式：Y值拾取模式和最近点拾取模式，能够显著提升数据可视化和分析的交互体验。

**特性**

- ✅ **双模式拾取**：支持Y值拾取和最近点拾取两种模式
- ✅ **智能插值**：支持线性插值计算，在数据点之间精确取值
- ✅ **高性能优化**：使用二分查找和窗口优化算法，支持大数据集的最近点拾取
- ✅ **多曲线支持**：同时处理多个曲线的数据拾取
- ✅ **自定义显示**：可自定义文本样式、特征点绘制和背景
- ✅ **寄生绘图支持**：支持宿主绘图和寄生绘图的数据获取

头文件引入

```cpp
#include "qwt_plot_series_data_picker.h"
```

效果：

![series-data-picker](../../../assets/screenshots/series-data-picker.png)

## 基本用法

`QwtPlotSeriesDataPicker`继承`QwtPlotPicker`,要创建拾取器，只要传入绘图的canvas区域即可，示例代码如下

```cpp
// 创建绘图对象
QwtPlot* plot = new QwtPlot(this);

// 创建数据拾取器
QwtPlotSeriesDataPicker* picker = new QwtPlotSeriesDataPicker(plot->canvas());

// 设置拾取模式
picker->setPickMode(QwtPlotSeriesDataPicker::PickYValue);

// 启用插值
picker->setInterpolationMode(QwtPlotSeriesDataPicker::LinearInterpolation);

// 设置文本显示位置
picker->setTextArea(QwtPlotSeriesDataPicker::TextPlaceAuto);
```

## 拾取模式设置

`QwtPlotSeriesDataPicker`提供了两种拾取模式，可以通过`setPickMode`函数进行拾取模式的设置，拾取模式枚举如下：

```cpp
enum PickSeriesMode
{
    PickYValue,  ///< 拾取y值（默认）
    PickNearestPoint  ///< 拾取最接近鼠标光标位置的点
};
```

### Y值拾取模式

Y值拾取模式（`QwtPlotSeriesDataPicker::PickYValue`）可以显示当前X位置对应所有曲线的Y值

```cpp
// Y值拾取模式 - 显示当前X位置对应的所有曲线的Y值
picker->setPickMode(QwtPlotSeriesDataPicker::PickYValue);
```

Y值拾取模式效果如下：

![series-data-picker-yvalue](../../../assets/picture/series-data-picker-yvalue.gif)

y值拾取模式下，可以通过`setInterpolationMode`方法设置是否进行插值计算，插值计算可以提高数据的精度，如果点比较稀疏，会通过线性插值方式计算出当前x轴对应的y值。

> **信息 — 注意**
>
> 插值方式默认开启

```cpp
// 不进行插值，使用最近的数据点
picker->setInterpolationMode(QwtPlotSeriesDataPicker::NoInterpolation);
```

### 临近点拾取模式

临近点拾取模式（`QwtPlotSeriesDataPicker::PickNearestPoint`）会计算距离鼠标最接近的点进行拾取并显示，这个模式尤其适合用于类似频谱等绘图的峰值数据拾取

```cpp
// 最近点拾取模式 - 显示距离鼠标最近的数据点
picker->setPickMode(QwtPlotSeriesDataPicker::PickNearestPoint);
```

临近点拾取模式效果如下：

![sseries-data-picker-nearest-value](../../../assets/picture/series-data-picker-nearest-value.gif)

临近点要计算曲线的点到鼠标位置的距离，如果全曲线遍历会非常耗时，为此，`Qwt7`提供了窗口搜索算法，能够快速找到距离鼠标最近的数据点

通过`setNearestSearchWindowSize`方法设置搜索窗口大小

```cpp
//临近点搜索窗口大小，窗口大小决定了临近点搜索的范围，避免全曲线遍历
void setNearestSearchWindowSize(int windowSize);
int nearestSearchWindowSize() const;
```

搜索窗口可以设置为以下几种方式：

- 0: 不使用窗口，搜索整个曲线
- 正数: 固定的窗口大小（数据点数量）
- 负数: 自适应窗口，使用曲线数据点总数的百分比（取绝对值，如-5表示5%）

搜索窗口大小默认为-5，也就是曲线点数的5%

> **警告 — 注意**
>
> Qwt7默认启用窗口搜索算法的阈值是1000个数据点，也就是说如果曲线点数超过1000窗口设置才会生效

> **说明 — 注意事项**
>
> 窗口优化算法要求曲线数据必须按X坐标升序排列。如果使用自定义数据源，请确保数据已正确排序。


## 其它属性设置

### 文本显示设置

`QwtPlotSeriesDataPicker`可以设置文本显示位置，文本显示位置通过枚举`QwtPlotSeriesDataPicker::TextPlacement`定义，你可以使用`setTextArea`函数设置文本放置的地方，默认为自动选择，会根据拾取模式智能选择

你可以通过以下方法设置文本的对其样式和背景颜色

```cpp
// 自定义文本背景
picker->setTextBackgroundBrush(QBrush(QColor(255, 255, 255, 180)));
// 设置文本对齐方式
picker->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
```

> **已知问题 — Bug**
>
> `QwtPlotSeriesDataPicker`的绘制区域是宿主绘图的图层，寄生绘图的属于宿主绘图的子窗口，因此寄生绘图绘制的图元会覆盖宿主绘图，也就是说寄生绘图显示的曲线是在`QwtPlotSeriesDataPicker`的绘制区域之上。目前的寄生绘图架构无法让`QwtPlotSeriesDataPicker`的内容显示在寄生绘图之上

### 特征点绘制

`QwtPlotSeriesDataPicker`拾取到的点称之为特征点，特征点会在曲线上标记出来，你可以通过以下方法设置特征点绘制样式

```cpp
// 启用/禁用特征点标记
picker->setEnableDrawFeaturePoint(true);

// 设置特征点大小（像素）
picker->setDrawFeaturePointSize(6);
```

## 自定义

### 自定义显示文本

显示的文本内容可以通过继承`QwtPlotSeriesDataPicker`并重写 `valueString` 方法来自定义数据点的显示格式：

下面是一个示例：

```cpp
class CustomDataPicker : public QwtPlotSeriesDataPicker {
public:
    explicit CustomDataPicker(QWidget* canvas) : QwtPlotSeriesDataPicker(canvas) {}
    
protected:
    QString valueString(const QPointF& value, QwtPlotItem* item, 
                       size_t seriesIndex, int order) const override {
        Q_UNUSED(seriesIndex);
        
        if (pickMode() == PickYValue) {
            QString text;
            if (order != 0) {
                text += "\n";
            }
            text += QString("%1: %2 (X=%3)")
                .arg(item->title().text())
                .arg(value.y(), 0, 'f', 3)
                .arg(value.x(), 0, 'f', 3);
            return text;
        }
        return QString("坐标: (%1, %2)").arg(value.x()).arg(value.y());
    }
};
```

`valueString`方法的`order`参数代表了显示的第n个特征点，如果你有多条曲线，这个参数会递增，你可以根据这个参数是否为0来换行

## 点击信号 (Click Signals)

`QwtPlotSeriesDataPicker` 提供了点击信号，用于响应用户的鼠标点击操作。

### 信号说明

`QwtPlotSeriesDataPicker` 发出 `clicked()` 与 `doubleClicked()` 两个信号，参数均为发出信号的 picker 指针与点击的屏幕位置，签名如下：

```cpp
// 单击信号
void clicked(QwtPlotSeriesDataPicker* picker, const QPoint& pos);

// 双击信号
void doubleClicked(QwtPlotSeriesDataPicker* picker, const QPoint& pos);
```

**信号参数 (Signal Parameters):**

| 参数 (Parameter) | 类型 (Type) | 说明 (Description) |
|------------------|-------------|-------------------|
| picker | QwtPlotSeriesDataPicker* | 被点击的 picker 指针 (Pointer to the clicked picker) |
| pos | QPoint | 点击事件的屏幕位置 (Screen position of the click event) |

### 左键限制 (Left Button Only)

> **信息 — 注意**
>
> 点击信号**仅响应鼠标左键** (Qt::LeftButton)。右键和中键点击不会触发这些信号。

### 双击触发行为 (Double-Fire Behavior)

> **警告 — 注意**
>
> 当用户双击时，会先触发 `clicked()` 信号，然后触发 `doubleClicked()` 信号。 这是 Qt 的标准行为，不是 Bug。
>
> 如果你需要区分单击和双击，建议只连接其中一个信号，不要同时连接两个信号。

## 数据获取 (Getting Picked Data)

通过 `featurePoints()` 方法可以获取当前拾取到的数据点信息。

### FeaturePoint 结构

每个被拾取的点由内嵌结构体 `QwtPlotSeriesDataPicker::FeaturePoint` 描述，它打包了所属绘图项（`item`）、拾取到的坐标（`feature`）以及该点在系列数据中的索引（`index`）。其定义如下：

```cpp
struct FeaturePoint
{
    QwtPlotItem* item { nullptr };  ///< 对应的曲线项 (Corresponding curve item)
    QPointF feature { 0, 0 };       ///< 特征点坐标 (Feature point coordinates)
    size_t index { 0 };             ///< 在曲线数据中的索引 (Index in the curve data)
};
```

**字段说明 (Field Description):**

| 字段 (Field) | 类型 (Type) | 说明 (Description) |
|--------------|-------------|-------------------|
| item | QwtPlotItem* | 曲线指针，可通过 `item->title().text()` 获取曲线名称 |
| feature | QPointF | 数据点的坐标值 (x, y) |
| index | size_t | 数据点在曲线样本中的索引位置 |

### featurePoints() 方法

`featurePoints()` 是一个 const 访问器，以 `QList<FeaturePoint>` 返回当前 tracker 位置拾取到的全部特征点，签名如下：

```cpp
// 获取当前拾取到的特征点列表
QList<FeaturePoint> featurePoints() const;
```

`featurePoints()` 返回当前 tracker 位置拾取到的所有特征点列表。通常在 `clicked` 或 `doubleClicked` 信号处理器中调用此方法获取点击位置的数据。

> **提示 — 提示**
>
> 返回的列表可能包含多个 `FeaturePoint`，因为鼠标位置可能同时对应多条曲线的数据点（特别是在 Y 值拾取模式下）。

## 组内信号转发 (Group Signal Forwarding)

`QwtPlotSeriesDataPickerGroup` 提供了组内点击信号的转发功能。

### 组信号说明

`QwtPlotSeriesDataPickerGroup` 会转发组内每个 picker 的 `clicked()` 与 `doubleClicked()` 信号，因此只需连接 group 一次即可覆盖所有成员；参数中的 `picker` 仍然指示被点击的具体成员。转发信号的签名如下：

```cpp
// 组内 picker 被点击时发出
void clicked(QwtPlotSeriesDataPicker* picker, const QPoint& pos);

// 组内 picker 被双击时发出
void doubleClicked(QwtPlotSeriesDataPicker* picker, const QPoint& pos);
```

### 同步机制 (Sync-Before-Signal)

> **信息 — 重要特性**
>
> Group 在发出点击信号之前，会先同步组内所有其他 picker 的位置。 这意味着当 `clicked` 信号触发时，组内所有 picker 的 `featurePoints()` 都已经更新到同步后的位置。

这个特性对于多子图联动场景非常有用：当用户点击某个子图时，其他子图的 picker 已经同步到对应的 X 轴比例位置，可以直接获取所有子图在相同比例位置的数据。

## 代码示例 (Code Example)

以下示例展示如何连接 Group 的点击信号并获取拾取数据：

```cpp
#include "qwt_plot_series_data_picker.h"
#include "qwt_plot_series_data_picker_group.h"

// 创建 Group 并添加多个 picker
QwtPlotSeriesDataPickerGroup* pickerGroup = new QwtPlotSeriesDataPickerGroup(this);
pickerGroup->addPicker(picker1);
pickerGroup->addPicker(picker2);
pickerGroup->addPicker(picker3);

// 连接 Group 的点击信号
connect(pickerGroup, &QwtPlotSeriesDataPickerGroup::clicked,
        this, &MyClass::onPickerGroupClicked);

// 点击事件处理器
void MyClass::onPickerGroupClicked(QwtPlotSeriesDataPicker* picker, const QPoint& pos)
{
    Q_UNUSED(pos);
    
    // 获取当前拾取到的特征点列表
    QList<QwtPlotSeriesDataPicker::FeaturePoint> fps = picker->featurePoints();
    
    // 遍历所有拾取到的数据点
    for (const auto& fp : fps) {
        if (fp.item) {
            qDebug() << "曲线 (Curve):" << fp.item->title().text()
                     << "X:" << fp.feature.x()
                     << "Y:" << fp.feature.y()
                     << "索引 (Index):" << fp.index;
        }
    }
}
```

### 多子图联动示例

在多子图布局中，可以通过 Group 的同步机制实现跨图数据检视：

```cpp
// 假设有多个子图，每个子图都有自己的 picker
QwtPlotSeriesDataPickerGroup* group = new QwtPlotSeriesDataPickerGroup(this);
group->addPicker(plot1Picker);
group->addPicker(plot2Picker);
group->addPicker(plot3Picker);

connect(group, &QwtPlotSeriesDataPickerGroup::clicked,
        this, [this](QwtPlotSeriesDataPicker* picker, const QPoint& pos) {
    Q_UNUSED(pos);
    
    // 获取被点击 picker 的数据
    auto fps = picker->featurePoints();
    if (!fps.isEmpty()) {
        const auto& fp = fps.first();
        double xValue = fp.feature.x();
        
        // 由于 Group 的同步机制，此时所有 picker 都已经同步
        // 可以获取其他 picker 在相同 X 比例位置的数据
        for (auto* p : { plot1Picker, plot2Picker, plot3Picker }) {
            auto otherFps = p->featurePoints();
            if (!otherFps.isEmpty()) {
                qDebug() << p->canvas()->parent()->objectName()
                         << "Y value at x=" << xValue 
                         << ":" << otherFps.first().feature.y();
            }
        }
    }
});
```


