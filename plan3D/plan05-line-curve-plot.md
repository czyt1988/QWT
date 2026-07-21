请先阅读 `README.md` 以了解全局规范。

# plan05 — 3D 线图 / 参数曲线 (LinePlot3D)

## 目标

实现 3D 线图，在三维空间中绘制折线和参数曲线。支持多条线系列、沿线颜色映射、管状渲染（圆柱带）模式。与已有的 `ParametricSurface`（2D 参数曲面）不同，本计划实现的是 1D 参数曲线——三维空间中的线段。

## 前置依赖

- **plan00**（渲染基础设施）：使用 GLBuffer/GLVertexArray/ShaderProgram
- **plan02**（GraphPlot 基类）：继承 GraphPlot

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_lineplot3d.h` | LinePlot3D 类声明 |
| `src/plot3d/qwt3d_lineplot3d.cpp` | LinePlot3D 类实现 |
| `src/plot3d/qwt3d_lineplot3d_p.h` | PIMPL 私有数据 |
| `examples/3D/lineplot3d/lineplot3d.cpp` | 示例程序 |
| `examples/3D/lineplot3d/CMakeLists.txt` | 示例构建配置 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/plot3d/CMakeLists.txt` | 添加新源文件 |
| `examples/CMakeLists.txt` | 注册新示例 |

---

## 技术方案

### 1. 类设计

```cpp
namespace Qwt3D {

/// @brief Line rendering mode
enum class LineRenderMode {
    Simple,   ///< GL_LINE_STRIP — simple lines (fastest)
    Tube      ///< Cylindrical tube geometry around the line
};

/// @brief Abstract base for parametric curves in 3D
/// @details User subclasses this to define a parametric curve (x(t), y(t), z(t))
class QWT3D_EXPORT ParametricCurve
{
public:
    virtual ~ParametricCurve() = default;
    /// @brief Evaluate the curve at parameter t
    /// @return (x, y, z) position at parameter t
    virtual Triple operator()(double t) const = 0;
    /// @brief Returns the parameter domain [tMin, tMax]
    virtual std::pair<double, double> domain() const = 0;
};

/// @brief 3D line/curve plot widget
class QWT3D_EXPORT LinePlot3D : public GraphPlot
{
    Q_OBJECT

public:
    explicit LinePlot3D(QWidget* parent = nullptr);
    ~LinePlot3D() override;

    /// @brief Set a single polyline
    /// @param points Ordered vertices forming the polyline
    void setData(TripleField const& points);

    /// @brief Set multiple polylines (multi-series)
    /// @param series Vector of polylines, each polyline is a vector of Triples
    void setData(std::vector<TripleField> const& series);

    /// @brief Set data from a parametric curve
    /// @param curve  Parametric curve object
    /// @param samples Number of sample points along the curve
    void setData(ParametricCurve const& curve, unsigned samples = 1000);

    /// @brief Set line width (for Simple mode)
    void setLineWidth(double w);
    double lineWidth() const;

    /// @brief Set render mode
    void setRenderMode(LineRenderMode mode);
    LineRenderMode renderMode() const;

    /// @brief Set tube radius (for Tube mode)
    void setTubeRadius(double r);
    double tubeRadius() const;

    /// @brief Set tube quality (number of segments around circumference)
    void setTubeQuality(unsigned segments);
    unsigned tubeQuality() const;

    /// @brief Enable color mapping along the line
    /// @param enabled If true, color varies along the line based on scalar value
    void setColorMapping(bool enabled);
    bool colorMappingEnabled() const;

protected:
    QWT_DECLARE_PRIVATE(LinePlot3D)

    void createData() override;

    /// @brief Build tube geometry from polyline
    void buildTubeGeometry();

    /// @brief Render using VBO
    void renderVBO() override;

    /// @brief Render using immediate mode
    void renderImmediate() override;
};

}  // namespace Qwt3D
```

### 2. 数据模型

LinePlot3D 使用 GraphPlot 的 `GraphData`，根据渲染模式设置不同的 `PrimitiveType`：

- `Simple` 模式：`PrimitiveType::Lines`，使用 `GL_LINE_STRIP` 绘制
- `Tube` 模式：`PrimitiveType::Bars`（复用 GL_TRIANGLES），生成圆柱带几何体

```
用户调用 setData(points) 或 setData(curve, samples)
    ↓
存储为 m_series (vector<TripleField>)
    ↓
createData()
    ├─ Simple: 构建 line strip 索引 → GraphData(indices) → VBO
    └─ Tube:  构建圆柱带顶点+索引 → GraphData(vertices, indices) → VBO
```

### 3. 简单线渲染（Simple 模式）

#### 3.1 几何构建

对于每条折线，生成 `GL_LINE_STRIP` 的顶点序列：

```cpp
void LinePlot3D::buildLineGeometry()
{
    QWT_D(d);
    auto& data = graphData();
    data->clear();
    data->primitiveType = PrimitiveType::Lines;

    for (size_t s = 0; s < d->m_series.size(); ++s) {
        auto const& seg = d->m_series[s];
        unsigned baseIdx = static_cast<unsigned>(data->vertices.size());

        for (auto const& v : seg) {
            data->vertices.push_back(v);
            // Color: if color mapping enabled, use parameter-based color
            // otherwise use data color functor
            RGBA c;
            if (d->m_colorMapping) {
                double t = static_cast<double>(data->vertices.size() - baseIdx) / seg.size();
                c = (*dataColor())(t, 0, 0);  // Map parameter to color
            } else {
                c = (*dataColor())(v);
            }
            data->colors.push_back(c);
        }

        // Build line strip indices: consecutive pairs
        for (size_t i = 1; i < seg.size(); ++i) {
            data->indices.push_back(baseIdx + i - 1);
            data->indices.push_back(baseIdx + i);
        }
    }

    d->m_vboDirty = true;
}
```

#### 3.2 着色器

使用 plan00 的线着色器（`kLineVertexShader`/`kLineFragmentShader`）：

```glsl
// kLineVertexShader
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec4 aColor;

uniform mat4 uModelView;
uniform mat4 uProjection;

out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = uProjection * uModelView * vec4(aPosition, 1.0);
}
```

线宽通过 `glLineWidth()` 设置（注意：大多数实现最大线宽为 1.0-10.0，粗线需要 Tube 模式）。

### 4. 管状渲染（Tube 模式）

#### 4.1 几何构建

管状几何构建**复用 plan00 的 `TubeBuilder` 工具类**（详见 `plan00-rendering-infrastructure.md` §8）：

```cpp
void LinePlot3D::buildTubeGeometry()
{
    QWT_D(d);
    graphData().clear();
    graphData().setPrimitiveType(PrimitiveType::Bars);  // GL_TRIANGLES

    TubeBuilder::Config config;
    config.radius = d->m_tubeRadius;
    config.segments = d->m_tubeQuality;
    config.capEnds = true;

    for (auto const& seg : d->m_series) {
        if (seg.size() < 2) continue;

        TriangleMesh tube;
        auto colorFunc = [this](Triple const& v) -> RGBA {
            return (*dataColor())(v);
        };
        TubeBuilder::build(seg, config, tube, colorFunc);

        // Merge tube mesh into GraphData
        appendMesh(tube);
    }

    d->m_vboDirty = true;
}
```

`TubeBuilder` 使用 parallel transport frame（而非 Frenet-Serret 标架），避免在拐点处产生法线跳变和管体扭曲。

#### 4.2 着色器

Tube 模式使用表面着色器（`kSurfaceVertexShader`/`kSurfaceFragmentShader`），支持光照和法线。

### 5. 参数曲线支持

```cpp
void LinePlot3D::setData(ParametricCurve const& curve, unsigned samples)
{
    TripleField points;
    auto [tMin, tMax] = curve.domain();
    double dt = (tMax - tMin) / samples;

    for (unsigned i = 0; i <= samples; ++i) {
        double t = tMin + i * dt;
        points.push_back(curve(t));
    }

    setData(points);
}
```

用户通过继承 `ParametricCurve` 定义曲线：

```cpp
// Example: Helix
class Helix : public Qwt3D::ParametricCurve {
    Triple operator()(double t) const override {
        return Triple(cos(t), sin(t), t * 0.1);
    }
    std::pair<double, double> domain() const override {
        return {0, 20 * PI};
    }
};

// Usage:
plot->setData(Helix(), 2000);
```

### 6. 多系列管理

每条线系列可以有独立的颜色：

```cpp
// Future API consideration (not in initial scope):
void setSeriesColor(unsigned seriesIndex, RGBA color);
void setSeriesWidth(unsigned seriesIndex, double width);
```

初始版本中，所有系列使用相同的 `Color` functor，颜色基于位置或参数。

### 7. Immediate Mode 回退

```cpp
void LinePlot3D::renderImmediate()
{
    QWT_DC(d);
    auto const& data = graphData();
    const Color* col = dataColor();

    if (d->m_renderMode == LineRenderMode::Simple) {
        // Use GL_LINES with index pairs (not GL_LINE_STRIP) to avoid
        // connecting different series' endpoints
        glLineWidth(static_cast<GLfloat>(d->m_lineWidth));
        glBegin(GL_LINES);
        if (!data->indices.empty()) {
            for (size_t i = 0; i < data->indices.size(); i += 2) {
                for (int j = 0; j < 2; ++j) {
                    unsigned idx = data->indices[i + j];
                    Triple const& v = data->vertices[idx];
                    RGBA c = data->colors.empty() ? (*col)(v) : data->colors[idx];
                    glColor4d(c.r, c.g, c.b, c.a);
                    glVertex3d(v.x, v.y, v.z);
                }
            }
        } else {
            // Single series fallback: consecutive pairs
            for (size_t i = 0; i + 1 < data->vertices.size(); i += 2) {
                for (int j = 0; j < 2; ++j) {
                    Triple const& v = data->vertices[i + j];
                    RGBA c = data->colors.empty() ? (*col)(v) : data->colors[i + j];
                    glColor4d(c.r, c.g, c.b, c.a);
                    glVertex3d(v.x, v.y, v.z);
                }
            }
        }
        glEnd();
    } else {
        // Tube mode: draw triangles
        glBegin(GL_TRIANGLES);
        for (size_t i = 0; i < data->indices.size(); i += 3) {
            for (int j = 0; j < 3; ++j) {
                unsigned idx = data->indices[i + j];
                Triple const& v = data->vertices[idx];
                RGBA const& c = data->colors[idx];
                Triple const& n = data->normals[idx];
                glNormal3d(n.x, n.y, n.z);
                glColor4d(c.r, c.g, c.b, c.a);
                glVertex3d(v.x, v.y, v.z);
            }
        }
        glEnd();
    }
}
```

---

## 验证方式

1. **编译验证**：`.\build.ps1 build`
2. **示例验证**：运行 `examples/3D/lineplot3d`，展示：
   - 螺旋线（Helix）参数曲线
   - Lorenz 吸引子轨迹（多段折线）
   - Simple 和 Tube 两种渲染模式切换
   - 沿线颜色映射（起点=蓝色，终点=红色）
3. **线宽验证**：Simple 模式下调整线宽，Tube 模式下调整管半径
4. **光照验证**：Tube 模式下启用光照，验证管体有正确的光照效果
5. **性能验证**：渲染 10K 点的曲线，Simple 模式帧率 > 60fps，Tube 模式(16段) > 30fps
6. **回退验证**：在兼容上下文下验证 immediate mode 渲染正常

---

## 风险与注意事项

1. **线宽限制**：`glLineWidth()` 在大多数 OpenGL 实现中最大值有限（通常 1.0-10.0）。对于粗线，必须使用 Tube 模式。

2. **管体法线**：Tube 模式中，每个顶点的法线应指向管体外侧（即 `offset` 方向，已在代码中实现）。错误的法线会导致光照不正确。

3. **管体接缝**：当折线有锐角弯折时，管体在弯折处可能产生拉伸或自相交。可以通过在弯折处增加额外采样点来缓解。

4. **参数曲线采样密度**：采样点太少会导致曲线不平滑。`setData(curve, samples)` 的 `samples` 参数应根据曲线复杂度调整。默认 1000 适用于大多数情况。

5. **与 ParametricSurface 的区别**：`ParametricSurface`（已实现）是 2D 参数 → 3D 曲面 `(x,y,z)=f(u,v)`，而 `ParametricCurve`（本计划新增）是 1D 参数 → 3D 曲线 `(x,y,z)=f(t)`。两者完全不同，不要混淆。

6. **多系列颜色**：初始版本中多系列使用统一颜色 functor。如果需要每系列独立颜色，可以在 `GraphData::colors` 中为不同系列设置不同颜色（在 `buildLineGeometry()` / `buildTubeGeometry()` 中按系列索引设置颜色）。

7. **沿曲线标量映射**：颜色映射可以基于弧长参数（0=起点，1=终点）或基于外部标量值。当前设计使用弧长参数，如需外部标量可扩展 `setData()` 接受标量数组。
