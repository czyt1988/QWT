# 注释规范

本文档规定 Qwt 项目代码注释的编写规范，确保文档与代码的一致性和完整性。

## 语言要求

> **警告 — 重要：仅使用英文**
>
> Qwt 是国际化项目，**所有源代码中的注释（包括 Doxygen 文档注释）必须使用英文，禁止出现中文**。
> 中文文档仅允许出现在 `docs/` 目录中。

## 主要规范要求

**特性**

- ✅ **Doxygen格式**：使用 Doxygen 标准格式编写注释
- ✅ **纯英文注释**：所有注释一律使用英文
- ✅ **位置规范**：详细注释写在 cpp 文件，简洁注释写在头文件
- ✅ **完整性**：类、函数、信号都需要完整的文档注释

## Doxygen 关键字规范

Doxygen 关键字统一使用 `@` 前缀（如 `@brief`、`@param`、`@return`、`@details`）：

```cpp
/**
 * @brief Brief description here
 * @param name Parameter description
 * @return Return value description
 */
```

## 注释位置原则

> **信息 — 核心原则**
>
> - **详细注释**写在 `.cpp` 文件中
> - **简洁注释**写在 `.h` 文件中
> - 保持头文件整洁，避免过多注释内容

## 源文件注释规范

所有新增代码必须使用 **Doxygen 英文格式**编写注释。

### 函数注释模板

```cpp
/**
 * @brief [Brief description]
 * @param param_name [Parameter description]
 * @return [Return value description]
 * @details [Detailed explanation]
 */
void MyClass::myFunction(int param_name)
{
    // ...
}
```

### 参数说明

参数方向标注是**推荐而非强制**的。对于 value 和 const ref 参数，方向显而易见，可省略 `[in]`。

但以下情况**必须**标注方向：

- `@param[out]` — 非 const 引用/指针，用于输出结果
- `@param[in,out]` — 非 const 引用/指针，既读取又写入

```cpp
/**
 * @brief Calculate the intersection point of two curves
 * @param curve1 First curve to intersect
 * @param curve2 Second curve to intersect
 * @param[out] intersectionPoint The calculated intersection point
 * @return true if intersection found, false otherwise
 */
bool calculateIntersection(const QwtPlotCurve* curve1,
                           const QwtPlotCurve* curve2,
                           QPointF& intersectionPoint);
```

### 详细说明示例

```cpp
/**
 * @brief Set the samples for the curve
 * @param xData Array of x coordinates
 * @param yData Array of y coordinates
 * @param size Number of samples
 * @details This method creates a new sample series from the provided arrays.
 *          The data is copied internally, so the original arrays can be
 *          modified or deleted after calling this method.
 */
void QwtPlotCurve::setSamples(const double* xData,
                              const double* yData,
                              int size);
```

## 头文件注释规范

头文件中的 `public` 函数声明仅添加**单行英文简要注释**。

### 简洁注释示例

```cpp
class MyClass {
public:
    // Constructor for MyClass
    MyClass();

    // Set the title text
    void setTitle(const QString& title);

    // Get the current title
    QString title() const;
};
```

> **警告 — 注意**
>
> 头文件中**不要**写详细的 Doxygen 注释块，详细内容应保留在对应的 `.cpp` 文件中。

## 类注释规范

类的 Doxygen 注释需要在头文件中添加。

### 类注释模板

```cpp
/**
 * @brief 2D plotting widget for displaying curves and other plot items
 * @details QwtPlot is the main widget for displaying 2D data. It manages
 *          axes, legends, and plot items like curves, markers, and grids.
 */
class QwtPlot : public QWidget
{
    QWT_DECLARE_PRIVATE(QwtPlot)

public:
    // Constructor
    QwtPlot(QWidget* parent = nullptr);

    // ...
};
```

### 带示例的类注释

对于功能性较强的类，应加入使用示例：

```cpp
/**
 * @brief Container widget for organizing multiple QwtPlot instances
 * @details QwtFigure provides a layout container similar to matplotlib's Figure,
 *          allowing multiple plots to be arranged in a single window.
 *
 * @code
 * QwtFigure* figure = new QwtFigure();
 * figure->setSizeInches(8, 6);
 *
 * QwtPlot* plot1 = new QwtPlot();
 * figure->addAxes(plot1, 0.0, 0.0, 0.5, 0.5);  // Left-top quadrant
 * @endcode
 */
class QwtFigure : public QWidget
{
    // ...
};
```

## 信号注释规范

Qt 的信号在头文件中声明，没有对应的 cpp 定义，因此信号的注释需要在头文件中完整添加。

### 信号注释示例

```cpp
class QwtPlot : public QWidget
{
Q_SIGNALS:
    /**
     * @brief Signal emitted when the plot scale changes
     * @param axisId The axis that changed its scale
     */
    void scaleChanged(QwtAxis::Position axisId);
};
```

## 注释风格检查表

在编写注释时，请检查以下要点：

| 检查项 | 位置 | 要求 |
|--------|------|------|
| 类注释 | 头文件 `.h` | 英文 Doxygen，包含使用示例 |
| public函数详细注释 | 源文件 `.cpp` | 英文 Doxygen |
| public函数简要注释 | 头文件 `.h` | 单行英文 |
| 信号注释 | 头文件 `.h` | 英文 Doxygen |
| private/protected函数 | 源文件 `.cpp` | 可选，建议英文 |
| 行内注释 | 任意位置 | 英文 |

## 常见错误示例

### ❌ 错误：头文件中写详细注释

```cpp
// 错误示例 - 头文件中写太多注释
class MyClass {
public:
    /**
     * @brief Constructor for MyClass
     * @param parent Parent widget pointer
     * @details This constructor initializes the class with...
     */
    MyClass(QWidget* parent = nullptr);
};
```

### ❌ 错误：使用中文注释

```cpp
// 错误示例 - 源码中使用了中文注释
void QwtPlotCurve::setSamples(const double* data, int size)
{
    m_data = data;  // 错误：中文注释
    m_size = size;
}
```

### ✅ 正确：简洁头文件注释 + 详细cpp注释

头文件 `MyClass.h`：

```cpp
class MyClass {
public:
    // Constructor
    MyClass(QWidget* parent = nullptr);
};
```

源文件 `MyClass.cpp`：

```cpp
/**
 * @brief Constructor for MyClass
 * @param parent Parent widget pointer
 * @details This constructor initializes the widget and sets up
 *          the internal data structures.
 */
MyClass::MyClass(QWidget* parent)
    : QWidget(parent)
{
}
```

## 相关文档

- [编码规范](coding-standards.md)
- [PIMPL模式使用指南](pimpl-pattern.md)
