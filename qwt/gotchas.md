# 高频陷阱（Agent 犯错集锦）

按"犯错频率 × 隐蔽程度"排序。每条都是本仓库**实际存在**的约束，不是泛泛而谈。

## 1. `QwtPlotItem` / `QwtPolarItem` / `Qwt3DPlotItem` 不是 QObject

`src/plot/qwt_plot_item.h:84` —— `class QWT_EXPORT QwtPlotItem`，**没有基类**、
没有 `Q_OBJECT`、没有信号槽。所以：

- ❌ `connect(curve, &QwtPlotCurve::clicked, ...)` —— 编译就过不了。
- ❌ 在自定义 item 里 `emit something()`。
- ✅ 通知重绘靠 `item->itemChanged()`（虚函数，item 内部调），宿主 `QwtPlot` 收到后
  按 `autoReplot` 策略重画。
- ✅ 真的需要信号时，把交互逻辑放到**你自己的 QObject/QWidget 子类**里，
  或者用挂在 canvas 上的 picker（`QwtPicker` 及其子类是正常 QObject）。

顺带：**值对象也不是 QObject**——`QwtSymbol`、`QwtColorMap`、`QwtText`、`QwtInterval`、
`QwtScaleDiv`、3D 的 `Qwt3DColor`/`Qwt3DTheme`。它们全部静默改值，不通知任何人。
典型后果："我改了 symbol 颜色，画面不变"——必须在改完值后让持有者刷新：

```cpp
symbol->setBrush(...);        // 静默
curve->itemChanged();         // 或重新 curve->setSymbol(symbol) 触发通知
```

## 2. 所有权陷阱（double free / 内存泄漏）

- `item->attach(plot)` 后，`QwtPlot` 默认接管所有权（`autoDelete()` 默认 true），
  析构时统一 delete。**不要**再手动 `delete item`。反过来，`attach` 到另一个 plot
  会自动从旧 plot 摘除，item 可以复用。
- `QwtPlotCurve::setSymbol(QwtSymbol*)` / `setCurveFitter(QwtCurveFitter*)` /
  `QwtDial::setNeedle(QwtDialNeedle*)` **转交所有权**：传 `new` 出来的指针，之后别再 delete。
- zoomer/panner/picker 构造时传入 canvas 控件，**父对象就是该 canvas**（QObject 父子关系），
  随画布一起销毁，**不要手动 delete**；plot 本身不持有它们。

## 3. 旧类名已在 v7 移除（照搬网上 Qwt 6 代码必错）

| 网上/旧代码里的名字 | 本仓库的正确名字 |
|--------------------|-----------------|
| `QwtPlotZoomer` | `QwtPlotAxisZoomer`（绑 2 个轴）/ `QwtPlotCanvasZoomer`（整画布） |
| `QwtPanner` | `QwtCachePanner`（像素缓存） |
| `QwtPlotPanner`（旧语义） | `QwtPlotCachePanner` |
| （想实时拖拽） | 新的 `QwtPlotPanner`（基于 QwtPicker，语义和旧同名类不同！） |

另外：网上大量示例基于上游 Qwt 6.2（Uwe Rathmann 原版），本仓库是 v7 维护分支，
API 有增删（`QwtColorMap` 移入 core 且 `rgb()` 改为
`rgb(double vMin, double vMax, double value)`；画布换成 `QwtPlotCanvas` 及其
GL/透明变体；3D 全面重构为 Plot+Item）。**拿不准就打开对应头文件看真实签名**，
不要凭记忆写。

## 4. 坐标轴 API 用 `QwtAxisId`，不是 int

`setAxisScale(QwtAxis::XBottom, ...)`、`enableAxis(QwtAxis::YRight)`。
旧枚举 `QwtPlot::yLeft`/`xBottom` 通过 `QWT_AXIS_COMPAT` 还能用，但**新代码一律
`QwtAxis::Position` + `QwtAxisId`**。极坐标侧则是 `QwtPolar::Scale`（`ScaleAzimuth`/
`ScaleRadius`）和 `QwtPolar::Axis`，两套体系别串。

## 5. 改了样式/数据不刷新 = 忘了通知

- item 数据变化：`curve->setSamples(...)` / `curve->setRawSamples(...)` 内部会通知；
  但**直接改外部缓冲区**再期望画面更新是不行的，要重新 `setRawSamples` 或调 `itemChanged()`。
- `QwtPlot::autoReplot`（默认 true）控制是否自动重画；高频实时场景示例里常见
  临时 `setAutoReplot(false)` + 手动 `replot()`，别把这当成"不需要通知"。
- 数据范围变了想让轴自动缩：`QwtPlotRescaler`（`playground/rescaler`），
  不要手写 `setAxisScale` + 遍历。

## 6. 3D 模块的三个专属坑

- **没有 `namespace Qwt3D`**：所有类裸前缀 `Qwt3D`（v7.3.3+ 已移除）。
  写 `using namespace Qwt3D;` 直接编译失败。
- **`Qwt3DPlot` 不持数据**：数据/绘制逻辑都在 item（`Qwt3DSurface` 等）。
  找"plot 怎么设数据"会一无所获，应该去 item 的 `loadFromData()`。
- **`Qwt3DColor` 家族静默**：就地改 functor 后必须 `surface->invalidateColors()`。
  更细的分层/依赖规则见 `src/plot3d/AGENTS.md`。

## 7. Qt5/Qt6 差异必须走兼容层

新写任何事件处理，鼠标位置用 `qwt::compat::eventPos(event)`，滚轮用
`qwt::compat::wheelEventDelta(event)`，文本宽度用 `qwt::compat::horizontalAdvance(fm, str)`。
直接写 `event->pos()` / `event->delta()` 在 Qt6 下已废弃（编译告警，行为还可能不一致），
见 `src/core/qwt_qt5qt6_compat.hpp`。

## 8. 源码规范（不改规范就等着 CI/评审打回）

- **注释一律英文**，Doxygen 用 `@brief`/`@param` 前缀；本 skill 与文档里的中文不要
  流进 `src/` 下的注释。
- include 顺序**手动排**（own header → Qt → STL → 本项目），`SortIncludes: false`：
  别让工具自动重排。
- PIMPL 用 `QWT_DECLARE_PRIVATE` / `QWT_D()` / `QWT_DC()`，别自己加 `d_ptr`。
- **不要手改** `src-amalgamate/QwtPlot.h/.cpp`（自动合成）与 `tools/`。
- 日志/调试用 `qDebug()`，别引入 `printf`/`std::cout` 到新代码里。

## 9. 构建与验证的预期管理

- 全量 `.\build.ps1` 会因为 `tests/flat_style_test`（历史遗留、仅 Qt5、依赖 `qgl.h`）
  返回非零退出码——这是**已知失败**，不代表你的改动有问题；判断成败看 qwt 主库目标。
- 声称"编译通过"前必须真的跑过构建；验证渲染相关改动的回路是运行
  `examples/2D/` 或 `examples/3D/` 下最接近的示例，而不是空想。
- `playground/` 是实验代码，**不是 API 范本**；要抄写法抄 `examples/`。

## 10. 日志、指针、小细节

- 日志/指针/空值：`nullptr` 不用 `NULL`，`static_cast` 不用 C 风格转换。
- 迭代 Qt 容器（可能 COW）用 `qwt_as_const(container)` 防深拷贝。
- 智能指针用 `qwt_make_unique<T>(...)`（C++11 兼容），不要 `std::make_unique`。
- 信号槽新语法：`connect(sender, &Sender::sig, receiver, &Receiver::slot)`；
  宏写 `Q_SIGNALS:` / `public Q_SLOTS:`。
- 完整规范以根目录 `AGENTS.md` 与 `docs/dev-guide/coding-standards.md` 为准。
