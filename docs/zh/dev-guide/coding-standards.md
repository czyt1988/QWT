# 编码规范

Qwt 源码遵循以下格式化、C++ 版本兼容、命名和 Qt 使用规范。

## 主要规范要求

Qwt 源码使用 `.clang-format` 配置格式化，使用项目提供的 C++11 兼容宏，优先采用 `override`、`nullptr`、`using` 等现代语法，并遵循 Qt 开发规范。

## 代码格式化

项目使用 `.clang-format` 配置文件进行代码格式化。

> **提示 — 格式化建议**
>
> - 在提交代码前运行 clang-format 格式化
> - IDE 可配置自动格式化（保存时触发）
> - 保持与现有代码风格一致

## C++11 兼容性宏

项目提供一组兼容性宏，确保代码在 C++11 和 C++17 环境下均可编译。

### 智能指针

使用 `qwt_make_unique` 以 C++11 兼容的方式创建 `std::unique_ptr`，示例如下：

```cpp
// 创建 unique_ptr（C++11兼容）
auto ptr = qwt_make_unique<MyClass>(args...);
```

### 容器迭代

迭代 Qt 容器时用 `qwt_as_const` 包裹，避免隐式共享引发的深拷贝：

```cpp
// Qt 容器安全迭代，防止深拷贝
for (const auto& item : qwt_as_const(container)) {
    // 处理 item
}
```

### 编译期常量

编译期常量使用 `constexpr` 声明，示例如下：

```cpp
// 使用 constexpr 定义编译期常量
constexpr int MAX_SAMPLES = 1000;
```

## 关键代码规范

### 1. 使用 override 和 final

使用标准 C++ 关键字代替旧宏：

```cpp
// 错误：使用旧宏
virtual void draw(QPainter* painter) QWT_OVERRIDE;

// 正确：使用标准关键字
virtual void draw(QPainter* painter) override;
```

| 旧宏 | 新关键字 |
|------|----------|
| `QWT_OVERRIDE` | `override` |
| `QWT_FINAL` | `final` |

### 2. 使用 nullptr 代替 NULL

空指针用 `nullptr` 声明，不用 `NULL`：

```cpp
// 错误
QwtPlot* plot = NULL;

// 正确
QwtPlot* plot = nullptr;
```

### 3. 使用 static_cast 代替 C 风格强制转换

强制转换使用 `static_cast<>`：

```cpp
// 错误：C 风格强制转换
int value = (int)doubleValue;

// 正确：static_cast
int value = static_cast<int>(doubleValue);
```

### 4. 使用 using 代替 typedef

用 `using` 别名代替 `typedef`：

```cpp
// 错误：旧式 typedef
typedef QVector<QwtSample> SampleVector;

// 正确：using 别名
using SampleVector = QVector<QwtSample>;
```

### 5. Qt 容器迭代规范

使用 `qwt_as_const` 防止 Qt 容器深拷贝：

```cpp
// 错误: 可能导致深拷贝
for (const auto& item : container) { }

// 正确: 安全迭代
for (const auto& item : qwt_as_const(container)) { }
```

> **警告 — 重要**
>
> Qt 容器在迭代时可能发生隐式共享分离（深拷贝），使用 `qwt_as_const` 可避免此问题。

## Qt 开发最佳实践

### 信号槽语法

使用 Qt5+ 的信号槽语法：

```cpp
// 错误：使用旧式 SIGNAL/SLOT 宏
connect(sender, SIGNAL(valueChanged(int)), receiver, SLOT(updateValue(int)));

// 正确：使用新式函数指针语法
connect(sender, &Sender::valueChanged, receiver, &Receiver::updateValue);
```

### Q_PROPERTY 使用

正确使用 `Q_PROPERTY` 宏：

```cpp
class MyWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)

public:
    QString title() const;
    void setTitle(const QString& title);

Q_SIGNALS:
    void titleChanged(const QString& title);
};
```

### 关键字使用规范

使用 Qt 官方推荐的宏：

```cpp
// 正确
Q_SIGNALS:    // 而非 signals:
Q_SLOT:       // 而非 slot:
public Q_SLOTS:  // 而非 public slots:
```

> **警告 — 禁止**
>
> 不要使用 `slot`、`signal` 等小写命名的宏，它们在某些编译器下可能有问题。

## 命名规范

### 类名

- 使用驼峰命名法，首字母大写
- 以 `Qwt` 开头：`QwtPlotCurve`

### 方法名

- 使用驼峰命名法，首字母小写
- getter 方法不加 `get` 前缀：`boundingRect()`
- setter 方法使用 `set` 前缀：`setSamples()`

### 变量名

- 成员变量使用 `m_` 前缀：`m_data`
- 局部变量使用驼峰命名法：`sampleCount`
- 常量使用全大写：`MAX_WIDTH`

## 相关文档

- [注释规范](comment-standards.md)
- [PIMPL模式使用指南](pimpl-pattern.md)