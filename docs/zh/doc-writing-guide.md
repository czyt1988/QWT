# Qwt 中文文档撰写规范

本规范用于指导 Qwt 中文文档的撰写，确保文档风格统一、内容完整、易于理解。

本项目文档使用mkdocs组织，采用[mkdocs-material](https://squidfunk.github.io/mkdocs-material/getting-started/)主题。但 Markdown 源文件必须保持**平台通用性**：同一份文件在 MkDocs 站点上、以及在 GitHub/GitLab 等其他平台上都要能正确渲染。只使用普通 Markdown 与 GitHub-Flavored Markdown 都支持的语法，避免依赖特定渲染器的扩展语法。

禁止使用：Material 提示框（`!!!`、`???`）、选项卡内容（`=== "标签"`）、属性列表（`{: .class }`）、emoji 短代码（`:material-...:`）、pymdownx 按键标记（`++ctrl++`）与高亮标记（`==高亮==`）。这些语法一旦脱离 MkDocs 构建就会以纯文本形式显示。

项目文档绘图优先考虑`mermaid`

## 文档结构

### 1. 标题层级

```markdown
# 类名/功能名称
## 主要功能特性
## 使用方法
### 子功能模块
#### 具体功能点
## 注意事项
## 参考资料
```

### 2. 必备章节

每个功能文档必须包含以下章节：

| 章节 | 必备程度 | 说明 |
|------|----------|------|
| 功能概述 | 必备 | 开头一句话说明类的用途和特点 |
| 主要功能特性 | 必备 | 列举核心功能，使用 ✅ 标记 |
| 使用方法 | 必备 | 详细的使用步骤和代码示例 |
| 注意事项 | 推荐 | 使用引用块标注重要信息 |
| 参考资料 | 可选 | 相关文档、示例路径链接 |

## 内容撰写原则

### 1. 文字说明要求

- **每个代码块前后必须有文字说明**：解释代码的作用、关键步骤、输出效果
- **避免纯代码堆砌**：代码不应占据文档主体，文字说明应占 60% 以上
- **逐步引导**：按照"是什么 → 为什么 → 怎么用"的逻辑顺序组织

### 2. 功能介绍格式

使用功能列表形式，每项功能前加 ✅ 标记：

```markdown
**特性**

- ✅ **功能名称**：简要说明
- ✅ **另一功能**：简要说明
```

### 3. 代码示例规范

代码示例必须包含：

1. **注释说明**：关键行必须有注释
2. **完整可运行**：示例代码应可直接运行（必要头文件包含）
3. **效果说明**：代码后说明运行效果，配合截图或示意图

```cpp
// 创建箱线图对象
QwtPlotBoxChart* boxChart = new QwtPlotBoxChart("标题");
boxChart->attach(plot);  // 必须附加到绘图才能显示

// 设置数据
QVector<QwtBoxSample> samples;
samples << QwtBoxSample(1.0, 10, 20, 35, 50, 60);
boxChart->setSamples(samples);  // 数据设置后自动刷新

// 效果：显示一个包含箱体、须须、中位线的箱线图
```

### 4. 概念解释要求

对于复杂概念，使用以下方式辅助说明：

- **mermaid UML图**：展示类关系、继承结构
- **mermaid 流程图**：展示工作流程
- **ASCII艺术图**：展示结构示意图
- **配图**：实际效果截图

示例（类关系图）：

```mermaid
classDiagram
    class QwtPlotSeriesItem {
        +drawSeries()
        +boundingRect()
    }
    class QwtSeriesStore~T~ {
        +setData()
        +sample()
    }
    class QwtPlotBoxChart {
        +setBoxStyle()
        +setOrientation()
    }
    
    QwtPlotSeriesItem <|-- QwtPlotBoxChart
    QwtSeriesStore <|-- QwtPlotBoxChart
```

### 5. 注意事项格式

重要信息使用引用块标注：加粗标签，可选地跟一个用破折号分隔的标题，空引用行分隔，内容逐行加 `> ` 前缀。这是普通 Markdown 语法，在 MkDocs 站点和 GitHub/GitLab 上都能正常渲染。

```markdown
> **警告 — 勿在 attach 之前装载数据**
>
> 可能导致严重问题的注意事项

> **说明 — 默认设置**
>
> 补充说明信息

> **提示**
>
> 使用技巧和建议，标题可省略
```

标签词表（与英文文档一一对应）：

| 标签 | 用途 |
|------|------|
| `说明` | 补充信息、默认行为 |
| `信息` | 背景、范围与上下文 |
| `提示` | 使用技巧、推荐做法、迁移提示 |
| `警告` | 会导致绘图错误或空白的注意事项 |
| `示例` | 示例代码位置或演示片段 |
| `已知问题` | 已知缺陷与规避方法 |
| `亮点` | 新特性与改进 |
| `摘要` | 长篇文章的内容总结 |

### 6. 属性/方法说明格式

表格形式展示核心属性和方法：

```markdown
### 核心方法

| 方法 | 参数 | 说明 |
|------|------|------|
| `setBoxStyle(Style)` | BoxStyle枚举 | 设置箱体显示样式 |
| `setOrientation(Orient)` | Qt::Orientation | 设置显示方向 |
```

## 图表使用规范

### 1. mermaid 类图

用于展示类的继承关系、组合关系：

```mermaid
classDiagram
    class BaseClass {
        +virtual method()
    }
    class DerivedClass {
        +override method()
    }
    BaseClass <|-- DerivedClass
```

### 2. mermaid 流程图

用于展示使用流程、工作流程：

```mermaid
flowchart TD
    A[创建对象] --> B[设置数据]
    B --> C[配置样式]
    C --> D[附加到绘图]
    D --> E[刷新显示]
```

### 3. 结构示意图

使用文本或 ASCII 艺术绘制结构示意：

```text
    │         ┌──┬──┐
    │         │  │  │ ← Q3
    │         │  ┼  │ ← 中位数
    │         │  │  │ ← Q1
    │    ─────┴──┴──┴─────
```

### 4. 效果截图

实际运行效果图片放在 `docs/assets/` 目录：

```markdown
![boxchart-demo](../../assets/screenshots/boxchart-demo.png)
```

对于效果截图的部分，应在前面文字描述见某个example中，例如：

```markdown
箱形图的例子位于:`examples/2D/boxchart`，例子截图如下：

![BoxChart](../../assets/screenshots/BoxChart.png)

```

## 文档风格统一

### 1. 语言风格

- 使用**中文为主**，技术术语可保留英文
- 避免口语化表达，使用正式书面语言
- 减少被动语态，多用主动语态："你可以设置..."而不是"可以设置..."

### 2. 术语规范

| 英文术语 | 中文翻译 | 说明 |
|----------|----------|------|
| Box-and-Whisker Plot | 箱线图 | 统计图表 |
| Quartile | 四分位数 | Q1, Q2, Q3 |
| Outlier | 异常值 | 超出正常范围的数据点 |
| Whisker | 须须 | 箱线图的延伸线 |

### 3. 代码风格

- 类名使用完整路径：`QwtPlotBoxChart`
- 方法名使用代码格式：`setBoxStyle()`
- 属性名使用代码格式：`boxStyle`
- 枚举值使用完整路径：`QwtPlotBoxChart::Rect`

## 撰写流程建议

1. **收集信息**：阅读类头文件、示例代码、相关文档
2. **确定结构**：按照必备章节规划文档框架
3. **编写内容**：
   - 先写功能概述和特性列表
   - 再写使用方法，每个代码块配合文字说明
   - 补充注意事项和参考资料
4. **添加图表**：绘制类图、流程图、结构示意图
5. **审阅修订**：检查代码可运行性、文字通顺度、格式一致性

## 文档模板

```markdown
# 功能名称使用指南

一句话概述类的用途和特点。

## 主要功能特性

**特性**

- ✅ **功能1**：说明
- ✅ **功能2**：说明

## 基本概念

### 核心概念解释

[文字说明]

```mermaid
classDiagram
    [类关系图]
```

## 使用方法

[如果有对应的示例，可以加入此内容]xxx的例子位于:`examples/xxx`，例子截图如下：

```markdown
![xxx](../../assets/screenshots/xxx.png)
```

### 1. 基本使用

[文字说明用途和场景]

```cpp
[代码示例]
```

[效果说明]

### 2. 进阶配置

[文字说明配置选项的作用]

| 方法 | 说明 |
|------|------|
| 方法1 | 说明 |
| 方法2 | 说明 |

```cpp
[配置代码]
```

> **警告 — setRawSamples 保存的是引用**
>
> 重要提示信息



---

本规范适用于 Qwt 所有中文使用指南文档的撰写。
