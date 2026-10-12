# 控件组件 - 仪表盘与滑块

Qwt 提供了一系列仪表控件组件，用于数值显示和交互控制。这些控件适用于仪器面板、控制系统、数据监控等场景。

## 控件类型概览

| 控件类 | 说明 |
|--------|------|
| `QwtDial` | 仪表盘/刻度盘 |
| `QwtKnob` | 旋钮控件 |
| `QwtSlider` | 滑块控件 |
| `QwtThermo` | 温度计/进度条 |
| `QwtWheel` | 滚轮控件 |
| `QwtCounter` | 数值计数器 |
| `QwtCompass` | 指南针/罗盘 |

## 使用方法

控件演示的例子位于:`examples/2D/controls`，例子截图如下：

![Dial Controls](../../../assets/screenshots/controls-dial.png)

![Knob Controls](../../../assets/screenshots/controls-knob.png)

![Slider Controls](../../../assets/screenshots/controls-sliders.png)

### 1. QwtDial - 仪表盘

`QwtDial` 是一个带指针的圆形仪表面板，用来指示当前值，适合角度、方向、转速一类的驾驶舱式显示。数值范围由继承来的 `setScale()` 设定，刻度间隔由 `setScaleStepSize()` 提示；外观主要由指针决定：`setNeedle()` 挂入指针对象（如 `QwtDialSimpleNeedle`，有 `Arrow` 箭头和 `Ray` 射线两种风格），`setMode()` 则决定旋转的是指针还是刻度盘。数值本身通过继承来的 `setValue()` 设置，变化经 `valueChanged()` 信号发出。下面是典型的配置流程：

```cpp
#include <QwtDial>
#include <QwtDialSimpleNeedle>

QwtDial* dial = new QwtDial();

// 设置数值范围（继承自 QwtAbstractScale）
dial->setScale(0.0, 360.0);  // 0-360度

// 设置当前值
dial->setValue(90.0);

// 设置刻度步进提示
dial->setScaleStepSize(30.0);  // 每30度一个刻度

// 表盘几何：起始方向与刻度覆盖的圆弧
dial->setOrigin(90.0);          // 刻度从顶部开始
dial->setScaleArc(0.0, 360.0);  // 刻度覆盖整个圆周

// 旋转模式
dial->setMode(QwtDial::RotateNeedle);  // 指针旋转（默认）
// 其他选项：QwtDial::RotateScale（指针固定，刻度旋转）

// 设置指针样式
dial->setNeedle(new QwtDialSimpleNeedle(QwtDialSimpleNeedle::Arrow));

// 连接值变化信号
connect(dial, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
```

### 2. QwtKnob - 旋钮控件

`QwtKnob` 模拟一个可用鼠标转动的实体旋钮，常用于音响或仪器面板上的参数微调。`setKnobStyle()` 选择表面明暗风格（`Flat` / `Raised` / `Sunken` / `Styled`），`setKnobWidth()` 决定旋钮直径，当前值由 `setMarkerStyle()` 设定的标记指示，`setTracking(true)` 则让拖动过程中实时发出数值，示例如下：

```cpp
#include <QwtKnob>

QwtKnob* knob = new QwtKnob();

// 设置数值范围（继承自 QwtAbstractScale）
knob->setScale(0.0, 100.0);

// 设置当前值
knob->setValue(50.0);

// 刻度间隔与数值范围覆盖的旋转角
knob->setScaleStepSize(10.0);  // 每10个单位一个刻度
knob->setTotalAngle(270.0);    // 数值范围横跨270度圆弧

// 设置旋钮外观
knob->setKnobStyle(QwtKnob::Raised);  // 凸起样式
// 其他选项：Flat（平面）、Sunken（凹陷）、Styled（Qt风格渐变）

// 指示当前值的标记
knob->setMarkerStyle(QwtKnob::Notch);
// 其他选项：NoMarker、Tick、Triangle、Dot、Nub

// 设置旋钮宽度
knob->setKnobWidth(50);

// 设置鼠标跟踪模式
knob->setTracking(true);  // 拖动时实时更新值
```

### 3. QwtSlider - 滑块控件

`QwtSlider` 是 Qwt 版的线性滑块，适合在一个范围内连续调节数值。`setOrientation()` 在 `Qt::Horizontal` 与 `Qt::Vertical` 之间选择方向，`setScalePosition()` 决定刻度画在槽的哪一侧（`LeadingScale`、`TrailingScale` 或 `NoScale`），`setHandleSize()` 调整手柄尺寸，下面的例子演示了常用配置：

```cpp
#include <QwtSlider>

QwtSlider* slider = new QwtSlider();

// 设置数值范围（继承自 QwtAbstractScale）
slider->setScale(0.0, 100.0);

// 设置当前值
slider->setValue(30.0);

// 设置方向
slider->setOrientation(Qt::Horizontal);  // 水平滑块
// 或 Qt::Vertical（垂直滑块）

// 设置刻度位置
slider->setScalePosition(QwtSlider::TrailingScale);  // 刻度在后
// 其他选项：LeadingScale、NoScale

// 设置滑块样式
slider->setHandleSize(QSize(10, 20));  // 滑块手柄大小

// 键盘/滚轮输入的步进个数（以步数为单位，继承自 QwtAbstractSlider）
slider->setSingleSteps(1);   // 每次方向键走1个单步
slider->setPageSteps(10);    // 每次翻页走10个单步
```

### 4. QwtThermo - 温度计

`QwtThermo` 把数值显示为一根管道里的液柱，是温度、液位、进度类指示的自然选择。`setOrientation()` 决定竖直还是水平摆放，`setFillBrush()` 设置液柱填充色，`setPipeWidth()` 设定管道宽度，`setAlarmLevel()` 与 `setAlarmBrush()` 则让数值越过报警阈值后切换成警告颜色：

```cpp
#include <QwtThermo>

QwtThermo* thermo = new QwtThermo();

// 设置数值范围（继承自 QwtAbstractScale）
thermo->setScale(0.0, 100.0);

// 设置当前值
thermo->setValue(75.0);

// 设置方向
thermo->setOrientation(Qt::Vertical);

// 设置填充颜色
thermo->setFillBrush(QBrush(Qt::red));

// 设置管道宽度
thermo->setPipeWidth(10);

// 设置刻度位置
thermo->setScalePosition(QwtThermo::TrailingScale);

// 设置警报级别（同时隐式启用警报）
thermo->setAlarmLevel(80.0);  // 80度警报
thermo->setAlarmBrush(QBrush(Qt::darkRed));
```

### 5. QwtWheel - 滚轮控件

`QwtWheel` 模拟一个用拇指搓动的机械滚轮，适合连续平滑地调节数值，比滑块更省面板空间。`setRange()` 定义工作区间，`setWheelWidth()` 设定滚轮宽度，`setSingleStep()` 决定每格步进量，`setPageStepCount()` 决定一次翻页跨越多少个单步，`setMass()` 则赋予它松手后继续旋转的飞轮效果；`setTracking(true)` 让拖动过程中实时发出数值，示例如下：

```cpp
#include <QwtWheel>

QwtWheel* wheel = new QwtWheel();

// 设置范围
wheel->setRange(0.0, 360.0);

// 设置当前值
wheel->setValue(180.0);

// 设置方向
wheel->setOrientation(Qt::Horizontal);

// 设置滚轮宽度
wheel->setWheelWidth(30);

// 设置边框宽度
wheel->setBorderWidth(2);

// 设置步进值
wheel->setSingleStep(1.0);    // 每格步进量
wheel->setPageStepCount(15);  // 一次翻页跨越15个单步

// 飞轮效果：松手后滚轮继续旋转
wheel->setMass(0.5);

// 设置鼠标跟踪
wheel->setTracking(true);
```

### 6. QwtCounter - 数值计数器

`QwtCounter` 把可编辑的数值显示框和若干组增减按钮组合在一起，用于精确的数值输入。`setNumButtons()` 决定显示框两侧各摆几组步进按钮，`setSingleStep()` 设置第一组按钮单击一次的增减量，`setIncSteps()` 为其余按钮指定更大的步进倍数，`setWrapping(true)` 则让数值到达边界后回绕到另一端：

```cpp
#include <QwtCounter>

QwtCounter* counter = new QwtCounter();

// 设置范围
counter->setRange(0.0, 9999.0);

// 设置当前值
counter->setValue(100.0);

// 设置步进按钮
counter->setNumButtons(3);  // 每侧3个步进按钮

// 设置步进值
counter->setSingleStep(1.0);                     // 按钮1单击一次加1.0
counter->setIncSteps(QwtCounter::Button2, 100);  // 按钮2单击一次加100个单步

// 设置循环模式
counter->setWrapping(true);  // 到达边界时循环
```

### 7. QwtCompass - 指南针

`QwtCompass` 是派生自 `QwtDial` 的方向指示盘，可以在圆形刻度之上叠加罗盘玫瑰。`setRose()` 挂入玫瑰图形（如 `QwtSimpleCompassRose`），N/E/S/W 等方位字母来自 `QwtCompassScaleDraw` 的标签映射（用 `setScaleDraw()` 挂入），`setNeedle()` 则可以换成 `QwtCompassWindArrow` 这样的风向箭头指针，下面的例子给出完整配置：

```cpp
#include <QwtCompass>
#include <QwtCompassScaleDraw>
#include <QwtSimpleCompassRose>
#include <QwtCompassWindArrow>
#include <QMap>

QwtCompass* compass = new QwtCompass();

// 设置范围（角度）
compass->setScale(0.0, 360.0);

// 设置当前方向
compass->setValue(45.0);  // 北偏东45度

// 设置玫瑰图样式
compass->setRose(new QwtSimpleCompassRose());

// 用方位字母替换数字刻度标签
QMap<double, QString> labelMap;
labelMap.insert(0.0, "N");
labelMap.insert(90.0, "E");
labelMap.insert(180.0, "S");
labelMap.insert(270.0, "W");
compass->setScaleDraw(new QwtCompassScaleDraw(labelMap));

// 设置指针（指示风来向的风向箭头）
compass->setNeedle(new QwtCompassWindArrow(QwtCompassWindArrow::Style1));
```

## 核心共性方法

七个控件并非都继承同一个基类：`QwtDial`、`QwtKnob`、`QwtSlider` 派生自 `QwtAbstractSlider`（其基类又是 `QwtAbstractScale`），`QwtThermo` 只派生自 `QwtAbstractScale`，而 `QwtWheel` 与 `QwtCounter` 是独立的 `QWidget` 子类，只是提供了相似的取值接口。各方法的可用于范围如下表：

| 方法 | 可用控件 | 说明 |
|------|------|------|
| `setScale(double lower, double upper)` | Dial / Knob / Slider / Thermo | 设置数值范围与刻度边界（来自 `QwtAbstractScale`） |
| `setRange(double min, double max)` | Wheel / Counter | 设置数值范围 |
| `setValue(double)` / `value()` | 全部七个控件 | 设置/获取当前值 |
| `setSingleSteps(uint)` / `setPageSteps(uint)` | Dial / Knob / Slider | 键盘/滚轮输入的单步与翻页步数（来自 `QwtAbstractSlider`） |
| `setSingleStep(double)` | Wheel / Counter | 单步步进量 |
| `setPageStepCount(int)` | Wheel | 一次翻页跨越的单步个数 |
| `setTracking(bool)` | Dial / Knob / Slider / Wheel | 拖动过程中实时发出数值 |
| `setWrapping(bool)` | Dial / Knob / Slider / Wheel / Counter | 到达边界时循环回绕 |

> **提示 — 控件使用建议**
>
> - 仪表盘适合角度或方向显示
> - 旋钮适合参数调节
> - 滑块适合线性范围调整
> - 温度计适合进度或温度显示
> - 滚轮适合连续平滑调节

> **示例 — 相关示例**
>
> - 控件演示：`examples/2D/controls`
> - 指南针：`examples/2D/dials`
> - 飞轮效果滚轮：`examples/2D/radio`

控件演示的更多截图如下：

![Wheel & Thermo Controls](../../../assets/screenshots/controls-wheel-thermo.png)

![Dial Cockpit](../../../assets/screenshots/dials-cockpit.png)

![Dial Compass](../../../assets/screenshots/dials-compass.png)